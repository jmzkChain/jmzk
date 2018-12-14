/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/trafficgen_plugin/trafficgen_plugin.hpp>

#include <signal.h>
#include <boost/asio/post.hpp>

#include <evt/chain/exceptions.hpp>
#include <evt/chain/transaction.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>

namespace evt {

static appbase::abstract_plugin& _trafficgen_plugin = app().register_plugin<trafficgen_plugin>();

using evt::chain::block_id_type;
using evt::chain::block_state_ptr;
using evt::chain::address;
using evt::chain::packed_transaction_ptr;
using evt::chain::private_key_type;

class trafficgen_plugin_impl : public std::enable_shared_from_this<trafficgen_plugin_impl> {
public:
    trafficgen_plugin_impl(controller& db)
        : db_(db) {}

public:
    void init();
    void pre_generate(const block_id_type& id);

private:
    void applied_block(const block_state_ptr& bs);
    void push_once(int index);

public:
    controller& db_;

    uint32_t start_num_  = 0;
    size_t   total_num_  = 0;
    bool     pushed_     = false;

    address          from_addr_;
    private_key_type from_priv_;

    std::string type_;

    std::vector<packed_transaction_ptr> packed_trxs_;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection_;
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
trafficgen_plugin_impl::pre_generate(const block_id_type& id) {
    using namespace evt::chain;
    using namespace evt::chain::contracts;

    ilog("Generating ptrxs...");

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

    ilog("Generating ptrxs... Done");
}

void
trafficgen_plugin_impl::applied_block(const block_state_ptr& bs) {
    if(bs->block_num >= start_num_) {
        if(packed_trxs_.empty()) {
            pre_generate(bs->id);
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
