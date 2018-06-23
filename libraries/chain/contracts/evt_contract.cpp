/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_contract.hpp>

#include <algorithm>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/utilities/safemath.hpp>

namespace evt { namespace chain { namespace contracts {

namespace __internal {

inline bool 
validate(const permission_def &permission) {
    uint32_t total_weight = 0;
    for(const auto& aw : permission.authorizers) {
        if(aw.weight == 0) {
            return false;
        }
        total_weight += aw.weight;
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
    EVT_ASSERT(!group.name().empty(), action_validate_exception, "Group name cannot be empty");
    EVT_ASSERT(!group.empty(), action_validate_exception, "Don't have root node");
    auto& root = group.root();
    return validate(group, root);
}

auto make_permission_checker = [](const auto& tokendb) {
    auto checker = [&](const auto& p, auto allowed_owner) {
        for(const auto& a : p.authorizers) {
            auto& ref = a.ref;

            switch(ref.type()) {
            case authorizer_ref::account_t: {
                continue;
            }
            case authorizer_ref::owner_t: {
                EVT_ASSERT(allowed_owner, action_validate_exception, "Owner group is not allowed in ${name} permission", ("name", p.name));
                continue;  
            }
            case authorizer_ref::group_t: {
                auto& name = ref.get_group();
                auto dbexisted = tokendb.exists_group(name);
                EVT_ASSERT(dbexisted, action_validate_exception, "Group ${name} is not valid, should create group first", ("name", name));
                break;
            }
            default: {
                EVT_ASSERT(false, action_validate_exception, "Not valid authorizer ref");
            }
            }  // switch
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
        EVT_ASSERT(context.has_authorized(ndact.name, N128(.create)), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
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
        domain.issue_time = context.control.head_block_time();
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
        EVT_ASSERT(context.has_authorized(itact.domain, N128(.issue)), action_validate_exception, "Authorized information doesn't match");
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_domain(itact.domain), action_validate_exception, "Domain ${name} not existed", ("name", itact.domain));
        EVT_ASSERT(!itact.owner.empty(), action_validate_exception, "Owner cannot be empty");

        auto check_name = [&](const auto& name) {
            const uint128_t reserved_flag = ((uint128_t)0x3f << (128-6));
            EVT_ASSERT(!name.empty() && (name.value & reserved_flag), action_validate_exception, "Token name starts with '.' is reserved for system usage");
            EVT_ASSERT(!tokendb.exists_token(itact.domain, name), action_validate_exception, "Token ${domain}-${name} already existed", ("domain",itact.domain)("name",name));
        };

        for(auto& n : itact.names) {
            check_name(n);
        }

        tokendb.issue_tokens(itact);
    }
    FC_CAPTURE_AND_RETHROW((itact));
}

namespace __internal {

public_key_type
get_reservered_public_key() {
    static public_key_type pkey;
    return pkey;
}

}  // namespace __internal

void
apply_evt_transfer(apply_context& context) {
    using namespace __internal;

    auto ttact = context.act.data_as<transfer>();
    try {
        EVT_ASSERT(context.has_authorized(ttact.domain, ttact.name), action_validate_exception, "Authorized information doesn't match");
        
        auto& tokendb = context.token_db;

        bool existed = false;
        tokendb.read_token(ttact.domain, ttact.name, [&](const auto& t) {
            EVT_ASSERT(t.owner[0] != get_reservered_public_key(), action_validate_exception, "Token ${domain}-${name} is already destroyed", ("domain",ttact.domain)("name",ttact.name));
            existed = true;
        });
        EVT_ASSERT(existed, action_validate_exception, "Token ${domain}-${name} not existed", ("domain",ttact.domain)("name",ttact.name));
        
        auto ut = db_updatetoken();
        ut.domain = ttact.domain;
        ut.name   = ttact.name;
        ut.owner  = ttact.to;

        tokendb.update_token(ut);
    }
    FC_CAPTURE_AND_RETHROW((ttact));
}

void
apply_evt_destroytoken(apply_context& context) {
    using namespace __internal;

    auto dtact = context.act.data_as<destroytoken>();
    try {
        EVT_ASSERT(context.has_authorized(dtact.domain, dtact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_token(dtact.domain, dtact.name), action_validate_exception, "Token ${domain}-${name} not existed", ("domain",dtact.domain)("name",dtact.name));
        
        auto ut = db_updatetoken();
        ut.domain = dtact.domain;
        ut.name   = dtact.name;
        ut.owner  = user_list{ get_reservered_public_key() };

        tokendb.update_token(ut);
    }
    FC_CAPTURE_AND_RETHROW((dtact));
}

void
apply_evt_newgroup(apply_context& context) {
    using namespace __internal;

    auto ngact = context.act.data_as<newgroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(group), ngact.name), action_validate_exception, "Authorized information doesn't match");
        EVT_ASSERT(ngact.name == ngact.group.name(), action_validate_exception, "The names in action are not the same");
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_group(ngact.name), action_validate_exception, "Group ${name} is already existed", ("name",ngact.name));
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
        EVT_ASSERT(context.has_authorized(N128(group), ugact.name), action_validate_exception, "Authorized information doesn't match");
        EVT_ASSERT(ugact.name == ugact.group.name(), action_validate_exception, "The names in action are not the same");

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_group(ugact.name), action_validate_exception, "Group ${name} not existed", ("name",ugact.name));
        EVT_ASSERT(validate(ugact.group), action_validate_exception, "Updated group is not valid");

        auto ug = db_updategroup();
        ug.name  = ugact.name;
        ug.group = ugact.group;

        tokendb.update_group(ug);
    }
    FC_CAPTURE_AND_RETHROW((ugact));
}

void
apply_evt_updatedomain(apply_context& context) {
    using namespace __internal;

    auto udact = context.act.data_as<updatedomain>();
    try {
        EVT_ASSERT(context.has_authorized(udact.name, N128(.update)), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
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

        auto ud = db_updatedomain();
        ud.name     = udact.name;
        ud.issue    = udact.issue;
        ud.transfer = udact.transfer;
        ud.manage   = udact.manage;

        tokendb.update_domain(ud);
    }
    FC_CAPTURE_AND_RETHROW((udact));
}

void
apply_evt_newaccount(apply_context& context) {
    using namespace __internal;

    auto naact = context.act.data_as<newaccount>();
    try {
        EVT_ASSERT(context.has_authorized(N128(account), naact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
        EVT_ASSERT(!naact.name.empty(), action_validate_exception, "Account name shouldn't be empty");
        EVT_ASSERT(!tokendb.exists_account(naact.name), action_validate_exception, "Account ${name} already existed", ("name",naact.name));
    
        auto account = account_def();
        account.name = naact.name;
        account.creator = config::system_account_name;
        account.create_time = context.control.head_block_time();
        account.balance = asset(10000);
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

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_account(uoact.name), action_validate_exception, "Account ${name} don't exist", ("name",uoact.name));
        EVT_ASSERT(uoact.owner.size() > 0, action_validate_exception, "Owner cannot be empty");

        auto ua = db_updateaccount();
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

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_account(teact.from), action_validate_exception, "Account ${name} don't exist", ("name",teact.from));
        EVT_ASSERT(tokendb.exists_account(teact.to), action_validate_exception, "Account ${name} don't exist", ("name",teact.to));
        EVT_ASSERT(teact.amount.get_amount() > 0, action_validate_exception, "Transfer amount must be positive");

        account_def facc, tacc;
        tokendb.read_account(teact.from, [&](const auto& a) {
            facc = a;
        });
        tokendb.read_account(teact.to, [&](const auto& a) {
            tacc = a;
        });

        EVT_ASSERT(facc.balance >= teact.amount, action_validate_exception, "Account ${name} don't have enough balance left", ("name",teact.from));
        
        bool r1, r2;
        decltype(facc.balance.get_amount()) r;
        r1 = safemath::test_sub(facc.balance.get_amount(), teact.amount.get_amount(), r);
        r2 = safemath::test_add(tacc.balance.get_amount(), teact.amount.get_amount(), r);
        EVT_ASSERT(r1 && r2, action_validate_exception, "Opeartions resulted in overflow results");
        facc.balance -= teact.amount;
        tacc.balance += teact.amount;

        auto fua    = db_updateaccount();
        fua.name    = facc.name;
        fua.balance = facc.balance;

        auto tua    = db_updateaccount();
        tua.name    = tacc.name;
        tua.balance = tacc.balance;

        tokendb.update_account(fua);
        tokendb.update_account(tua);
    }
    FC_CAPTURE_AND_RETHROW((teact));
}

namespace __internal {

bool
check_involved_node(const group& group, const group::node& node, const public_key_type& key) {
    auto result = false;
    group.visit_node(node, [&](const auto& n) {
        if(n.is_leaf()) {
            if(group.get_leaf_key(n) == key) {
                result = true;
                // find one, return false to stop iterate group
                return false;
            }
            return true;
        }
        if(check_involved_node(group, n, key)) {
            result = true;
            // find one, return false to stop iterate group
            return false;
        }
        return true;
    });
    return result;
}

auto check_involved_permission = [](const auto& tokendb, const auto& permission, const auto& key) {
    for(auto& a : permission.authorizers) {
        auto& ref = a.ref;
        switch(ref.type()) {
        case authorizer_ref::account_t: {
            if(ref.get_account() == key) {
                return true;
            }
            break;
        }
        case authorizer_ref::group_t: {
            const auto& group = ref.get_group();
            auto result = false;
            tokendb.read_group(group, [&](const auto& g) {
                result = check_involved_node(g, g.root(), key);
            });
            if(result) {
                return true;
            }
            break;
        }
        }  // switch
    }
    return false;
};

auto check_involved_domain = [](const auto& tokendb, const auto& domain, auto pname, const auto& key) {
    auto result = false;
    tokendb.read_domain(domain, [&](const auto& d) {
        switch(pname) {
        case N(issue): {
            result = check_involved_permission(tokendb, d.issue, key);
        }
        case N(transfer): {
            result = check_involved_permission(tokendb, d.transfer, key);
        }
        case N(manage): {
            result = check_involved_permission(tokendb, d.manage, key);
        }
        }  // switch
    });
    return result;
};

auto check_involved_group = [](const auto& tokendb, const auto& group, const auto& key) {
    auto result = false;
    tokendb.read_group(group, [&](const auto& g) {
        if(g.key() == key) {
            result = true;
        }
    });
    return result;
};

auto check_involved_owner = [](const auto& tokendb, const auto& domain, const auto& name, const auto& key) {
    auto result = false;
    tokendb.read_token(domain, name, [&](const auto& t) {
        if(std::find(t.owner.cbegin(), t.owner.cend(), key) != t.owner.cend()) {
            result = true;
        }
    });
    return result;
};

auto check_duplicate_meta_domain = [](const auto& tokendb, const auto& domain, const auto& key) {
    auto result = false;
    tokendb.read_domain(domain, [&](const auto& d) {
        if(std::find_if(d.metas.cbegin(), d.metas.cend(), [&](const auto& meta) { return meta.key == key; }) != d.metas.cend()) {
            result = true;
        }
    });
    return result;
};

auto check_duplicate_meta_group = [](const auto& tokendb, const auto& group, const auto& key) {
    auto result = false;
    tokendb.read_group(group, [&](const auto& g) {
        if(std::find_if(g.metas_.cbegin(), g.metas_.cend(), [&](const auto& meta) { return meta.key == key; }) != g.metas_.cend()) {
            result = true;
        }
    });
    return result;
};

auto check_duplicate_meta_token = [](const auto& tokendb, const auto& domain, const auto& name, const auto& key) {
    auto result = false;
    tokendb.read_token(domain, name, [&](const auto& t) {
        if(std::find_if(t.metas.cbegin(), t.metas.cend(), [&](const auto& meta) { return meta.key == key; }) != t.metas.cend()) {
            result = true;
        }
    });
    return result;
};

}  // namespace __internal

void
apply_evt_addmeta(apply_context& context) {
    using namespace __internal;

    const auto& act   = context.act;
    auto        amact = context.act.data_as<addmeta>();
    try {
        auto& tokendb = context.token_db;

        if(act.domain == N128(group)) {
            EVT_ASSERT(tokendb.exists_group(act.key), action_validate_exception, "Group ${name} doesn't existed", ("name",act.key));
            EVT_ASSERT(!check_duplicate_meta_group(tokendb, act.key, amact.key), action_validate_exception, "Metadata with key ${key} is already existed", ("key",amact.key));
            // check involved, only group manager(aka. group key) can add meta
            EVT_ASSERT(check_involved_group(tokendb, act.key, amact.creator), action_validate_exception, "Creator is not involved in group ${name}", ("name",act.key));

            auto ud = db_updategroup();
            ud.name  = act.key;
            ud.metas = meta_list { meta(amact.key, amact.value, amact.creator) };

            tokendb.update_group(ud);
        }
        else if(act.key == N128(.meta)) {
            EVT_ASSERT(tokendb.exists_domain(act.domain), action_validate_exception, "Domain ${name} doesn't existed", ("name",act.domain));
            EVT_ASSERT(!check_duplicate_meta_domain(tokendb, act.domain, amact.key), action_validate_exception, "Metadata with key ${key} is already existed", ("key",amact.key));
            // check involved, only person involved in `manage` permission can add meta
            EVT_ASSERT(check_involved_domain(tokendb, act.domain, N(manage), amact.creator), action_validate_exception, "Creator is not involved in domain ${name}", ("name",act.key));

            auto ud = db_updatedomain();
            ud.name  = act.domain;
            ud.metas = meta_list { meta(amact.key, amact.value, amact.creator) };

            tokendb.update_domain(ud);
        }
        else {
            EVT_ASSERT(tokendb.exists_token(act.domain, act.key), action_validate_exception, "Token ${domain}-${name} not existed", ("domain",act.domain)("name",act.key));
            EVT_ASSERT(!check_duplicate_meta_token(tokendb, act.domain, act.key, amact.key), action_validate_exception, "Metadata with key ${key} is already existed", ("key",amact.key));

            // check involved, only person involved in `issue` and `transfer` permissions or `owners` can add meta
            auto involved = check_involved_owner(tokendb, act.domain, act.key, amact.creator)
                || check_involved_domain(tokendb, act.domain, N(issue), amact.creator)
                || check_involved_domain(tokendb, act.domain, N(transfer), amact.creator);
            EVT_ASSERT(involved, action_validate_exception, "Creator is not involved in token ${domain}-${name}", ("domain",act.domain)("name",act.key));

            auto ut = db_updatetoken();
            ut.domain = act.domain;
            ut.name   = act.key;
            ut.metas  = meta_list { meta(amact.key, amact.value, amact.creator) };

            tokendb.update_token(ut);
        }
    }
    FC_CAPTURE_AND_RETHROW((context.act));
}

void
apply_evt_newdelay(apply_context& context) {
    using namespace __internal;

    auto ndact = context.act.data_as<newdelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), ndact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
        EVT_ASSERT(!ndact.name.empty(), action_validate_exception, "Proposal name shouldn't be empty");
        EVT_ASSERT(!tokendb.exists_delay(ndact.name), action_validate_exception, "Delay ${name} already existed", ("name",ndact.name));

        auto delay = delay_def {
            ndact.name,
            ndact.proposer,
            delay_status::proposed,
            ndact.trx
        };
        auto& keys = context.trx_context.trx.recover_keys(context.control.get_chain_id());
        delay.signed_keys.reserve(keys.size());
        delay.signed_keys.insert(delay.signed_keys.end(), keys.cbegin(), keys.cend());

        tokendb.add_delay(delay);
    }
    FC_CAPTURE_AND_RETHROW((ndact));
}

void
apply_evt_approvedelay(apply_context& context) {
    using namespace __internal;

    auto adact = context.act.data_as<approvedelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), adact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
        bool  existed = false;
        flat_set<public_key_type> signed_keys;
        tokendb.read_delay(adact.name, [&](const auto& delay) {
            EVT_ASSERT(delay.status == delay_status::proposed, action_validate_exception, "Delay is not in proper status");
            signed_keys = delay.trx.get_signature_keys(adact.signatures, context.control.get_chain_id());
            existed = true;
        });
        EVT_ASSERT(existed, action_validate_exception, "Delay ${name} is not existed", ("name",adact.name));

        auto& keys = context.trx_context.trx.recover_keys(context.control.get_chain_id());
        EVT_ASSERT(signed_keys.size() == keys.size(), action_validate_exception, "Signed keys and signatures are not match");

        auto it  = signed_keys.cbegin();
        auto it2 = keys.cbegin();
        for(; it != signed_keys.cend(); it++, it2++) {
            EVT_ASSERT(*it == *it2, action_validate_exception, "Signed keys and signatures are not match");
        }

        auto ud = db_updatedelay();
        ud.signed_keys->insert(ud.signed_keys->end(), signed_keys.cbegin(), signed_keys.cend());

        tokendb.update_delay(ud);
    }
    FC_CAPTURE_AND_RETHROW((adact))
}

void
apply_evt_canceldelay(apply_context& context) {
    using namespace __internal;

    auto cdact = context.act.data_as<canceldelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), cdact.name), action_validate_exception, "Authorized information doesn't match");

        auto& tokendb = context.token_db;
        bool  existed = false;
        tokendb.read_delay(cdact.name, [&](const auto& delay) {
            EVT_ASSERT(delay.status == delay_status::proposed, action_validate_exception, "Delay is not in proper status");
            existed = true;
        });
        EVT_ASSERT(existed, action_validate_exception, "Delay ${name} is not existed", ("name",cdact.name));

        auto ud = db_updatedelay();
        ud.status = delay_status::cancelled;

        tokendb.update_delay(ud);
    }
    FC_CAPTURE_AND_RETHROW((cdact))
}

void
apply_evt_executedelay(apply_context& context) {

}

}}} // namespace evt::chain::contracts
