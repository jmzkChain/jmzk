/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements everipass and everipay actions
 */

jmzk_ACTION_IMPL_BEGIN(everipass) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
    try {

        DECLARE_TOKEN_DB()

        auto& link  = epact.link;
        auto  flags = link.get_header();

        jmzk_ASSERT(flags & jmzk_link::version1, jmzk_link_version_exception, "Unexpected jmzkLink version, current supported version is Versoin 1");
        jmzk_ASSERT(flags & jmzk_link::everiPass, jmzk_link_type_exception, "Not a everiPass link");

        auto& d = *link.get_segment(jmzk_link::domain).strv;
        auto& t = *link.get_segment(jmzk_link::token).strv;

        jmzk_ASSERT(context.has_authorized(name128(d), name128(t)), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(jmzk_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.jmzk_link_expired_secs) {
                jmzk_THROW(jmzk_link_expiration_exception, "jmzk-Link is expired, now: ${n}, timestamp: ${t}",
                    ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        auto keys  = link.restore_keys();
        auto token = make_empty_cache_ptr<token_def>();
        READ_DB_TOKEN(token_type::token, d, t, token, unknown_token_exception, "Cannot find token: {} in {}", t, d);

        jmzk_ASSERT(!check_token_destroy(*token), token_destroyed_exception, "Destroyed token cannot be destroyed during everiPass.");
        jmzk_ASSERT(!check_token_locked(*token), token_locked_exception, "Locked token cannot be destroyed during everiPass.");

        if(flags & jmzk_link::destroy) {
            auto dt   = destroytoken();
            dt.domain = d;
            dt.name   = t;

            auto dtact = action(dt.domain, dt.name, dt);
            context.control.check_authorization(keys, dtact);

            token->owner = address_list{ address() };
            UPD_DB_TOKEN(token_type::token, *token);
        }
        else {
            // only check owner
            jmzk_ASSERT(token->owner.size() == keys.size(), everipass_exception, "Owner size and keys size don't match");
            for(auto& o : token->owner) {
                jmzk_ASSERT(keys.find(o.get_public_key()) != keys.end(), everipass_exception, "Owner didn't sign");
            }
        }
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(everipay) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
    try {
        check_address_reserved(epact.payee, false /* throw_on_reserved */);

        DECLARE_TOKEN_DB()

        auto& link  = epact.link;
        auto  flags = link.get_header();

        jmzk_ASSERT(flags & jmzk_link::version1, jmzk_link_version_exception,
            "jmzk-Link version is not expected, current supported version is Versoin-1");
        jmzk_ASSERT(flags & jmzk_link::everiPay, jmzk_link_type_exception, "Not a everiPay link");

        auto& lsym_id = *link.get_segment(jmzk_link::symbol_id).intv;
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(lsym_id)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(jmzk_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.jmzk_link_expired_secs) {
                jmzk_THROW(jmzk_link_expiration_exception,"jmzk-Link is expired, now: ${n}, timestamp: ${t}",
                    ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        // check symbol
        auto sym = epact.number.sym();
        jmzk_ASSERT2(lsym_id == sym.id(), everipay_exception,
            "Id of symbols don't match, provided: {}, expected: {}", lsym_id, sym.id());
        jmzk_ASSERT(lsym_id != Pjmzk_SYM_ID, everipay_exception, "Pinned jmzk cannot be paid.");

        // check fixed amount
        auto fixed_amount = int64_t(0);
        if(link.has_segment(jmzk_link::fixed_amount)) {
            fixed_amount = *link.get_segment(jmzk_link::fixed_amount).intv;
            jmzk_ASSERT2(!link.has_segment(jmzk_link::fixed_amount_str), jmzk_link_exception, "Cannot use fixed_amount_str while using fixed_amount segment");
        }
        else if(link.has_segment(jmzk_link::fixed_amount_str)) {
            fixed_amount = std::stoul(*link.get_segment(jmzk_link::fixed_amount_str).strv);
        }

        if(fixed_amount > 0) {
            jmzk_ASSERT2(epact.number.amount() == fixed_amount, everipay_exception, "Paid amount should be fixed at: {:n}, actual: {:n}", fixed_amount, epact.number.amount());
        }

        // check max pay
        auto max_pay = int64_t(0);
        if(link.has_segment(jmzk_link::max_pay)) {
            max_pay = *link.get_segment(jmzk_link::max_pay).intv;
            jmzk_ASSERT2(!link.has_segment(jmzk_link::max_pay_str), jmzk_link_exception, "Cannot use max_pay_str while using max_pay segment");
        }
        else {
            max_pay = std::stoul(*link.get_segment(jmzk_link::max_pay_str).strv);
        }
        jmzk_ASSERT2(epact.number.amount() <= max_pay, everipay_exception,
            "Exceed max allowd paid amount: {:n}, actual: {:n}", max_pay, epact.number.amount());

        // check link id
        auto link_id = link.get_link_id();
        jmzk_ASSERT(!tokendb.exists_token(token_type::jmzklink, std::nullopt, link_id), jmzk_link_dupe_exception,
            "Duplicate jmzk-Link ${id}", ("id", fc::to_hex((char*)&link_id, sizeof(link_id))));

        auto link_obj = jmzk_link_object {
            .link_id   = link_id,
            .block_num = context.control.pending_block_state()->block->block_num(),
            .trx_id    = context.trx_context.trx_meta->id
        };
        ADD_DB_TOKEN(token_type::jmzklink, link_obj);

        // check signature
        auto keys = link.restore_keys();
        jmzk_ASSERT(keys.size() == 1, everipay_exception, "There're more than one signature on everiPay link, which is invalid");
        
        // check payee
        auto payer = address(*keys.begin());
        jmzk_ASSERT(payer != epact.payee, everipay_exception, "Payer and payee shouldn't be the same one");

        // do transfer
        transfer_fungible(context, payer, epact.payee, epact.number, N(everipay));
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts
