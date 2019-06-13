/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

/**
 * Implements newfungible, updfungible, issuefungible, transferft, destroyft and evt2pevts actions
 */

EVT_ACTION_IMPL_BEGIN(newfungible) {
    using namespace internal;

    auto nfact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(nfact.sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(!nfact.name.empty(), fungible_name_exception, "FT name cannot be empty");
        EVT_ASSERT(!nfact.sym_name.empty(), fungible_symbol_exception, "FT symbol name cannot be empty");
        EVT_ASSERT(nfact.sym.id() > 0, fungible_symbol_exception, "FT symbol id should be larger than zero");
        EVT_ASSERT(nfact.total_supply.sym() == nfact.sym, fungible_symbol_exception, "Symbols in `total_supply` and `sym` are not match.");
        EVT_ASSERT(nfact.total_supply.amount() > 0, fungible_supply_exception, "Supply cannot be zero");
        EVT_ASSERT(nfact.total_supply.amount() <= asset::max_amount, fungible_supply_exception, "Supply exceeds the maximum allowed.");

        DECLARE_TOKEN_DB()

        EVT_ASSERT(!tokendb.exists_token(token_type::fungible, std::nullopt, nfact.sym.id()), fungible_duplicate_exception,
            "FT with symbol id: ${s} is already existed", ("s",nfact.sym.id()));

        EVT_ASSERT(nfact.issue.name == "issue", permission_type_exception,
            "Name ${name} does not match with the name of issue permission.", ("name",nfact.issue.name));
        EVT_ASSERT(nfact.issue.threshold > 0 && validate(nfact.issue), permission_type_exception,
            "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(nfact.manage.name == "manage", permission_type_exception,
            "Name ${name} does not match with the name of manage permission.", ("name",nfact.manage.name));
        EVT_ASSERT(validate(nfact.manage), permission_type_exception,
            "Manage permission is not valid, which may be caused by duplicated keys.");
        if constexpr(EVT_ACTION_VER() > 1) {
            EVT_ASSERT(nfact.transfer.name == "transfer", permission_type_exception,
                "Name ${name} does not match with the name of transfer permission.", ("name",nfact.transfer.name));
            EVT_ASSERT(validate(nfact.transfer), permission_type_exception,
                "Transfer permission is not valid, which may be caused by duplicated keys.");
        }

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nfact.issue, false);
        pchecker(nfact.manage, false);
        if constexpr(EVT_ACTION_VER() > 1) {
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
        if constexpr(EVT_ACTION_VER() > 1) {
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
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updfungible) {
    using namespace internal;

    auto ufact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(ufact.sym_id)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto fungible = make_empty_cache_ptr<fungible_def>();
        READ_DB_TOKEN(token_type::fungible, std::nullopt, ufact.sym_id, fungible, unknown_fungible_exception,
            "Cannot find FT with sym id: {}", ufact.sym_id);

        auto pchecker = make_permission_checker(tokendb);
        if(ufact.issue.has_value()) {
            EVT_ASSERT(ufact.issue->name == "issue", permission_type_exception,
                "Name ${name} does not match with the name of issue permission.", ("name",ufact.issue->name));
            EVT_ASSERT(validate(*ufact.issue), permission_type_exception,
                "Issue permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.issue, false);

            fungible->issue = std::move(*ufact.issue);
        }
        if constexpr(EVT_ACTION_VER() > 1) {
            if(ufact.transfer.has_value()) {
                auto dt = get_metavalue(*fungible, get_metakey<reserved_meta_key::disable_set_transfer>(fungible_metas));
                if(dt.has_value() && *dt == "true") {
                    EVT_THROW(fungible_cannot_update_exception, "Transfer permission of this FT cannot be updated");
                }

                EVT_ASSERT(ufact.transfer->name == "transfer", permission_type_exception,
                    "Name ${name} does not match with the name of transfer permission.", ("name",ufact.transfer->name));
                EVT_ASSERT(validate(*ufact.transfer), permission_type_exception,
                    "Transfer permission is not valid, which may be caused by duplicated keys.");
                pchecker(*ufact.transfer, true /* allowed_owner */);

                fungible->transfer = std::move(*ufact.transfer);
            }
        }
        if(ufact.manage.has_value()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(ufact.manage->name == "manage", permission_type_exception,
                "Name ${name} does not match with the name of manage permission.", ("name",ufact.manage->name));
            EVT_ASSERT(validate(*ufact.manage), permission_type_exception,
                "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.manage, false);

            fungible->manage = std::move(*ufact.manage);
        }

        UPD_DB_TOKEN(token_type::fungible, *fungible);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(issuefungible) {
    using namespace internal;

    auto& ifact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = ifact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        check_address_reserved(ifact.address);

        DECLARE_TOKEN_DB()
        EVT_ASSERT(tokendb.exists_token(token_type::fungible, std::nullopt, sym.id()), fungible_duplicate_exception,
            "{sym} FT doesn't exist", ("sym",sym));

        auto addr = get_fungible_address(sym);
        EVT_ASSERT(addr != ifact.address, fungible_address_exception, "From and to are the same address");

        try {
            transfer_fungible(context, addr, ifact.address, ifact.number, N(issuefungible), false /* pay charge */);
        }
        catch(balance_exception&) {
            EVT_THROW2(fungible_supply_exception, "Exceeds total supply of fungible with sym id: {}.", sym.id());
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(transferft) {
    using namespace internal;

    auto& tfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = tfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(tfact.from != tfact.to, fungible_address_exception, "From and to are the same address");
        EVT_ASSERT(sym != pevt_sym(), asset_symbol_exception, "Pinned EVT cannot be transfered");
        check_address_reserved(tfact.to);

        transfer_fungible(context, tfact.from, tfact.to, tfact.number, N(transferft));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(recycleft) {
    using namespace internal;

    auto& rfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = rfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(sym != pevt_sym(), asset_symbol_exception, "Pinned EVT cannot be recycled");

        auto addr = get_fungible_address(sym);
        transfer_fungible(context, rfact.address, addr, rfact.number, N(recycleft), false /* pay bonus */);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(destroyft) {
    using namespace internal;

    auto& dfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = dfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be destroyed");

        auto addr = address();
        transfer_fungible(context, dfact.address, addr, dfact.number, N(destroyft), false /* pay bonus */);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(evt2pevt) {
    using namespace internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();

    try {
        EVT_ASSERT(epact.number.sym() == evt_sym(), asset_symbol_exception, "Only EVT tokens can be converted to Pinned EVT tokens");
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(evt_sym().id())), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        check_address_reserved(epact.to);

        transfer_fungible(context, epact.from, epact.to, asset(epact.number.amount(), pevt_sym()), N(evt2pevt), false /* pay bonus */);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
