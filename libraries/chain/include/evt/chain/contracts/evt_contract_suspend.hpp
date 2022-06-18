/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements newsuspend, aprvsuspend, cancelsuspend and execsuspend
 */

jmzk_ACTION_IMPL_BEGIN(newsuspend) {
    using namespace internal;

    auto nsact = context.act.data_as<newsuspend>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.suspend), nsact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        auto now = context.control.pending_block_time();
        jmzk_ASSERT(nsact.trx.expiration > now, suspend_expired_tx_exception,
            "Expiration of suspend transaction is ahead of now, expired is ${expir}, now is ${now}", ("expir",nsact.trx.expiration)("now",now));

        context.control.validate_tapos(nsact.trx);

        check_name_reserved(nsact.name);
        for(auto& act : nsact.trx.actions) {
            jmzk_ASSERT(act.domain != N128(suspend), suspend_invalid_action_exception, "Actions in 'suspend' domain are not allowd deferred-signning");
            jmzk_ASSERT(act.name != N(everipay), suspend_invalid_action_exception, "everiPay action is not allowd deferred-signning");
            jmzk_ASSERT(act.name != N(everipass), suspend_invalid_action_exception, "everiPass action is not allowd deferred-signning");
        }

        DECLARE_TOKEN_DB()
        jmzk_ASSERT(!tokendb.exists_token(token_type::suspend, std::nullopt, nsact.name), suspend_duplicate_exception,
            "Suspend ${name} already exists.", ("name",nsact.name));

        auto suspend     = suspend_def();
        suspend.name     = nsact.name;
        suspend.proposer = nsact.proposer;
        suspend.status   = suspend_status::proposed;
        suspend.trx      = std::move(nsact.trx);

        ADD_DB_TOKEN(token_type::suspend, suspend);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(aprvsuspend) {
    using namespace internal;

    auto& aeact = context.act.data_as<add_clr_t<ACT>>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.suspend), aeact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto suspend = make_empty_cache_ptr<suspend_def>();
        READ_DB_TOKEN(token_type::suspend, std::nullopt, aeact.name, suspend, unknown_suspend_exception,
            "Cannot find suspend proposal: {}", aeact.name);

        jmzk_ASSERT(suspend->status == suspend_status::proposed, suspend_status_exception,
            "Suspend transaction is not in 'proposed' status.");

        auto signed_keys = suspend->trx.get_signature_keys(aeact.signatures, context.control.get_chain_id());
        auto required_keys = context.control.get_suspend_required_keys(suspend->trx, signed_keys);
        jmzk_ASSERT(signed_keys == required_keys, suspend_not_required_keys_exception,
            "Provided keys are not required in this suspend transaction");
       
        for(auto it = signed_keys.cbegin(); it != signed_keys.cend(); it++) {
            jmzk_ASSERT(suspend->signed_keys.find(*it) == suspend->signed_keys.end(), suspend_duplicate_key_exception,
                "Public key ${key} is already signed this suspend transaction", ("key",*it)); 
        }

        suspend->signed_keys.merge(signed_keys);
        suspend->signatures.insert(suspend->signatures.cend(), aeact.signatures.cbegin(), aeact.signatures.cend());
        
        UPD_DB_TOKEN(token_type::suspend, *suspend);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(cancelsuspend) {
    using namespace internal;

    auto& csact = context.act.data_as<add_clr_t<ACT>>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.suspend), csact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto suspend = make_empty_cache_ptr<suspend_def>();
        READ_DB_TOKEN(token_type::suspend, std::nullopt, csact.name, suspend, unknown_suspend_exception,
            "Cannot find suspend proposal: {}", csact.name);
        
        jmzk_ASSERT(suspend->status == suspend_status::proposed, suspend_status_exception,
            "Suspend transaction is not in 'proposed' status.");
        suspend->status = suspend_status::cancelled;

        UPD_DB_TOKEN(token_type::suspend, *suspend);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(execsuspend) {
    using namespace internal;

    auto& esact = context.act.data_as<add_clr_t<ACT>>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.suspend), esact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto suspend = make_empty_cache_ptr<suspend_def>();
        READ_DB_TOKEN(token_type::suspend, std::nullopt, esact.name, suspend, unknown_suspend_exception,
            "Cannot find suspend proposal: {}", esact.name);

        jmzk_ASSERT(suspend->signed_keys.find(esact.executor) != suspend->signed_keys.end(), suspend_executor_exception,
            "Executor hasn't sign his key on this suspend transaction");

        auto now = context.control.pending_block_time();
        jmzk_ASSERT(suspend->status == suspend_status::proposed, suspend_status_exception,
            "Suspend transaction is not in 'proposed' status.");
        jmzk_ASSERT(suspend->trx.expiration > now, suspend_expired_tx_exception,
            "Suspend transaction is expired at ${expir}, now is ${now}", ("expir",suspend->trx.expiration)("now",now));

        // instead of add signatures to transaction, check authorization and payer here
        context.control.check_authorization(suspend->signed_keys, suspend->trx);
        if(suspend->trx.payer.type() == address::public_key_t) {
            jmzk_ASSERT(suspend->signed_keys.find(suspend->trx.payer.get_public_key()) != suspend->signed_keys.end(), payer_exception,
                "Payer ${pay} needs to sign this suspend transaction", ("pay",suspend->trx.payer));
        }

        auto strx = signed_transaction(suspend->trx, {});
        auto name = (std::string)esact.name;
        strx.transaction_extensions.emplace_back(std::make_pair((uint16_t)transaction_ext::suspend_name, vector<char>(name.cbegin(), name.cend())));

        auto mtrx = std::make_shared<transaction_metadata>(strx);

        auto trace = context.control.push_suspend_transaction(mtrx, fc::time_point::maximum());
        bool transaction_failed = trace && trace->except;
        if(transaction_failed) {
            suspend->status = suspend_status::failed;
            fmt::format_to(context.get_console_buffer(), "{}", trace->except->to_string());
        }
        else {
            suspend->status = suspend_status::executed;
        }
        UPD_DB_TOKEN(token_type::suspend, *suspend);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts
