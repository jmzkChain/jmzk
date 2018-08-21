/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/evt_link_plugin/evt_link_plugin.hpp>

#include <deque>
#include <fc/io/json.hpp>
#include <fc/container/flat_fwd.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

#include <evt/utilities/spinlock.hpp>

namespace evt {

static appbase::abstract_plugin& _evt_link_plugin = app().register_plugin<evt_link_plugin>();

using namespace evt;
using evt::chain::bytes;
using evt::chain::link_id_type;
using evt::chain::block_state_ptr;
using evt::chain::contracts::evt_link;
using evt::chain::contracts::everipay;
using evt::utilities::spinlock;
using evt::utilities::spinlock_guard;
using std::condition_variable_any;

class evt_link_plugin_impl {
public:
    evt_link_plugin_impl(controller& db)
        : db_(db) {}
    ~evt_link_plugin_impl();

public:
    void init();

private:
    void consume_queues();
    void applied_block(const chain::block_state_ptr& bs);
    void _applied_block(const chain::block_state_ptr& bs);

    void get_trx_id_for_link_id(const link_id_type& link_id, deferred_id id);

private:
    controller& db_;

    spinlock                    lock_;
    condition_variable_any      cond_;
    std::thread                 consume_thread_;
    std::atomic_bool            done_{false};
    std::atomic_bool            init_{false};

    std::deque<block_state_ptr>             blocks_;
    fc::flat_map<link_id_type, deferred_id> link_ids_;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection_;

private:
    friend class evt_link_plugin;
};

void
evt_link_plugin_impl::consume_queues() {
    try {
        while(true) {
            lock_.lock();
            while(blocks_.empty() && !done_) {
                cond_.wait(lock_);
            }

            auto blocks = std::move(blocks_);
            lock_.unlock();

            if(done_) {
                ilog("draining queue, size: ${q}", ("q", blocks.size()));
                break;
            }

            auto it = blocks.begin();
            while(it != blocks.end()) {
                _applied_block(*it);
                it++;

                blocks.pop_front();
            }
        }
        ilog("evt_link_plugin consume thread shutdown gracefully");
    }
    catch(fc::exception& e) {
        elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while consuming block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while consuming block");
    }
}

void
evt_link_plugin_impl::applied_block(const chain::block_state_ptr& bs) {
    if(!init_) {
        {
            spinlock_guard l(lock_);
            blocks_.emplace_back(bs);
        }
        cond_.notify_one();
    }
}

void
evt_link_plugin_impl::_applied_block(const chain::block_state_ptr& bs) {
    lock_.lock();
    if(link_ids_.empty()) {
        lock_.unlock();
        return;
    }
    lock_.unlock();

    for(auto& trx : bs->trxs) {
        for(auto& act : trx->trx.actions) {
            if(act.name != N(everipay)) {
                continue;
            }

            auto& epact       = act.data_as<const everipay&>();
            auto& link_id_str = *epact.link.get_segment(evt_link::link_id).strv;

            lock_.lock();
            auto it = link_ids_.find(*(link_id_type*)(link_id_str.data()));
            if(it != link_ids_.end()) {
                lock_.unlock();

                auto vo         = fc::mutable_variant_object();
                vo["block_num"] = bs->block_num;
                vo["trx_id"]    = trx->id;

                app().get_plugin<http_plugin>().set_deferred_response(it->second, 200, fc::json::to_string(vo));

                spinlock_guard l(lock_);
                link_ids_.erase(it);
                return;
            }
            lock_.unlock();
        }
    }
}

void
evt_link_plugin_impl::get_trx_id_for_link_id(const link_id_type& link_id, deferred_id id) {
    // try to fetch from chain first
    try {
        auto& obj = db_.get_link_obj_for_link_id(link_id);
        if(obj.block_num > db_.fork_db_head_block_num()) {
            // block not finalize yet
            spinlock_guard l(lock_);
            link_ids_.emplace(link_id, id);
            return;
        }

        auto vo         = fc::mutable_variant_object();
        vo["block_num"] = obj.block_num;
        vo["trx_id"]    = obj.trx_id;

        app().get_plugin<http_plugin>().set_deferred_response(id, 200, fc::json::to_string(vo));
    }
    catch(const chain::evt_link_existed_exception&) {
        // cannot find now, put into map
        spinlock_guard l(lock_);
        link_ids_.emplace(link_id, id);
    }
}

void
evt_link_plugin_impl::init() {
    init_ = true;

    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection_.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));

    consume_thread_ = std::thread([this] { consume_queues(); });
}

evt_link_plugin_impl::~evt_link_plugin_impl() {
    try {
        done_ = true;
        cond_.notify_one();

        consume_thread_.join();
    }
    catch(std::exception& e) {
        elog("Exception on evt_link_plugin shutdown of consume thread: ${e}", ("e", e.what()));
    }
}

evt_link_plugin::evt_link_plugin() {}
evt_link_plugin::~evt_link_plugin() {}

void
evt_link_plugin::set_program_options(options_description&, options_description&) {}

void
evt_link_plugin::plugin_initialize(const variables_map&) {
    my_.reset(new evt_link_plugin_impl(app().get_plugin<chain_plugin>().chain()));
    my_->init();
}

void
evt_link_plugin::plugin_startup() {
    ilog("starting evt_link_plugin");

    app().get_plugin<http_plugin>().add_deferred_handler("/v1/evt_link/get_trx_id_for_link_id", [&](auto, auto body, auto id) {
        try {
            auto var  = fc::json::from_string(body);
            auto data = bytes();
            fc::from_variant(var["link_id"], data);

            if(data.size() != sizeof(link_id_type)) {
                EVT_THROW(chain::evt_link_id_exception, "EVT-Link id is not in proper length");
            }

            my_->get_trx_id_for_link_id(*(link_id_type*)&data[0], id);
        }
        catch(...) {
            http_plugin::handle_exception("evt_link", "get_trx_id_for_link_id", body, [id](auto code, auto body) {
                app().get_plugin<http_plugin>().set_deferred_response(id, code, body);
            });
        }
    });

    my_->init_ = false;
}

void
evt_link_plugin::plugin_shutdown() {}

}  // namespace evt
