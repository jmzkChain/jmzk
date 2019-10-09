/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

/**
 * Implements everipass and everipay actions
 */

EVT_ACTION_IMPL_BEGIN(everipass) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
    try {

        DECLARE_TOKEN_DB()

        auto& link  = epact.link;
        auto  flags = link.get_header();

        EVT_ASSERT(flags & evt_link::version1, evt_link_version_exception, "Unexpected EvtLink version, current supported version is Versoin 1");
        EVT_ASSERT(flags & evt_link::everiPass, evt_link_type_exception, "Not a everiPass link");

        auto& d = *link.get_segment(evt_link::domain).strv;
        auto& t = *link.get_segment(evt_link::token).strv;

        EVT_ASSERT(context.has_authorized(name128(d), name128(t)), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(evt_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.evt_link_expired_secs) {
                EVT_THROW(evt_link_expiration_exception, "EVT-Link is expired, now: ${n}, timestamp: ${t}",
                    ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        auto keys  = link.restore_keys();
        auto token = make_empty_cache_ptr<token_def>();
        READ_DB_TOKEN(token_type::token, d, t, token, unknown_token_exception, "Cannot find token: {} in {}", t, d);

        EVT_ASSERT(!check_token_destroy(*token), token_destroyed_exception, "Destroyed token cannot be destroyed during everiPass.");
        EVT_ASSERT(!check_token_locked(*token), token_locked_exception, "Locked token cannot be destroyed during everiPass.");

        if(flags & evt_link::destroy) {
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
            EVT_ASSERT(token->owner.size() == keys.size(), everipass_exception, "Owner size and keys size don't match");
            for(auto& o : token->owner) {
                EVT_ASSERT(keys.find(o.get_public_key()) != keys.end(), everipass_exception, "Owner didn't sign");
            }
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(everipay) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
    try {
        check_address_reserved(epact.payee, false /* throw_on_reserved */);

        DECLARE_TOKEN_DB()

        auto& link  = epact.link;
        auto  flags = link.get_header();

        EVT_ASSERT(flags & evt_link::version1, evt_link_version_exception,
            "EVT-Link version is not expected, current supported version is Versoin-1");
        EVT_ASSERT(flags & evt_link::everiPay, evt_link_type_exception, "Not a everiPay link");

        auto& lsym_id = *link.get_segment(evt_link::symbol_id).intv;
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(lsym_id)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(evt_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.evt_link_expired_secs) {
                EVT_THROW(evt_link_expiration_exception,"EVT-Link is expired, now: ${n}, timestamp: ${t}",
                    ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        // check symbol
        auto sym = epact.number.sym();
        EVT_ASSERT2(lsym_id == sym.id(), everipay_exception,
            "Id of symbols don't match, provided: {}, expected: {}", lsym_id, sym.id());
        EVT_ASSERT(lsym_id != PEVT_SYM_ID, everipay_exception, "Pinned EVT cannot be paid.");

        // check fixed amount
        auto fixed_amount = int64_t(0);
        if(link.has_segment(evt_link::fixed_amount)) {
            fixed_amount = *link.get_segment(evt_link::fixed_amount).intv;
            EVT_ASSERT2(!link.has_segment(evt_link::fixed_amount_str), evt_link_exception, "Cannot use fixed_amount_str while using fixed_amount segment");
        }
        else if(link.has_segment(evt_link::fixed_amount_str)) {
            fixed_amount = std::stoul(*link.get_segment(evt_link::fixed_amount_str).strv);
        }

        if(fixed_amount > 0) {
            EVT_ASSERT2(epact.number.amount() == fixed_amount, everipay_exception, "Paid amount should be fixed at: {:n}, actual: {:n}", fixed_amount, epact.number.amount());
        }

        // check max pay
        auto max_pay = int64_t(0);
        if(link.has_segment(evt_link::max_pay)) {
            max_pay = *link.get_segment(evt_link::max_pay).intv;
            EVT_ASSERT2(!link.has_segment(evt_link::max_pay_str), evt_link_exception, "Cannot use max_pay_str while using max_pay segment");
        }
        else {
            max_pay = std::stoul(*link.get_segment(evt_link::max_pay_str).strv);
        }
        EVT_ASSERT2(epact.number.amount() <= max_pay, everipay_exception,
            "Exceed max allowd paid amount: {:n}, actual: {:n}", max_pay, epact.number.amount());

        // check link id
        auto link_id = link.get_link_id();
        EVT_ASSERT(!tokendb.exists_token(token_type::evtlink, std::nullopt, link_id), evt_link_dupe_exception,
            "Duplicate EVT-Link ${id}", ("id", fc::to_hex((char*)&link_id, sizeof(link_id))));

        auto link_obj = evt_link_object {
            .link_id   = link_id,
            .block_num = context.control.pending_block_state()->block->block_num(),
            .trx_id    = context.trx_context.trx_meta->id
        };
        ADD_DB_TOKEN(token_type::evtlink, link_obj);

        // check signature
        auto keys = link.restore_keys();
        EVT_ASSERT(keys.size() == 1, everipay_exception, "There're more than one signature on everiPay link, which is invalid");
        
        // check payee
        auto payer = address(*keys.begin());
        EVT_ASSERT(payer != epact.payee, everipay_exception, "Payer and payee shouldn't be the same one");

        // do transfer
        transfer_fungible(context, payer, epact.payee, epact.number, N(everipay));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
