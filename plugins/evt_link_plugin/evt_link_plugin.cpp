/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/evt_link_plugin/evt_link_plugin.hpp>

#include <deque>
#include <tuple>
#include <chrono>
#include <unordered_map>
#include <thread>
#if __has_include(<condition>)
#include <condition>
using std::condition_variable_any;
#else
#include <boost/thread/condition.hpp>
using boost::condition_variable_any;
#endif

#define XXH_INLINE_ALL
#include <xxhash.h>

#include <boost/asio.hpp>
#include <fc/io/json.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

#include <evt/utilities/spinlock.hpp>

namespace evt {

static appbase::abstract_plugin& _evt_link_plugin = app().register_plugin<evt_link_plugin>();

using evt::chain::bytes;
using evt::chain::link_id_type;
using evt::chain::block_state_ptr;
using evt::chain::transaction_trace_ptr;
using evt::chain::contracts::evt_link;
using evt::chain::contracts::everipay;
using evt::utilities::spinlock;
using evt::utilities::spinlock_guard;

using boost::asio::steady_timer;
using steady_timer_ptr = std::shared_ptr<steady_timer>;

struct evt_link_id_hasher {
    size_t
    operator()(const link_id_type& id) const {
        return XXH64(&id, sizeof(id), 0);
    }
};

class evt_link_plugin_impl : public std::enable_shared_from_this<evt_link_plugin_impl> {
public:
    using deferred_pair = std::pair<deferred_id, steady_timer_ptr>;
    enum { kDeferredId = 0, kTimer };

public:
    evt_link_plugin_impl(controller& db)
        : db_(db) {}
    ~evt_link_plugin_impl();

public:
    void init();
    void get_trx_id_for_link_id(const link_id_type& link_id, deferred_id id);
    void add_and_schedule(const link_id_type& link_id, deferred_id id);

private:
    void consume_queues();
    void applied_block(const block_state_ptr& bs);
    void _applied_block(const block_state_ptr& bs);
    void applied_transaction(const transaction_trace_ptr&);
    void _applied_transaction(const transaction_trace_ptr&);

    template<typename T>
    void response(const link_id_type& link_id, T&& response_fun);

public:
    controller& db_;

    spinlock               lock_;
    condition_variable_any cond_;
    std::thread            consume_thread_;
    std::atomic_bool       done_{false};
    std::atomic_bool       init_{false};
    uint32_t               timeout_;

    std::deque<block_state_ptr>                                              blocks_;
    std::unordered_multimap<link_id_type, deferred_pair, evt_link_id_hasher> link_ids_;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection_;
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
evt_link_plugin_impl::applied_block(const block_state_ptr& bs) {
    if(!init_) {
        {
            spinlock_guard l(lock_);
            blocks_.emplace_back(bs);
        }
        cond_.notify_one();
    }
}

void
evt_link_plugin_impl::_applied_block(const block_state_ptr& bs) {
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

            auto& epact = act.data_as<const everipay&>();

            response(epact.link.get_link_id(), [&] {
                auto vo         = fc::mutable_variant_object();
                vo["block_num"] = bs->block_num;
                vo["trx_id"]    = trx->id;
                vo["err_code"]  = 0;

                return fc::json::to_string(vo);
            });
        }
    }
}

template<typename T>
void
evt_link_plugin_impl::response(const link_id_type& link_id, T&& response_fun) {
    lock_.lock();
    auto sz = link_ids_.count(link_id);
    if(sz > 0) {
        lock_.unlock();

        auto json = response_fun();
        auto wptr = std::weak_ptr<evt_link_plugin_impl>(shared_from_this());
        boost::asio::post(app().get_io_service(), [wptr, json, link_id] {
            auto self = wptr.lock();
            if(self) {
                self->lock_.lock();
                auto pair = self->link_ids_.equal_range(link_id);
                if(pair.first != self->link_ids_.end()) {
                    self->lock_.unlock();

                    std::for_each(pair.first, pair.second, [&](auto& it) {
                        app().get_plugin<http_plugin>().set_deferred_response(std::get<kDeferredId>(it.second), 200, json);
                    });

                    spinlock_guard l(self->lock_);
                    self->link_ids_.erase(link_id);
                    return;
                }
                else {
                    self->lock_.unlock();
                }
            }
        });
    }
    else {
        lock_.unlock();
    }
}

void
evt_link_plugin_impl::add_and_schedule(const link_id_type& link_id, deferred_id id) {
    lock_.lock();
    auto it = link_ids_.emplace(link_id, std::make_pair(id, std::make_shared<steady_timer>(app().get_io_service())));
    lock_.unlock();

    auto timer = std::get<kTimer>(it->second);
    timer->expires_from_now(std::chrono::milliseconds(timeout_));
    
    auto wptr = std::weak_ptr<evt_link_plugin_impl>(shared_from_this());
    timer->async_wait([wptr, link_id](auto& ec) {
        auto self = wptr.lock();
        if(self && ec != boost::asio::error::operation_aborted) {
            self->lock_.lock();
            auto pair = self->link_ids_.equal_range(link_id);
            if(pair.first == self->link_ids_.end()) {
                self->lock_.unlock();
                wlog("Cannot find context for id: ${id}", ("id",link_id));
                return;
            }
            
            auto ids = std::vector<deferred_id>();
            std::for_each(pair.first, pair.second, [&ids](auto& it) {
                ids.emplace_back(std::get<kDeferredId>(it.second));
            });

            self->link_ids_.erase(link_id);
            self->lock_.unlock();

            try {
                EVT_THROW(chain::exceed_evt_link_watch_time_exception, "Exceed EVT-Link watch time: ${time} ms", ("time",self->timeout_));
            }
            catch(...) {
                http_plugin::handle_exception("evt_link", "get_trx_id_for_link_id", "", [&ids](auto code, auto body) {
                    for(auto id : ids) {
                        app().get_plugin<http_plugin>().set_deferred_response(id, code, body);
                    }
                });
            }
        }
    });
}

void
evt_link_plugin_impl::get_trx_id_for_link_id(const link_id_type& link_id, deferred_id id) {
    // try to fetch from chain first
    try {
        auto& obj = db_.get_link_obj_for_link_id(link_id);
        if(obj.block_num > db_.fork_db_head_block_num()) {
            // block not finalize yet
            add_and_schedule(link_id, id);
            return;
        }

        auto vo         = fc::mutable_variant_object();
        vo["block_num"] = obj.block_num;
        vo["trx_id"]    = obj.trx_id;

        app().get_plugin<http_plugin>().set_deferred_response(id, 200, fc::json::to_string(vo));
    }
    catch(const chain::evt_link_existed_exception&) {
        // cannot find now, put into map
        add_and_schedule(link_id, id);
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
evt_link_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("evt-link-timeout", bpo::value<uint32_t>()->default_value(5000), "Max time waitting for the deferred request.")
    ;
}

void
evt_link_plugin::plugin_initialize(const variables_map& options) {
    my_ = std::make_shared<evt_link_plugin_impl>(app().get_plugin<chain_plugin>().chain());
    my_->timeout_ = options.at("evt-link-timeout").as<uint32_t>();
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
evt_link_plugin::plugin_shutdown() {
    my_->accepted_block_connection_.reset();
    my_.reset();
}

}  // namespace evt
