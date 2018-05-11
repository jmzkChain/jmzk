/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_contract.hpp>

#include <algorithm>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <evt/chain/contracts/chain_initializer.hpp>

#include <evt/chain/chain_controller.hpp>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/exceptions.hpp>

#include <evt/utilities/safemath.hpp>

namespace evt { namespace chain { namespace contracts {

namespace __internal {

inline bool 
validate(const permission_def &permission) {
    uint32_t total_weight = 0;
    const contracts::authorizer_weight* prev = nullptr;
    for(const auto& aw : permission.authorizers) {
        if(aw.weight == 0) {
            return false;
        }
        total_weight += aw.weight;
        prev = &aw;
    }
    return total_weight >= permission.threshold;
}

inline bool
validate(const group& group, const group::node& node) {
    EVT_ASSERT(node.validate(), group_type_exception, "Node is invalid: ${node}", ("node",node));
    if(!node.is_leaf()) {
        auto total_weight = 0u;
        auto result = true;
        group.visit_node(node, [&](auto& n) {
            if(!validate(group, n)) {
                result = false;
                return false;
            } 
            total_weight += n.weight;
            return true;
        });
        if(!result) {
            return false;
        }
        return total_weight >= node.threshold;
    }
    return true;
}

inline bool
validate(const group& group) {
    EVT_ASSERT(group_id::from_group_key(group.key()) == group.id(), action_validate_exception, "Group id and key are not match");
    EVT_ASSERT(group.nodes_.size() > 0, action_validate_exception, "Don't have root node");
    auto& root = group.root();
    return validate(group, root);
}

auto make_permission_checker = [](const auto& tokendb) {
    auto checker = [&](const auto& p, auto allowed_owner) {
        for(const auto& a : p.authorizers) {
            auto& ref = a.ref;
            if(ref.is_account_ref()) {
                continue;
            }
            FC_ASSERT(ref.is_group_ref());
            auto& gid = ref.get_group();
            if(gid.empty()) {
                // owner group
                EVT_ASSERT(allowed_owner, action_validate_exception, "Owner group is not allowed in ${name} permission", ("name", p.name));
                continue;
            }
            auto dbexisted = tokendb.exists_group(gid);
            EVT_ASSERT(dbexisted, action_validate_exception, "Group ${id} is not valid, should create group first", ("id", gid));
        }
    };
    return checker;
};

} // namespace __internal

void
apply_evt_newdomain(apply_context& context) {
    using namespace __internal;

    auto ndact = context.act.data_as<newdomain>();
    try {
        EVT_ASSERT(context.has_authorized("domain", (uint128_t)ndact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(!tokendb.exists_domain(ndact.name), action_validate_exception, "Domain ${name} already existed", ("name",ndact.name));

        EVT_ASSERT(!ndact.name.empty(), action_validate_exception, "Domain name shouldn't be empty");
        EVT_ASSERT(ndact.issue.name == "issue", action_validate_exception, "Name of issue permission is not valid, provided: ${name}", ("name",ndact.issue.name));
        EVT_ASSERT(ndact.issue.threshold > 0 && validate(ndact.issue), action_validate_exception, "Issue permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
        EVT_ASSERT(ndact.transfer.name == "transfer", action_validate_exception, "Name of transfer permission is not valid, provided: ${name}", ("name",ndact.transfer.name));
        EVT_ASSERT(ndact.transfer.threshold > 0 && validate(ndact.transfer), action_validate_exception, "Transfer permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(ndact.manage.name == "manage", action_validate_exception, "Name of transfer permission is not valid, provided: ${name}", ("name",ndact.manage.name));
        EVT_ASSERT(validate(ndact.manage), action_validate_exception, "Manage permission not valid, maybe exist duplicate keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(ndact.issue, false);
        pchecker(ndact.transfer, true);
        pchecker(ndact.manage, false);

        domain_def domain;
        domain.name = ndact.name;
        domain.issuer = ndact.issuer;
        domain.issue_time = context.controller.head_block_time();
        domain.issue = ndact.issue;
        domain.transfer = ndact.transfer;
        domain.manage = ndact.manage;
        
        tokendb.add_domain(domain);       
    }
    FC_CAPTURE_AND_RETHROW((ndact)) 
}

void
apply_evt_issuetoken(apply_context& context) {
    auto itact = context.act.data_as<issuetoken>();
    try {
        EVT_ASSERT(context.has_authorized(itact.domain, N128(issue)), action_validate_exception, "Authorized information doesn't match");
        
        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(tokendb.exists_domain(itact.domain), action_validate_exception, "Domain ${name} not existed", ("name", itact.domain));
        EVT_ASSERT(!itact.owner.empty(), action_validate_exception, "Owner cannot be empty");

        for(auto& n : itact.names) {
            EVT_ASSERT(!tokendb.exists_token(itact.domain, n), action_validate_exception, "Token ${domain}-${name} already existed", ("domain",itact.domain)("name",n));
        }
        tokendb.issue_tokens(itact);
    }
    FC_CAPTURE_AND_RETHROW((itact));
}

void
apply_evt_transfer(apply_context& context) {
    auto ttact = context.act.data_as<transfer>();
    EVT_ASSERT(context.has_authorized(ttact.domain, (uint128_t)ttact.name), action_validate_exception, "Authorized information doesn't match");
    
    auto& tokendb = context.mutable_tokendb;
    EVT_ASSERT(tokendb.exists_token(ttact.domain, ttact.name), action_validate_exception, "Token ${domain}-${name} not existed", ("domain",ttact.domain)("name",ttact.name));
    
    tokendb.transfer_token(ttact);
}

void
apply_evt_newgroup(apply_context& context) {
    using namespace __internal;

    auto ngact = context.act.data_as<newgroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(group), ngact.id), action_validate_exception, "Authorized information doesn't match");
        
        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(!tokendb.exists_group(ngact.id), action_validate_exception, "Group ${id} is already existed", ("id",ngact.id));
        EVT_ASSERT(validate(ngact.group), action_validate_exception, "Input group is not valid");

        tokendb.add_group(ngact.group);
    }
    FC_CAPTURE_AND_RETHROW((ngact));
}

void
apply_evt_updategroup(apply_context& context) {
    using namespace __internal;

    auto ugact = context.act.data_as<updategroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(group), ugact.id), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(tokendb.exists_group(ugact.id), action_validate_exception, "Group ${id} not existed", ("id",ugact.id));
        EVT_ASSERT(validate(ugact.group), action_validate_exception, "Updated group is not valid");

        tokendb.update_group(ugact);
    }
    FC_CAPTURE_AND_RETHROW((ugact));
}

void
apply_evt_updatedomain(apply_context& context) {
    using namespace __internal;

    auto udact = context.act.data_as<updatedomain>();
    try {
        EVT_ASSERT(context.has_authorized(N128(domain), udact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(tokendb.exists_domain(udact.name), action_validate_exception, "Domain ${name} is not existed", ("name",udact.name));

        EVT_ASSERT(!udact.name.empty(), action_validate_exception, "Domain name shouldn't be empty");

        auto pchecker = make_permission_checker(tokendb);
        if(udact.issue.valid()) {
            EVT_ASSERT(udact.issue->name == "issue", action_validate_exception, "Name of issue permission is not valid, provided: ${name}", ("name",udact.issue->name));
            EVT_ASSERT(udact.issue->threshold > 0 && validate(*udact.issue), action_validate_exception, "Issue permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
            pchecker(*udact.issue, false);
        }
        if(udact.transfer.valid()) {
            EVT_ASSERT(udact.transfer->name == "transfer", action_validate_exception, "Name of transfer permission is not valid, provided: ${name}", ("name",udact.transfer->name));
            EVT_ASSERT(udact.transfer->threshold > 0 && validate(*udact.transfer), action_validate_exception, "Transfer permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
            pchecker(*udact.transfer, true);
        }
        if(udact.manage.valid()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(udact.manage->name == "manage", action_validate_exception, "Name of manage permission is not valid, provided: ${name}", ("name",udact.manage->name));
            EVT_ASSERT(validate(*udact.manage), action_validate_exception, "Manage permission not valid, maybe exist duplicate keys.");
            pchecker(*udact.manage, false);
        }

        tokendb.update_domain(udact);
    }
    FC_CAPTURE_AND_RETHROW((udact));
}

void
apply_evt_newaccount(apply_context& context) {
    using namespace __internal;

    auto naact = context.act.data_as<newaccount>();
    try {
        EVT_ASSERT(context.has_authorized(N128(account), naact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(!naact.name.empty(), action_validate_exception, "Account name shouldn't be empty");
        EVT_ASSERT(!tokendb.exists_account(naact.name), action_validate_exception, "Account ${name} already existed", ("name",naact.name));
    
        auto account = account_def();
        account.name = naact.name;
        account.creator = config::system_account_name;
        account.balance = asset(0);
        account.frozen_balance = asset(0);
        account.owner = std::move(naact.owner);

        tokendb.add_account(account);
    }
    FC_CAPTURE_AND_RETHROW((naact));
}

void
apply_evt_updateowner(apply_context& context) {
    using namespace __internal;

    auto uoact = context.act.data_as<updateowner>();
    try {
        EVT_ASSERT(context.has_authorized(N128(account), uoact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(tokendb.exists_account(uoact.name), action_validate_exception, "Account ${name} don't exist", ("name",uoact.name));
        EVT_ASSERT(uoact.owner.size() > 0, action_validate_exception, "Owner cannot be empty");

        auto ua = updateaccount();
        ua.name = uoact.name;
        ua.owner = uoact.owner;
        tokendb.update_account(ua);
    }
    FC_CAPTURE_AND_RETHROW((uoact));
}

void
apply_evt_transferevt(apply_context& context) {
    using namespace __internal;

    auto teact = context.act.data_as<transferevt>();
    try {
        EVT_ASSERT(context.has_authorized(N128(account), teact.from), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.mutable_tokendb;
        EVT_ASSERT(tokendb.exists_account(teact.from), action_validate_exception, "Account ${name} don't exist", ("name",teact.from));
        EVT_ASSERT(tokendb.exists_account(teact.to), action_validate_exception, "Account ${name} don't exist", ("name",teact.to));
        EVT_ASSERT(teact.amount.amount > 0, action_validate_exception, "Transfer amount must be positive");

        account_def facc, tacc;
        tokendb.read_account(teact.from, [&](const auto& a) {
            facc = a;
        });
        tokendb.read_account(teact.to, [&](const auto& a) {
            tacc = a;
        });

        EVT_ASSERT(facc.balance >= teact.amount, action_validate_exception, "Account ${name} don't have enough balance left", ("name",teact.from));
        
        bool r1, r2;
        r1 = safemath::test_sub(facc.balance.amount, teact.amount.amount, facc.balance.amount);
        r2 = safemath::test_add(tacc.balance.amount, teact.amount.amount, tacc.balance.amount);
        EVT_ASSERT(r1 && r2, action_validate_exception, "Opeartions resulted in overflow results");
        facc.balance -= teact.amount;
        tacc.balance += teact.amount;

        auto fua = updateaccount();
        fua.name = facc.name;
        fua.balance = facc.balance;

        auto tua = updateaccount();
        tua.name = tacc.name;
        tua.balance = tacc.balance;

        tokendb.update_account(fua);
        tokendb.update_account(tua);
    }
    FC_CAPTURE_AND_RETHROW((teact));
}

} } } // namespace evt::chain::contracts
