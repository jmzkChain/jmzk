/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

/**
 * Implements newdomain, issuetoken, transfer, destroytoken and updatedomain actions
 */

EVT_ACTION_IMPL_BEGIN(newdomain) {
    using namespace internal;

    auto ndact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(ndact.name, N128(.create)), action_authorize_exception, "Invalid authorization fields in action(domain and key).");

        check_name_reserved(ndact.name);

        DECLARE_TOKEN_DB()
        EVT_ASSERT(!tokendb.exists_token(token_type::domain, std::nullopt, ndact.name), domain_duplicate_exception,
            "Domain ${name} already exists.", ("name",ndact.name));

        EVT_ASSERT(ndact.issue.name == "issue", permission_type_exception,
            "Name ${name} does not match with the name of issue permission.", ("name",ndact.issue.name));
        EVT_ASSERT(ndact.issue.threshold > 0 && validate(ndact.issue), permission_type_exception,
            "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        EVT_ASSERT(ndact.transfer.name == "transfer", permission_type_exception,
            "Name ${name} does not match with the name of transfer permission.", ("name",ndact.transfer.name));
        EVT_ASSERT(validate(ndact.transfer), permission_type_exception,
            "Transfer permission is not valid, which may be caused by duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(ndact.manage.name == "manage", permission_type_exception,
            "Name ${name} does not match with the name of manage permission.", ("name",ndact.manage.name));
        EVT_ASSERT(validate(ndact.manage), permission_type_exception,
            "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(ndact.issue, false);
        pchecker(ndact.transfer, true /* allowed_owner */);
        pchecker(ndact.manage, false);

        auto domain        = domain_def();
        domain.name        = ndact.name;
        domain.creator     = ndact.creator;
        // NOTICE: we should use pending_block_time() below
        // but for historical mistakes, we use head_block_time()
        domain.create_time = context.control.head_block_time();
        domain.issue       = std::move(ndact.issue);
        domain.transfer    = std::move(ndact.transfer);
        domain.manage      = std::move(ndact.manage);
        
        ADD_DB_TOKEN(token_type::domain, domain);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(issuetoken) {
    using namespace internal;

    auto itact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(itact.domain, N128(.issue)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(!itact.owner.empty(), token_owner_exception, "Owner cannot be empty.");
        for(auto& o : itact.owner) {
            check_address_reserved(o);
        }

        DECLARE_TOKEN_DB()
        EVT_ASSERT2(tokendb.exists_token(token_type::domain, std::nullopt, itact.domain), unknown_domain_exception,
            "Cannot find domain: {}.", itact.domain);

        auto check_name = [&](const auto& name) {
            check_name_reserved(name);
            EVT_ASSERT2(!tokendb.exists_token(token_type::token, itact.domain, name), token_duplicate_exception,
                "Token: {} in {} is already exists.", name, itact.domain);
        };

        auto values  = small_vector<db_value, 4>();
        auto data    = small_vector<std::string_view, 4>();
        values.reserve(itact.names.size());
        data.reserve(itact.names.size());

        auto token   = token_def();
        token.domain = itact.domain;
        token.owner  = itact.owner;

        for(auto& n : itact.names) {
            check_name(n);

            token.name = n;
            values.emplace_back(make_db_value(token));
            data.emplace_back(values.back().as_string_view());
        }

        tokendb.put_tokens(token_type::token, action_op::add, itact.domain, std::move(itact.names), data);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

namespace internal {

bool
check_token_destroy(const token_def& token) {
    if(token.owner.size() != 1) {
        return false;
    }
    return token.owner[0].is_reserved();
}

bool
check_token_locked(const token_def& token) {
    if(token.owner.size() != 1) {
        return false;
    }
    auto& addr = token.owner[0];
    return addr.is_generated() && addr.get_prefix() == N(lock);
}

}  // namespace internal

EVT_ACTION_IMPL_BEGIN(transfer) {
    using namespace internal;

    auto ttact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(ttact.domain, ttact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(!ttact.to.empty(), token_owner_exception, "New owner cannot be empty.");
        for(auto& addr : ttact.to) {
            check_address_reserved(addr);
        }

        DECLARE_TOKEN_DB()

        auto token = make_empty_cache_ptr<token_def>();
        READ_DB_TOKEN(token_type::token, ttact.domain, ttact.name, token, unknown_token_exception,
            "Cannot find token: {} in {}", ttact.name, ttact.domain);
        assert(token->name == ttact.name);

        EVT_ASSERT(!check_token_destroy(*token), token_destroyed_exception, "Destroyed token cannot be transfered.");
        EVT_ASSERT(!check_token_locked(*token), token_locked_exception, "Locked token cannot be transfered.");

        token->owner = std::move(ttact.to);
        UPD_DB_TOKEN(token_type::token, *token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(destroytoken) {
    using namespace internal;

    auto dtact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(dtact.domain, dtact.name), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto domain = make_empty_cache_ptr<domain_def>();
        READ_DB_TOKEN(token_type::domain, std::nullopt, dtact.domain, domain, unknown_domain_exception,
            "Cannot find domain: {}", dtact.domain);       

        auto dd = get_metavalue(*domain, get_metakey<reserved_meta_key::disable_destroy>(domain_metas));
        if(dd.has_value() && *dd == "true") {
            EVT_THROW(token_cannot_destroy_exception, "Token in this domain: ${d} cannot be destroyed", ("d",dtact.domain));
        }

        auto token = make_empty_cache_ptr<token_def>();
        READ_DB_TOKEN(token_type::token, dtact.domain, dtact.name, token, unknown_token_exception,
            "Cannot find token: {} in {}", dtact.name, dtact.domain);
        assert(token->name == dtact.name);

        EVT_ASSERT(!check_token_destroy(*token), token_destroyed_exception, "Token is already destroyed.");
        EVT_ASSERT(!check_token_locked(*token), token_locked_exception, "Locked token cannot be destroyed.");

        token->owner = address_list{ address() };
        UPD_DB_TOKEN(token_type::token, *token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updatedomain) {
    using namespace internal;

    auto udact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(udact.name, N128(.update)), action_authorize_exception,
            "Authorized information does not match");

        DECLARE_TOKEN_DB()

        auto domain = make_empty_cache_ptr<domain_def>();
        READ_DB_TOKEN(token_type::domain, std::nullopt, udact.name, domain, unknown_domain_exception,"Cannot find domain: {}", udact.name);

        auto pchecker = make_permission_checker(tokendb);
        if(udact.issue.has_value()) {
            EVT_ASSERT(udact.issue->name == "issue", permission_type_exception,
                "Name ${name} does not match with the name of issue permission.", ("name",udact.issue->name));
            EVT_ASSERT(validate(*udact.issue), permission_type_exception,
                "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
            pchecker(*udact.issue, false);

            domain->issue = std::move(*udact.issue);
        }
        if(udact.transfer.has_value()) {
            auto dt = get_metavalue(*domain, get_metakey<reserved_meta_key::disable_set_transfer>(domain_metas));
            if(dt.has_value() && *dt == "true") {
                EVT_THROW(domain_cannot_update_exception, "Transfer permission of this domain cannot be updated");
            }

            EVT_ASSERT(udact.transfer->name == "transfer", permission_type_exception,
                "Name ${name} does not match with the name of transfer permission.", ("name",udact.transfer->name));
            EVT_ASSERT(validate(*udact.transfer), permission_type_exception,
                "Transfer permission is not valid, which may be caused by duplicated keys.");
            pchecker(*udact.transfer, true /* allowed_owner */);

            domain->transfer = std::move(*udact.transfer);
        }
        if(udact.manage.has_value()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(udact.manage->name == "manage", permission_type_exception,
                "Name ${name} does not match with the name of manage permission.", ("name",udact.manage->name));
            EVT_ASSERT(validate(*udact.manage), permission_type_exception,
                "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*udact.manage, false);

            domain->manage = std::move(*udact.manage);
        }

        UPD_DB_TOKEN(token_type::domain, *domain);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
