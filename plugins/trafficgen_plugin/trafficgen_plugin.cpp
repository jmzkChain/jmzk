/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/trafficgen_plugin/trafficgen_plugin.hpp>

#include <signal.h>
#include <boost/asio/post.hpp>

#include <evt/chain/exceptions.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/token_database.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>

namespace evt {

static appbase::abstract_plugin& _trafficgen_plugin = app().register_plugin<trafficgen_plugin>();

using evt::chain::address;
using evt::chain::action;
using evt::chain::block_id_type;
using evt::chain::block_state_ptr;
using evt::chain::packed_transaction_ptr;
using evt::chain::private_key_type;

class trafficgen_plugin_impl : public std::enable_shared_from_this<trafficgen_plugin_impl> {
public:
    trafficgen_plugin_impl(controller& db)
        : db_(db) {}

public:
    void init();

private:
    int  pre_nft_setup(const block_id_type& id);
    void pre_ft_generate(const block_id_type& id);
    void pre_nft_generate(const block_id_type& id);
    void applied_block(const block_state_ptr& bs);
    void push_once(int index);
    void push_trx(const action& act, const block_id_type& id);

public:
    controller& db_;

    uint32_t start_num_  = 0;
    size_t   total_num_  = 0;
    bool     pushed_     = false;

    address          from_addr_;
    private_key_type from_priv_;

    std::string type_;

    std::vector<packed_transaction_ptr> packed_trxs_;

    std::optional<boost::signals2::scoped_connection> accepted_block_connection_;
};

void
trafficgen_plugin_impl::init() {
    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection_.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));
}

void
trafficgen_plugin_impl::push_trx(const action& act, const block_id_type& id) {
    using namespace evt::chain;
    using namespace evt::chain::contracts;

    auto now = fc::time_point::now();
    auto trx = signed_transaction();
    trx.set_reference_block(id);
    trx.actions.emplace_back(act);
    trx.expiration = now + fc::minutes(10);
    trx.payer = from_addr_;
    trx.max_charge = 10000;

    trx.sign(from_priv_, db_.get_chain_id());

    auto ptrx = std::make_shared<packed_transaction>(trx);
    app().get_method<chain::plugin_interface::incoming::methods::transaction_async>()(ptrx, true, [](const auto& result) -> void {
        if(result.template contains<fc::exception_ptr>()) {
            wlog("Push init trx failed e: ${e}", ("e",*result.template get<fc::exception_ptr>()));
        }
    });
}

int
trafficgen_plugin_impl::pre_nft_setup(const block_id_type& id) {
    using namespace evt::chain;
    using namespace evt::chain::contracts;

    auto& tdb = db_.token_db();
    if(tdb.exists_token(token_type::domain, std::nullopt, "tttesttt")) {
        auto d = domain_def();
        auto s = std::string();
        tdb.read_token(token_type::domain, std::nullopt, "tttesttt", s);

        auto ds = fc::datastream<const char*>(s.data(), s.size());
        fc::raw::unpack(ds, d);

        if(d.creator != from_addr_) {
            ilog("Test domain created by another address: ${a} but provided is: ${p}", ("a",d.creator)("b",from_addr_));
            return 0;
        }
        return 1;
    }

    ilog("Generating pre nft trx...");

    auto nd    = newdomain();
    nd.name    = "tttesttt";
    nd.creator = from_addr_.get_public_key();

    auto issue = permission_def();
    issue.name = N(issue);
    issue.threshold = 1;
    issue.authorizers.emplace_back(authorizer_weight(authorizer_ref(from_addr_.get_public_key()), 1));

    auto manage = permission_def();
    manage.name = N(manage);
    manage.threshold = 0;

    auto transfer = permission_def();
    transfer.name = N(transfer);
    transfer.threshold = 1;
    transfer.authorizers.emplace_back(authorizer_weight(authorizer_ref(from_addr_.get_public_key()), 1));

    nd.issue    = issue;
    nd.transfer = transfer;
    nd.manage   = manage;

    auto ndact = action(N128(tttesttt), N128(.create), nd);
    push_trx(ndact, id);

    for(auto i = 0; i < 20; i++) {
        auto it   = issuetoken();
        it.domain = "tttesttt";
        it.owner.emplace_back(from_addr_);
        for(auto j = i * 10'000; j < 10'000 * (i + 1); j++) {
            it.names.emplace_back(name128::from_number(j));
        }

        auto itact = action(N128(tttesttt), N128(.issue), it);
        push_trx(itact, id);
    }

    ilog("Generating pre nft trx... Done");
    return 1;
}

void
trafficgen_plugin_impl::pre_ft_generate(const block_id_type& id) {
    using namespace evt::chain;
    using namespace evt::chain::contracts;

    ilog("Generating ft ptrxs...");

    packed_trxs_.reserve(total_num_);

    auto tt   = transferft();
    tt.from   = from_addr_;
    tt.number = asset(10, evt_sym());
    tt.memo   = "FROM THE NEW WORLD";

    auto ttact = action(N128(.fungible), N128(1), tt);

    auto now = fc::time_point::now();
    for(auto i = 0u; i < total_num_; i++) {
        auto priv = private_key_type::generate();
        auto pub  = priv.get_public_key();

        tt.to = pub;
        ttact.set_data(tt);

        auto trx = signed_transaction();
        trx.set_reference_block(id);
        trx.actions.emplace_back(ttact);
        trx.expiration = now + fc::minutes(10);
        trx.payer = from_addr_;
        trx.max_charge = 10000;
        trx.sign(from_priv_, db_.get_chain_id());

        packed_trxs_.emplace_back(std::make_shared<packed_transaction>(trx));
    }

    ilog("Generating ft ptrxs... Done");
}

void
trafficgen_plugin_impl::pre_nft_generate(const block_id_type& id) {
    using namespace evt::chain;
    using namespace evt::chain::contracts;

    ilog("Generating nft ptrxs...");

    packed_trxs_.reserve(total_num_);

    auto tt   = transfer();
    tt.domain = "tttesttt";
    tt.memo   = "FROM THE NEW WORLD";

    auto ttact = action(N128(tttesttt), N128(0), tt);

    auto now = fc::time_point::now();
    for(auto i = 0u; i < total_num_; i++) {
        auto priv = private_key_type::generate();
        auto pub  = priv.get_public_key();

        tt.name = name128::from_number(i);
        tt.to.emplace_back(pub);
        ttact.set_data(tt);
        ttact.key = name128::from_number(i);

        auto trx = signed_transaction();
        trx.set_reference_block(id);
        trx.actions.emplace_back(ttact);
        trx.expiration = now + fc::minutes(10);
        trx.payer = from_addr_;
        trx.max_charge = 10000;
        trx.sign(from_priv_, db_.get_chain_id());

        packed_trxs_.emplace_back(std::make_shared<packed_transaction>(trx));
    }

    ilog("Generating nft ptrxs... Done");
}

void
trafficgen_plugin_impl::applied_block(const block_state_ptr& bs) {
    if(bs->block_num >= start_num_) {
        if(packed_trxs_.empty()) {
            if(type_ == "ft") {
                pre_ft_generate(bs->id);
            }
            else if(type_ == "nft") {
                if(pre_nft_setup(bs->id)) {
                    pre_nft_generate(bs->id);
                }
            }
        }
        else {
            auto now = fc::time_point::now();
            if(!pushed_ && std::abs((db_.head_block_time() - now).to_seconds()) < 1) {
                const auto& exec = app().get_io_service().get_executor();
                for(auto i = 0u; i < total_num_; i++) {
                    boost::asio::post(exec, std::bind(&trafficgen_plugin_impl::push_once, this, (int)i));
                }
                pushed_ = true;
            }
        }
    }
}

void
trafficgen_plugin_impl::push_once(int index) {
    try {
        auto ptrx = packed_trxs_[index];
        app().get_method<chain::plugin_interface::incoming::methods::transaction_async>()(ptrx, true, [index](const auto& result) -> void {
            if(result.template contains<fc::exception_ptr>()) {
                wlog("Push failed at index: ${i}, e: ${e}", ("i",index)("e",*result.template get<fc::exception_ptr>()));
            }
        });
    }
    catch(boost::interprocess::bad_alloc&) {
        raise(SIGUSR1);
    }
    catch(fc::unrecoverable_exception&) {
        raise(SIGUSR1);
    }
    catch(...) {
        wlog("Push failed at index: ${i}", ("i",index));
    }
}

void
trafficgen_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("traffic-start-num", bpo::value<uint32_t>()->default_value(0), "From which block num start trafficgen.")
        ("traffic-total", bpo::value<size_t>()->default_value(0), "Total transactions to be generated")
        ("traffic-from", bpo::value<std::string>(), "Address of sender when generating")
        ("traffic-from-priv", bpo::value<std::string>(), "Private key of sender when generating")
        ("traffic-type", bpo::value<std::string>()->default_value("ft"), "Type of transactions, can be 'nft' or 'ft'")
    ;
}

void
trafficgen_plugin::plugin_initialize(const variables_map& options) {
    my_ = std::make_shared<trafficgen_plugin_impl>(app().get_plugin<chain_plugin>().chain());
    my_->start_num_ = options.at("traffic-start-num").as<uint32_t>();
    my_->total_num_ = options.at("traffic-total").as<size_t>();

    EVT_ASSERT(my_->total_num_ <= 200'000, chain::plugin_config_exception, "Total number of generating transactions cannot be large than 200'000");

    if(options.count("traffic-from") && options.count("traffic-from-priv")) {
        my_->from_addr_ = address(options.at("traffic-from").as<std::string>());
        my_->from_priv_ = private_key_type(options.at("traffic-from-priv").as<std::string>());
        my_->init(); 
    }

    if(options.count("traffic-type")) {
        auto type = options.at("traffic-type").as<std::string>();
        EVT_ASSERT(type == "ft" || type == "nft", chain::plugin_config_exception, "Not valid value for --traffic-type option");
        my_->type_ = type;
    }
}

void
trafficgen_plugin::plugin_startup() {
    ilog("starting trafficgen_plugin");
}

void
trafficgen_plugin::plugin_shutdown() {
    my_->accepted_block_connection_.reset();
    my_.reset();
}

}  // namespace evt
