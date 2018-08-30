
void
producer_plugin_impl::schedule_production_loop() {
    chain::controller& chain = app().get_plugin<chain_plugin>().chain();
    _timer.cancel();
    std::weak_ptr<producer_plugin_impl> weak_this = shared_from_this();

    bool last_block;
    auto result = start_block(last_block);

    if(result == start_block_result::failed) {
        elog("Failed to start a pending block, will try again later");
        _timer.expires_from_now(boost::posix_time::microseconds(config::block_interval_us / 10));

        // we failed to start a block, so try again later?
        _timer.async_wait([weak_this, cid = ++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if(self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
                self->schedule_production_loop();
            }
        });
    }
    else if(result == start_block_result::waiting) {
        if(!_producers.empty() && !production_disabled_by_policy()) {
            fc_dlog(_log, "Waiting till another block is received and scheduling Speculative/Production Change");
            schedule_delayed_production_loop(weak_this, calculate_pending_block_time());
        }
        else {
            fc_dlog(_log, "Waiting till another block is received");
            // nothing to do until more blocks arrive
        }
    }
    else if(_pending_block_mode == pending_block_mode::producing) {
        // we succeeded but block may be exhausted
        static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        if(result == start_block_result::succeeded) {
            // ship this block off no later than its deadline
            EOS_ASSERT(chain.pending_block_state(), missing_pending_block_state, "producing without pending_block_state, start_block succeeded");
            auto deadline = chain.pending_block_time().time_since_epoch().count() + (last_block ? _last_block_time_offset_us : _produce_time_offset_us);
            _timer.expires_at(epoch + boost::posix_time::microseconds(deadline));
            fc_dlog(_log, "Scheduling Block Production on Normal Block #${num} for ${time}", ("num", chain.pending_block_state()->block_num)("time", deadline));
        }
        else {
            EOS_ASSERT(chain.pending_block_state(), missing_pending_block_state, "producing without pending_block_state");
            auto expect_time = chain.pending_block_time() - fc::microseconds(config::block_interval_us);
            // ship this block off up to 1 block time earlier or immediately
            if(fc::time_point::now() >= expect_time) {
                _timer.expires_from_now(boost::posix_time::microseconds(0));
                fc_dlog(_log, "Scheduling Block Production on Exhausted Block #${num} immediately", ("num", chain.pending_block_state()->block_num));
            }
            else {
                _timer.expires_at(epoch + boost::posix_time::microseconds(expect_time.time_since_epoch().count()));
                fc_dlog(_log, "Scheduling Block Production on Exhausted Block #${num} at ${time}", ("num", chain.pending_block_state()->block_num)("time", expect_time));
            }
        }

        _timer.async_wait([&chain, weak_this, cid = ++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if(self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
                // pending_block_state expected, but can't assert inside async_wait
                auto block_num = chain.pending_block_state() ? chain.pending_block_state()->block_num : 0;
                auto res       = self->maybe_produce_block();
                fc_dlog(_log, "Producing Block #${num} returned: ${res}", ("num", block_num)("res", res));
            }
        });
    }
    else if(_pending_block_mode == pending_block_mode::speculating && !_producers.empty() && !production_disabled_by_policy()) {
        fc_dlog(_log, "Specualtive Block Created; Scheduling Speculative/Production Change");
        EOS_ASSERT(chain.pending_block_state(), missing_pending_block_state, "speculating without pending_block_state");
        const auto& pbs = chain.pending_block_state();
        schedule_delayed_production_loop(weak_this, pbs->header.timestamp);
    }
    else {
        fc_dlog(_log, "Speculative Block Created");
    }
}

void
producer_plugin_impl::schedule_delayed_production_loop(const std::weak_ptr<producer_plugin_impl>& weak_this, const block_timestamp_type& current_block_time) {
    // if we have any producers then we should at least set a timer for our next available slot
    optional<fc::time_point> wake_up_time;
    for(const auto& p : _producers) {
        auto next_producer_block_time = calculate_next_block_time(p, current_block_time);
        if(next_producer_block_time) {
            auto producer_wake_up_time = *next_producer_block_time - fc::microseconds(config::block_interval_us);
            if(wake_up_time) {
                // wake up with a full block interval to the deadline
                wake_up_time = std::min<fc::time_point>(*wake_up_time, producer_wake_up_time);
            }
            else {
                wake_up_time = producer_wake_up_time;
            }
        }
    }

    if(wake_up_time) {
        fc_dlog(_log, "Scheduling Speculative/Production Change at ${time}", ("time", wake_up_time));
        static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        _timer.expires_at(epoch + boost::posix_time::microseconds(wake_up_time->time_since_epoch().count()));
        _timer.async_wait([weak_this, cid = ++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if(self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
                self->schedule_production_loop();
            }
        });
    }
    else {
        fc_dlog(_log, "Not Scheduling Speculative/Production, no local producers had valid wake up times");
    }
}
