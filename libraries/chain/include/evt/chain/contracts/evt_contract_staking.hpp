/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

namespace mp = boost::multiprecision;

/**
 * Implements newstakepool, updstakepool, newvalidator, staketkns, unstaketkns and toactivetkns actions
 */

EVT_ACTION_IMPL_BEGIN(newstakepool) {
    using namespace internal;

    auto nsact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(nsact.sym_id)), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        EVT_ASSERT2(nsact.sym_id == EVT_SYM_ID, staking_symbol_exception, "Only EVT is supported to stake currently");
        EVT_ASSERT2(!tokendb.exists_token(token_type::stakepool, std::nullopt, name128::from_number(nsact.sym_id)), stakepool_duplicate_exception,
            "Stakepool with sym id: {} already exists.", nsact.sym_id);
        EVT_ASSERT2(nsact.sym_id == nsact.purchase_threshold.sym().id(), symbol_type_exception,
            "Purchase threshold's symbol should match stake pool");

        auto stakepool               = stakepool_def();
        stakepool.sym_id             = nsact.sym_id;
        stakepool.parameter_r        = nsact.parameter_r;
        stakepool.parameter_t        = nsact.parameter_t;
        stakepool.parameter_q        = nsact.parameter_q;
        stakepool.parameter_w        = nsact.parameter_w;
        stakepool.begin_time         = context.control.pending_block_time();
        stakepool.total              = asset(0, nsact.purchase_threshold.sym());
        stakepool.purchase_threshold = nsact.purchase_threshold;
        
        ADD_DB_TOKEN(token_type::stakepool, stakepool);
        // READ_DB_TOKEN(token_type::stakepool, std::nullopt, name128::from_number(nsact.sym_id), stakepool, unknown_stakepool_exception,"Cannot find stakepool");
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updstakepool) {
    using namespace internal;

    auto usact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(usact.sym_id)), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        EVT_ASSERT2(usact.sym_id == EVT_SYM_ID, staking_symbol_exception, "Only EVT is supported to stake currently");
        EVT_ASSERT2(usact.sym_id == usact.purchase_threshold.sym().id(), symbol_type_exception,
            "Purchase threshold's symbol should match stake pool");

        auto stakepool = make_empty_cache_ptr<stakepool_def>();
        READ_DB_TOKEN(token_type::stakepool, std::nullopt, usact.sym_id, stakepool, unknown_stakepool_exception,
            "Cannot find stakepool with sym id: {}", usact.sym_id);

        stakepool->parameter_r        = usact.parameter_r;
        stakepool->parameter_t        = usact.parameter_t;
        stakepool->parameter_q        = usact.parameter_q;
        stakepool->parameter_w        = usact.parameter_w;
        stakepool->purchase_threshold = usact.purchase_threshold;
        
        UPD_DB_TOKEN(token_type::stakepool, *stakepool);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(newvalidator) {
    using namespace internal;

    auto nvact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.staking), nvact.name), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        check_name_reserved(nvact.name);

        DECLARE_TOKEN_DB()
        EVT_ASSERT2(!tokendb.exists_token(token_type::validator, std::nullopt, nvact.name), validator_duplicate_exception,
            "validator {} already exists.", nvact.name);

        EVT_ASSERT(nvact.withdraw.name == "withdraw", permission_type_exception,
            "Name ${name} does not match with the name of withdraw permission.", ("name",nvact.withdraw.name));
        EVT_ASSERT(nvact.withdraw.threshold > 0 && validate(nvact.withdraw), permission_type_exception,
            "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(nvact.manage.name == "manage", permission_type_exception,
            "Name ${name} does not match with the name of manage permission.", ("name",nvact.manage.name));
        EVT_ASSERT(validate(nvact.manage), permission_type_exception,
            "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nvact.withdraw, false);
        pchecker(nvact.manage, false);

        auto validator              = validator_def();
        validator.name              = nvact.name;
        validator.creator           = nvact.creator;
        validator.create_time       = context.control.pending_block_time();
        validator.last_updated_time = context.control.pending_block_time();
        validator.withdraw          = std::move(nvact.withdraw);
        validator.manage            = std::move(nvact.manage);
        validator.commission        = nvact.commission;

        validator.initial_net_value = asset(1'00000, evt_sym());
        validator.current_net_value = asset(1'00000, evt_sym());;
        validator.total_units       = 0;
        
        ADD_DB_TOKEN(token_type::validator, validator);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(staketkns) {
    using namespace internal;

    auto stact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.staking), stact.validator), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto sym  = stact.amount.sym();
        EVT_ASSERT2(sym == evt_sym(), staking_symbol_exception, "Only EVT is supported to stake currently");

        auto prop = property_stakes();
        READ_DB_ASSET(stact.staker, sym, prop);
        EVT_ASSERT2(prop.amount >= stact.amount.amount(), staking_amount_exception, "Don't have enough balance to stake");

        switch(stact.type) {
        case stake_type::active: {
            EVT_ASSERT2(stact.fixed_days == 0, staking_days_exception, "Active staking cannot have fixed days");
            break;
        }
        case stake_type::fixed: {
            EVT_ASSERT2(stact.fixed_days > 0, staking_days_exception, "Fixed staking should have positive fixed days");
            break;
        }
        };  // switch

        auto validator = make_empty_cache_ptr<validator_def>();
        READ_DB_TOKEN(token_type::validator, std::nullopt, stact.validator, validator, unknown_validator_exception,
            "Cannot find validator: {}", stact.validator);

        EVT_ASSERT2(stact.amount >= validator->current_net_value, staking_amount_exception, "Needs to stake at least one unit");

        auto stakepool = make_empty_cache_ptr<stakepool_def>();
        READ_DB_TOKEN(token_type::stakepool, std::nullopt, sym.id(), stakepool, unknown_stakepool_exception,
            "Cannot find stakepool");

        EVT_ASSERT2(stact.amount >= stakepool->purchase_threshold, staking_amount_exception, "Needs to stake more than purchase threshold in stakepool");

        auto units = (int64_t)mp::floor(real_type(stact.amount.amount()) / real_type(validator->current_net_value.amount()));
        auto total = asset(units * validator->current_net_value.amount(), sym);

        // add units to validator
        validator->total_units += units;

        // add amount to stake pool
        stakepool->total += total;

        // freeze tokens and add stake share
        auto share       = stakeshare_def();
        share.validator  = stact.validator;
        share.units      = units;
        share.net_value  = total;
        share.time       = context.control.pending_block_time();
        share.type       = stact.type;
        share.fixed_days = stact.fixed_days;

        prop.amount        -= total.amount();
        prop.frozen_amount += total.amount();
        prop.stake_shares.emplace_back(share);

        UPD_DB_TOKEN(token_type::stakepool, *stakepool);
        UPD_DB_TOKEN(token_type::validator, *validator);
        PUT_DB_ASSET(stact.staker, prop);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(toactivetkns) {
    using namespace internal;
    namespace mp = boost::multiprecision;

    auto tatact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.staking), tatact.validator), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        EVT_ASSERT2(tatact.sym_id == EVT_SYM_ID, staking_symbol_exception, "Only EVT is supported to stake currently");

        auto prop = property_stakes();
        READ_DB_ASSET(tatact.staker, evt_sym(), prop);

        auto& conf = context.control.get_global_properties().stake_configuration;

        int64_t diff_amount = 0, diff_units = 0;
        for(auto& s : prop.stake_shares) {
            if(s.type == stake_type::active) {
                continue;
            }
            if(s.time + fc::days(s.fixed_days) > context.control.pending_block_time()) {
                // not expired
                continue;
            }

            auto months = (real_type)s.fixed_days / 30;
            auto roi    = mp::exp(mp::log(months / conf.fixed_R)) / conf.fixed_T;

            auto new_uints = (int64_t)mp::floor(real_type(s.units) * (1 + roi));
            diff_amount += s.net_value.amount() * (new_uints - s.units);
            diff_units  += (new_uints - s.units);

            s.units = new_uints;
            s.type = stake_type::active;
            s.fixed_days = 0;
        }
        // EVT_ASSERT2(diff_amount > 0, staking_active_exception, "There're no shares can be actived currently");

        // update pool
        auto stakepool = make_empty_cache_ptr<stakepool_def>();
        READ_DB_TOKEN(token_type::stakepool, std::nullopt, tatact.sym_id, stakepool, staking_exception,
            "Cannot find stakepool");

        stakepool->total += asset(diff_amount, evt_sym());

        // update validator
        auto validator = make_empty_cache_ptr<validator_def>();
        READ_DB_TOKEN(token_type::validator, std::nullopt, tatact.validator, validator, unknown_validator_exception,
            "Cannot find validator: {}", tatact.validator);

        validator->total_units += diff_units;

        // update database
        UPD_DB_TOKEN(token_type::stakepool, *stakepool);
        UPD_DB_TOKEN(token_type::validator, *validator);
        PUT_DB_ASSET(tatact.staker, prop);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(unstaketkns) {
    using namespace internal;

    auto ustact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.staking), ustact.validator), action_authorize_exception, "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT2(ustact.sym_id == evt_sym().id(), staking_symbol_exception, "Only EVT is supported to unstake currently");

        DECLARE_TOKEN_DB()

        EVT_ASSERT2(ustact.units > 0, staking_units_exception, "Unstake units should be large than 0");

        auto prop = property_stakes();
        READ_DB_ASSET(ustact.staker, evt_sym(), prop);

        // check and unfreeze shares

        auto& conf = context.control.get_global_properties().stake_configuration;

        switch(ustact.op) {
        case unstake_op::propose: {
            auto remainning_units = ustact.units;

            uint i = 0u;
            for(; i < prop.stake_shares.size(); i++) {
                auto& s = prop.stake_shares[i];
                if(s.validator != ustact.validator) {
                    continue;
                }

                // only active shares can be proposed
                if(s.type != stake_type::active) {
                    continue;
                }

                // adds to pending shares
                prop.pending_shares.emplace_back(s);
                prop.pending_shares.back().time = context.control.pending_block_time();

                // update units
                auto units = std::min(s.units, remainning_units);
                s.units   -= units;
                remainning_units -= units;
                prop.pending_shares.back().units = units;

                if(remainning_units == 0) {
                    break;
                }
            }
            EVT_ASSERT2(remainning_units == 0, staking_not_enough_exception, "Don't have enough staking units");

            // remove empty stake shares
            if(prop.stake_shares[i].units == 0) {
                i++;
            }
            if(i > 0) {
                prop.stake_shares.erase(prop.stake_shares.begin(), prop.stake_shares.begin() + i);
            }
            break;
        }
        case unstake_op::cancel: {
            auto remainning_units = ustact.units;

            uint i = 0u;
            for(; i < prop.pending_shares.size(); i++) {
                auto& s = prop.pending_shares[i];
                if(s.validator != ustact.validator) {
                    continue;
                }

                // adds to stake shares back
                prop.stake_shares.emplace_back(s);
                prop.stake_shares.back().time = context.control.pending_block_time();

                // update units
                auto units = std::min(s.units, remainning_units);
                s.units   -= units;
                remainning_units -= units;
                prop.pending_shares.back().units = units;

                if(remainning_units == 0) {
                    break;
                }
            }
            EVT_ASSERT2(remainning_units == 0, staking_not_enough_exception, "Don't have enough pending staking units");

            // remove empty pending shares
            if(prop.pending_shares[i].units == 0) {
                i++;
            }
            if(i > 0) {
                prop.pending_shares.erase(prop.pending_shares.begin(), prop.pending_shares.begin() + i);
            }
            break;
        }
        case unstake_op::settle: {
            int64_t frozen_amount = 0, bonus_amount = 0, remainning_units = ustact.units;

            uint i = 0u;
            for(; i < prop.pending_shares.size(); i++) {
                auto& s = prop.pending_shares[i];
                if(s.validator != ustact.validator) {
                    continue;
                }

                // only expired pending unstake shares can be settled
                if(s.time + fc::days(conf.unstake_pending_days) < context.control.pending_block_time()) {
                    continue;
                }

                // update units
                auto units = std::min(s.units, remainning_units);
                s.units   -= units;
                remainning_units -= units;

                // update amounts
                auto validator = make_empty_cache_ptr<validator_def>();
                READ_DB_TOKEN(token_type::validator, std::nullopt, ustact.validator, validator, unknown_validator_exception,
                    "Cannot find validator: {}", ustact.validator);

                auto amount = s.net_value.amount() * units;
                auto diff   = (validator->current_net_value.amount() - s.net_value.amount()) * units;

                frozen_amount += amount;
                bonus_amount  += diff;
            }
            EVT_ASSERT2(remainning_units == 0, staking_not_enough_exception, "Don't have enough pending staking units");

            // unfreeze property
            prop.amount        += frozen_amount;
            prop.frozen_amount -= frozen_amount;
            PUT_DB_ASSET(ustact.staker, prop);

            // transfer bonus from FT's initial pool
            auto addr = get_fungible_address(evt_sym());

            try {
                transfer_fungible(context, addr, ustact.staker, asset(bonus_amount, evt_sym()), N(unstaketkns), false /* pay charge */);
            }
            catch(balance_exception&) {
                EVT_THROW2(fungible_supply_exception, "Exceeds total supply of fungible with sym id: {}.", ustact.sym_id);
            }

            // remove empty pending shares
            if(prop.pending_shares[i].units == 0) {
                i++;
            }
            if(i > 0) {
                prop.pending_shares.erase(prop.pending_shares.begin(), prop.pending_shares.begin() + i);
            }
            break;
        }
        };  // switch
        
        // save changes to db
        PUT_DB_ASSET(ustact.staker, prop);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
