/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements newgroup and updategroup actions
 */

jmzk_ACTION_IMPL_BEGIN(newgroup) {
    using namespace internal;

    auto ngact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.group), ngact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(!ngact.group.key().is_generated(), group_key_exception, "Group key cannot be generated key");
        jmzk_ASSERT(ngact.name == ngact.group.name(), group_name_exception,
            "Group name not match, act: ${n1}, group: ${n2}", ("n1",ngact.name)("n2",ngact.group.name()));
        
        check_name_reserved(ngact.name);
        
        DECLARE_TOKEN_DB()
        jmzk_ASSERT(!tokendb.exists_token(token_type::group, std::nullopt, ngact.name), group_duplicate_exception,
            "Group ${name} already exists.", ("name",ngact.name));
        jmzk_ASSERT(validate(ngact.group), group_type_exception, "Input group is not valid.");

        ADD_DB_TOKEN(token_type::group, ngact.group);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

jmzk_ACTION_IMPL_BEGIN(updategroup) {
    using namespace internal;

    auto ugact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.group), ugact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        jmzk_ASSERT(ugact.name == ugact.group.name(), group_name_exception, "Names in action are not the same.");

        DECLARE_TOKEN_DB()
        
        auto group = make_empty_cache_ptr<group_def>();
        READ_DB_TOKEN(token_type::group, std::nullopt, ugact.name, group, unknown_group_exception, "Cannot find group: {}", ugact.name);
        
        jmzk_ASSERT(!group->key().is_reserved(), group_key_exception, "Reserved group key cannot be used to udpate group");
        jmzk_ASSERT(validate(ugact.group), group_type_exception, "Updated group is not valid.");

        *group = ugact.group;
        UPD_DB_TOKEN(token_type::group, *group);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts
