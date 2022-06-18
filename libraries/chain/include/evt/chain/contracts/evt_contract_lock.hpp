/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements newlock, aprvlock and tryunlock actions
 */

jmzk_ACTION_IMPL_BEGIN(newlock) {
    using namespace internal;

    auto nlact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.lock), nlact.name), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()
        jmzk_ASSERT(!tokendb.exists_token(token_type::lock, std::nullopt, nlact.name), lock_duplicate_exception,
            "Lock assets with same name: ${n} is already existed", ("n",nlact.name));

        auto now = context.control.pending_block_time();
        jmzk_ASSERT(nlact.unlock_time > now, lock_unlock_time_exception,
            "Now is ahead of unlock time, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));
        jmzk_ASSERT(nlact.deadline > now && nlact.deadline > nlact.unlock_time, lock_unlock_time_exception,
            "Now is ahead of unlock time or deadline, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));

        // check condition
        switch(nlact.condition.type()) {
        case lock_type::cond_keys: {
            auto& lck = nlact.condition.template get<lock_condkeys>();
            jmzk_ASSERT(lck.threshold > 0 && lck.cond_keys.size() >= lck.threshold, lock_condition_exception,
                "Conditional keys for lock should not be empty or threshold should not be zero");
        }    
        }  // switch
        
        // check succeed and fiil addresses(shouldn't be reserved)
        for(auto& addr : nlact.succeed) {
            check_address_reserved(addr);
        }
        for(auto& addr : nlact.failed) {
            check_address_reserved(addr);
        }

        // check assets(need to has authority)
        jmzk_ASSERT(nlact.assets.size() > 0, lock_assets_exception, "Assets for lock should not be empty");

        auto has_fungible = false;
        auto keys         = context.trx_context.trx_meta->recover_keys(context.control.get_chain_id());
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.template get<locknft_def>();
                jmzk_ASSERT(tokens.names.size() > 0, lock_assets_exception, "NFT assets should be provided.");

                auto tt   = transfer();
                tt.domain = tokens.domain;
                for(auto& tn : tokens.names) {
                    tt.name = tn;

                    auto ttact = action(tt.domain, tt.name, tt);
                    context.control.check_authorization(keys, ttact);
                }
                break;
            }
            case asset_type::fungible: {
                auto& fungible = la.template get<lockft_def>();

                jmzk_ASSERT(fungible.amount.sym().id() != Pjmzk_SYM_ID, lock_assets_exception, "Pinned jmzk cannot be used to be locked.");
                has_fungible = true;

                auto tf   = transferft();
                tf.from   = fungible.from;
                tf.number = fungible.amount;

                auto tfact = action(N128(.fungible), name128::from_number(fungible.amount.sym().id()), tf);
                context.control.check_authorization(keys, tfact);
                break;
            }
            }  // switch
        }

        // check succeed and fail addresses(size should be match)
        if(has_fungible) {
            // because fungible assets cannot be transfer to multiple addresses.
            jmzk_ASSERT(nlact.succeed.size() == 1, lock_address_exception,
                "Size of address for succeed situation should be only one when there's fungible assets needs to lock");
            jmzk_ASSERT(nlact.failed.size() == 1, lock_address_exception,
                "Size of address for failed situation should be only one when there's fungible assets needs to lock");
        }
        else {
            jmzk_ASSERT(nlact.succeed.size() > 0, lock_address_exception, "Size of address for succeed situation should not be empty");
            jmzk_ASSERT(nlact.failed.size() > 0, lock_address_exception, "Size of address for failed situation should not be empty");
        }

        // transfer assets to lock address
        auto laddr = address(N(.lock), N128(nlact.name), 0);
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.template get<locknft_def>();

                for(auto& tn : tokens.names) {
                    auto token = make_empty_cache_ptr<token_def>();
                    READ_DB_TOKEN(token_type::token, tokens.domain, tn, token, unknown_token_exception,
                        "Cannot find token: {} in {}", tn, tokens.domain);
                    token->owner = { laddr };

                    UPD_DB_TOKEN(token_type::token, *token);
                }
                break;
            }
            case asset_type::fungible: {
                auto& fungible = la.template get<lockft_def>();
                // the transfer below doesn't need to pay for the passive bonus
                // will pay in the unlock time
                transfer_fungible(context, fungible.from, laddr, fungible.amount, N(newlock), false /* pay bonus */);
                break;
            }
            }  // switch
        }

        // add locl proposal to token database
        auto lock        = lock_def();
        lock.name        = nlact.name;
        lock.proposer    = nlact.proposer;
        lock.status      = lock_status::proposed;
        lock.unlock_time = nlact.unlock_time;
        lock.deadline    = nlact.deadline;
        lock.assets      = std::move(nlact.assets);
        lock.condition   = std::move(nlact.condition);
        lock.succeed     = std::move(nlact.succeed);
        lock.failed      = std::move(nlact.failed);

        ADD_DB_TOKEN(token_type::lock, lock);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(aprvlock) {
    using namespace internal;

    auto& alact = context.act.data_as<add_clr_t<ACT>>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.lock), alact.name), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto lock = make_empty_cache_ptr<lock_def>();
        READ_DB_TOKEN(token_type::lock, std::nullopt, alact.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", alact.name);

        auto now = context.control.pending_block_time();
        jmzk_ASSERT(lock->unlock_time > now, lock_expired_exception,
            "Now is ahead of unlock time, cannot approve anymore, unlock time is ${u}, now is ${n}", ("u",lock->unlock_time)("n",now));

        switch(lock->condition.type()) {
        case lock_type::cond_keys: {
            jmzk_ASSERT(alact.data.type() == lock_aprv_type::cond_key, lock_aprv_data_exception,
                "Type of approve data is not conditional key");
            auto& lck = lock->condition.template get<lock_condkeys>();

            jmzk_ASSERT(std::find(lck.cond_keys.cbegin(), lck.cond_keys.cend(), alact.approver) != lck.cond_keys.cend(),
                lock_aprv_data_exception, "Approver is not valid");
            jmzk_ASSERT(lock->signed_keys.find(alact.approver) == lock->signed_keys.cend(), lock_duplicate_key_exception,
                "Approver is already signed this lock assets proposal");
        }
        }  // switch

        lock->signed_keys.emplace(alact.approver);
        UPD_DB_TOKEN(token_type::lock, *lock);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(tryunlock) {
    using namespace internal;

    auto& tuact = context.act.data_as<add_clr_t<ACT>>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.lock), tuact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto lock = make_empty_cache_ptr<lock_def>();
        READ_DB_TOKEN(token_type::lock, std::nullopt, tuact.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", tuact.name);

        auto now = context.control.pending_block_time();
        jmzk_ASSERT(lock->unlock_time < now, lock_not_reach_unlock_time,
            "Not reach unlock time, cannot unlock, unlock time is ${u}, now is ${n}", ("u",lock->unlock_time)("n",now));

        // std::add_pointer_t<decltype(lock.succeed)> pkeys = nullptr;
        fc::small_vector_base<address>* pkeys = nullptr;
        switch(lock->condition.type()) {
        case lock_type::cond_keys: {
            auto& lck = lock->condition.template get<lock_condkeys>();
            if(lock->signed_keys.size() >= lck.threshold) {
                pkeys        = &lock->succeed;
                lock->status = lock_status::succeed;
            }
            break;
        }
        }  // switch

        if(pkeys == nullptr) {
            // not succeed
            jmzk_ASSERT(lock->deadline < now, lock_not_reach_deadline,
                "Not reach deadline and conditions are not satisfied, proposal is still avaiable.");
            pkeys        = &lock->failed;
            lock->status = lock_status::failed;
        }

        auto laddr = address(N(.lock), N128(nlact.name), 0);
        for(auto& la : lock->assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.template get<locknft_def>();

                for(auto& tn : tokens.names) {
                    auto token = make_empty_cache_ptr<token_def>();
                    READ_DB_TOKEN(token_type::token, tokens.domain, tn, token, unknown_token_exception,
                        "Cannot find token: {} in {}", tn, tokens.domain);
                    token->owner = *pkeys;
                    UPD_DB_TOKEN(token_type::token, *token);
                }
                break;
            }
            case asset_type::fungible: {
                FC_ASSERT(pkeys->size() == 1);

                auto& fungible = la.template get<lockft_def>();
                auto& toaddr   = (*pkeys)[0];
                
                transfer_fungible(context, laddr, toaddr, fungible.amount, N(tryunlock));
            }
            }  // switch
        }

        UPD_DB_TOKEN(token_type::lock, *lock);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts
