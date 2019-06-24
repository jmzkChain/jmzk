/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

/**
 * Implements newvalidator actions
 */

EVT_ACTION_IMPL_BEGIN(newvalidator) {
    using namespace internal;

    auto nvact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.validator), nvact.name), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        check_name_reserved(nvact.name);

        DECLARE_TOKEN_DB()
        EVT_ASSERT2(!tokendb.exists_token(token_type::validator, N128(.validator), nvact.name), validator_duplicate_exception,
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
        pchecker(nvact.with, false);
        pchecker(nvact.manage, false);

        auto validator              = validator_def();
        validator.name              = nvact.name;
        validator.creator           = nvact.creator;
        validator.create_time       = context.control.pending_block_time();
        validator.last_updated_time = context.control.pending_block_time();
        validator.withdraw          = std::move(nvact.withdraw);
        validator.manage            = std::move(nvact.manage);
        validator.commission        = nvact.commission;

        validator.initial_net_value = 1;
        validator.current_net_value = 1;
        validator.total_units       = 0;
        
        ADD_DB_TOKEN(token_type::validator, validator);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()



}}} // namespace evt::chain::contracts
