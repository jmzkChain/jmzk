/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements newfungible, updfungible, issuefungible, transferft, destroyft and jmzk2pjmzks actions
 */

jmzk_ACTION_IMPL_BEGIN(newfungible) {
    using namespace internal;

    auto nfact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(nfact.sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(!nfact.name.empty(), fungible_name_exception, "FT name cannot be empty");
        jmzk_ASSERT(!nfact.sym_name.empty(), fungible_symbol_exception, "FT symbol name cannot be empty");
        jmzk_ASSERT(nfact.sym.id() > 0, fungible_symbol_exception, "FT symbol id should be larger than zero");
        jmzk_ASSERT(nfact.total_supply.sym() == nfact.sym, fungible_symbol_exception, "Symbols in `total_supply` and `sym` are not match.");
        jmzk_ASSERT(nfact.total_supply.amount() > 0, fungible_supply_exception, "Supply cannot be zero");
        jmzk_ASSERT(nfact.total_supply.amount() <= asset::max_amount, fungible_supply_exception, "Supply exceeds the maximum allowed.");

        DECLARE_TOKEN_DB()

        jmzk_ASSERT(!tokendb.exists_token(token_type::fungible, std::nullopt, nfact.sym.id()), fungible_duplicate_exception,
            "FT with symbol id: ${s} is already existed", ("s",nfact.sym.id()));

        jmzk_ASSERT(nfact.issue.name == "issue", permission_type_exception,
            "Name ${name} does not match with the name of issue permission.", ("name",nfact.issue.name));
        jmzk_ASSERT(nfact.issue.threshold > 0 && validate(nfact.issue), permission_type_exception,
            "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        jmzk_ASSERT(nfact.manage.name == "manage", permission_type_exception,
            "Name ${name} does not match with the name of manage permission.", ("name",nfact.manage.name));
        jmzk_ASSERT(validate(nfact.manage), permission_type_exception,
            "Manage permission is not valid, which may be caused by duplicated keys.");
        if constexpr(jmzk_ACTION_VER() > 1) {
            jmzk_ASSERT(nfact.transfer.name == "transfer", permission_type_exception,
                "Name ${name} does not match with the name of transfer permission.", ("name",nfact.transfer.name));
            jmzk_ASSERT(validate(nfact.transfer), permission_type_exception,
                "Transfer permission is not valid, which may be caused by duplicated keys.");
        }

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nfact.issue, false);
        pchecker(nfact.manage, false);
        if constexpr(jmzk_ACTION_VER() > 1) {
            pchecker(nfact.transfer, true /* allowed_owner */);
        }

        auto fungible = fungible_def();
        
        fungible.name        = nfact.name;
        fungible.sym_name    = nfact.sym_name;
        fungible.sym         = nfact.sym;
        fungible.creator     = nfact.creator;
        // NOTICE: we should use pending_block_time() below
        // but for historical mistakes, we use head_block_time()
        fungible.create_time = context.control.head_block_time();
        fungible.issue       = std::move(nfact.issue);
        fungible.manage      = std::move(nfact.manage);
        if constexpr(jmzk_ACTION_VER() > 1) {
            fungible.transfer = std::move(nfact.transfer);
        }
        else {
            // set with default transfer(owner only)
            fungible.transfer = permission_def {
                .name        = N(transfer),
                .threshold   = 1,
                .authorizers = { authorizer_weight(authorizer_ref(), 1) }
            };
            fungible.metas.emplace_back(get_metakey<reserved_meta_key::disable_set_transfer>(fungible_metas), "true", authorizer_ref(nfact.creator));
        }
        fungible.total_supply = nfact.total_supply;

        ADD_DB_TOKEN(token_type::fungible, fungible);

        auto addr = get_fungible_address(fungible.sym);
        auto prop = MAKE_PROPERTY(fungible.total_supply.amount(), fungible.sym);
        PUT_DB_ASSET(addr, prop);

        context.add_new_ft_holder(ft_holder { .addr = addr, .sym_id = nfact.sym.id() }); 
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(updfungible) {
    using namespace internal;

    auto ufact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(ufact.sym_id)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto fungible = make_empty_cache_ptr<fungible_def>();
        READ_DB_TOKEN(token_type::fungible, std::nullopt, ufact.sym_id, fungible, unknown_fungible_exception,
            "Cannot find FT with sym id: {}", ufact.sym_id);

        auto pchecker = make_permission_checker(tokendb);
        if(ufact.issue.has_value()) {
            jmzk_ASSERT(ufact.issue->name == "issue", permission_type_exception,
                "Name ${name} does not match with the name of issue permission.", ("name",ufact.issue->name));
            jmzk_ASSERT(validate(*ufact.issue), permission_type_exception,
                "Issue permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.issue, false);

            fungible->issue = std::move(*ufact.issue);
        }
        if constexpr(jmzk_ACTION_VER() > 1) {
            if(ufact.transfer.has_value()) {
                auto dt = get_metavalue(*fungible, get_metakey<reserved_meta_key::disable_set_transfer>(fungible_metas));
                if(dt.has_value() && *dt == "true") {
                    jmzk_THROW(fungible_cannot_update_exception, "Transfer permission of this FT cannot be updated");
                }

                jmzk_ASSERT(ufact.transfer->name == "transfer", permission_type_exception,
                    "Name ${name} does not match with the name of transfer permission.", ("name",ufact.transfer->name));
                jmzk_ASSERT(validate(*ufact.transfer), permission_type_exception,
                    "Transfer permission is not valid, which may be caused by duplicated keys.");
                pchecker(*ufact.transfer, true /* allowed_owner */);

                fungible->transfer = std::move(*ufact.transfer);
            }
        }
        if(ufact.manage.has_value()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            jmzk_ASSERT(ufact.manage->name == "manage", permission_type_exception,
                "Name ${name} does not match with the name of manage permission.", ("name",ufact.manage->name));
            jmzk_ASSERT(validate(*ufact.manage), permission_type_exception,
                "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.manage, false);

            fungible->manage = std::move(*ufact.manage);
        }

        UPD_DB_TOKEN(token_type::fungible, *fungible);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(issuefungible) {
    using namespace internal;

    auto& ifact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = ifact.number.sym();
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        check_address_reserved(ifact.address);

        DECLARE_TOKEN_DB()
        jmzk_ASSERT(tokendb.exists_token(token_type::fungible, std::nullopt, sym.id()), fungible_duplicate_exception,
            "{sym} FT doesn't exist", ("sym",sym));

        auto addr = get_fungible_address(sym);
        jmzk_ASSERT(addr != ifact.address, fungible_address_exception, "From and to are the same address");

        try {
            transfer_fungible(context, addr, ifact.address, ifact.number, N(issuefungible), false /* pay charge */);
        }
        catch(balance_exception&) {
            jmzk_THROW2(fungible_supply_exception, "Exceeds total supply of fungible with sym id: {}.", sym.id());
        }
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(transferft) {
    using namespace internal;

    auto& tfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = tfact.number.sym();
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(tfact.from != tfact.to, fungible_address_exception, "From and to are the same address");
        jmzk_ASSERT(sym != pjmzk_sym(), asset_symbol_exception, "Pinned jmzk cannot be transfered");
        check_address_reserved(tfact.to);

        transfer_fungible(context, tfact.from, tfact.to, tfact.number, N(transferft));
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(recycleft) {
    using namespace internal;

    auto& rfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = rfact.number.sym();
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(sym != pjmzk_sym(), asset_symbol_exception, "Pinned jmzk cannot be recycled");

        auto addr = get_fungible_address(sym);
        transfer_fungible(context, rfact.address, addr, rfact.number, N(recycleft), false /* pay bonus */);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(destroyft) {
    using namespace internal;

    auto& dfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = dfact.number.sym();
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(sym != pjmzk_sym(), fungible_symbol_exception, "Pinned jmzk cannot be destroyed");

        auto addr = address();
        transfer_fungible(context, dfact.address, addr, dfact.number, N(destroyft), false /* pay bonus */);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(jmzk2pjmzk) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();

    try {
        jmzk_ASSERT(epact.number.sym() == jmzk_sym(), asset_symbol_exception, "Only jmzk tokens can be converted to Pinned jmzk tokens");
        jmzk_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(jmzk_sym().id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        check_address_reserved(epact.to);

        transfer_fungible(context, epact.from, epact.to, asset(epact.number.amount(), pjmzk_sym()), N(jmzk2pjmzk), false /* pay bonus */);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts
