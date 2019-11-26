/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/staking_plugin/staking_plugin.hpp>

#include <evt/chain/types.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/contracts/types.hpp>

namespace evt {

using namespace evt::chain;
using namespace evt::chain::contracts;

static appbase::abstract_plugin& _staking_plugin = app().register_plugin<staking_plugin>();

class staking_plugin_impl : public std::enable_shared_from_this<staking_plugin_impl> {
public:
    using signature_provider_type = std::function<chain::signature_type(chain::digest_type)>;

    struct staking_config {
        account_name     validator;
        fc::microseconds evtwd_provider_timeout_us;
        public_key_type  payer;

        std::map<chain::public_key_type, signature_provider_type> signature_providers;
    };

public:
    staking_plugin_impl(controller& db)
        : db_(db) {}
    ~staking_plugin_impl();

public:
    void init(const staking_config& config);

private:
    void applied_block(const block_state_ptr& bs);

public:
    uint32_t       current_period_start_num_;
    uint32_t       last_recv_period_start_num_;
    controller&    db_;
    staking_config config_;
    bool           init_ = true;

    std::optional<boost::signals2::scoped_connection> accepted_block_connection_;
};

void
staking_plugin_impl::applied_block(const block_state_ptr& bs) {
    if(init_) {
        return;
    }

    auto curr_block_num = bs->block_num;

    auto& ctx  = db_.get_global_properties().staking_ctx;
    auto& conf = db_.get_global_properties().staking_configuration;

    if(last_recv_period_start_num_ == ctx.period_start_num) {
        // already received
        return;
    }
    if(current_period_start_num_ < ctx.period_start_num) {
        // switch to next period
        current_period_start_num_ = ctx.period_start_num;
        return;
    }
    else {
        // at this period but not received
        if(curr_block_num <= ctx.period_start_num + (conf.cycles_per_period - 1) * conf.blocks_per_cycle) {
            // not receive cycle yet
            return;
        }
    }

    auto recv      = recvstkbonus();
    recv.validator = config_.validator;
    recv.sym_id    = EVT_SYM_ID;

    auto trx = signed_transaction();
    trx.actions.emplace_back(action(N128(.staking), config_.validator, recv));

    trx.expiration = db_.fork_db_head_block_time() + fc::seconds(30);
    trx.payer      = config_.payer;
    trx.max_charge = 10000;
    trx.set_reference_block(db_.fork_db_head_block_id());

    auto digest = trx.sig_digest(db_.get_chain_id());
    for(auto& pair : config_.signature_providers) {
        trx.signatures.emplace_back(pair.second(digest));
    }

    auto ptrx = std::make_shared<packed_transaction>(trx);
    app().get_method<chain::plugin_interface::incoming::methods::transaction_async>()(std::make_shared<transaction_metadata>(ptrx), true, [](const auto& result) -> void {
        if(result.template contains<fc::exception_ptr>()) {
            wlog("Push trx failed: ${e}", ("e",result.template get<fc::exception_ptr>()->to_string()));
        }
        else {
            ilog("Received staking bonus");
        }
    });

    // update state
    last_recv_period_start_num_ = ctx.period_start_num;
}

void
staking_plugin_impl::init(const staking_config& config) {
    config_ = config;

    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection_.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));
}

staking_plugin_impl::~staking_plugin_impl() {}

staking_plugin::staking_plugin() {}
staking_plugin::~staking_plugin() {}

void
staking_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("staking-validator", bpo::value<std::string>(), "Registered validator for staking.")
        ("staking-payer", bpo::value<std::string>(), "Payer address for pushing trx.")
        ("staking-signature-provider", boost::program_options::value<vector<string>>()->composing()->multitoken(),
            "Key=Value pairs in the form <public-key>=<provider-spec>\n"
            "Where:\n"
            "   <public-key>    \tis a string form of a vaild EVT public key\n\n"
            "   <provider-spec> \tis a string in the form <provider-type>:<data>\n\n"
            "   <provider-type> \tis KEY, or EVTWD\n\n"
            "   KEY:<data>      \tis a string form of a valid EVT private key which maps to the provided public key\n\n"
            "   EVTWD:<data>    \tis the URL where evtwd is available and the approptiate wallet(s) are unlocked")
        ("staking-evtwd-provider-timeout", boost::program_options::value<int32_t>()->default_value(5), "Limits the maximum time (in milliseconds) that is allowed for pushing staking trx to a evtwd provider for signing")
    ;
}

static staking_plugin_impl::signature_provider_type
make_key_signature_provider(const private_key_type& key) {
    return [key](const chain::digest_type& digest) {
        return key.sign(digest);
    };
}

static staking_plugin_impl::signature_provider_type
make_evtwd_signature_provider(const std::shared_ptr<staking_plugin_impl>& impl, const string& url_str, const public_key_type pubkey) {
    auto evtwd_url = fc::url(url_str);
    std::weak_ptr<staking_plugin_impl> weak_impl = impl;

    return [weak_impl, evtwd_url, pubkey](const chain::digest_type& digest) {
        auto impl = weak_impl.lock();
        if(impl) {
            fc::variant params;
            fc::to_variant(std::make_pair(digest, pubkey), params);
            auto deadline = impl->config_.evtwd_provider_timeout_us.count() >= 0 ? fc::time_point::now() + impl->config_.evtwd_provider_timeout_us : fc::time_point::maximum();
            return app().get_plugin<http_client_plugin>().get_client().post_sync(evtwd_url, params, deadline).as<chain::signature_type>();
        }
        else {
            return signature_type();
        }
    };
}

void
staking_plugin::plugin_initialize(const variables_map& options) {
    my_ = std::make_shared<staking_plugin_impl>(app().get_plugin<chain_plugin>().chain());

    auto config = staking_plugin_impl::staking_config();
    try {
        config.validator = options["staking-validator"].as<std::string>();
        config.payer     = public_key_type(options["staking-payer"].as<std::string>());

        if(options.count("staking-signature-provider")) {
            const auto& key_spec_pairs = options["staking-signature-provider"].as<std::vector<std::string>>();
            for(const auto& key_spec_pair : key_spec_pairs) {
                try {
                    auto delim = key_spec_pair.find("=");
                    EVT_ASSERT(delim != std::string::npos, plugin_config_exception, "Missing \"=\" in the key spec pair");
                    auto pub_key_str = key_spec_pair.substr(0, delim);
                    auto spec_str    = key_spec_pair.substr(delim + 1);

                    auto spec_delim = spec_str.find(":");
                    EVT_ASSERT(spec_delim != std::string::npos, plugin_config_exception, "Missing \":\" in the key spec pair");
                    auto spec_type_str = spec_str.substr(0, spec_delim);
                    auto spec_data     = spec_str.substr(spec_delim + 1);

                    auto pubkey = public_key_type(pub_key_str);

                    if(spec_type_str == "KEY") {
                        auto privkey = private_key_type(spec_data);
                        config.signature_providers[pubkey] = make_key_signature_provider(privkey);
                        FC_ASSERT(privkey.get_public_key() == pubkey,
                            "Public key provided with private key should be paired, provided: {p1}, expected: {p2}", ("p1", privkey.get_public_key())("p2",pubkey));

                    }
                    else if(spec_type_str == "EVTWD") {
                        config.signature_providers[pubkey] = make_evtwd_signature_provider(my_, spec_data, pubkey);
                    }
                    else {
                        EVT_THROW(plugin_config_exception, "Invalid key provider");
                    }
                }
                catch(...) {
                    elog("Malformed signature provider: \"${val}\", ignoring!", ("val", key_spec_pair));
                }
            }
        }

        EVT_ASSERT(config.signature_providers.find(config.payer) != config.signature_providers.cend(),
            plugin_config_exception, "Must provide signature provider for payer");

        config.evtwd_provider_timeout_us = fc::milliseconds(options.at("staking-evtwd-provider-timeout").as<int32_t>());
    }
    FC_LOG_AND_RETHROW();

    my_->init(config);
}

void
staking_plugin::plugin_startup() {
    ilog("starting staking_plugin");
    my_->init_ = false;
}

void
staking_plugin::plugin_shutdown() {
    my_->accepted_block_connection_.reset();
    my_.reset();
}

} // namespace evt
