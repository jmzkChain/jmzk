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
    EVT_ASSERT(!group.name().empty(), group_type_exception, "Group name cannot be empty.");
    EVT_ASSERT(!group.empty(), group_type_exception, "Root node does not exist.");
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
                EVT_ASSERT(allowed_owner, permission_type_exception, "Owner group does not show up in ${name} permission, and it only appears in Transfer.", ("name", p.name));
                continue;  
            }
            case authorizer_ref::group_t: {
                auto& name = ref.get_group();
                auto dbexisted = tokendb.exists_group(name);
                EVT_ASSERT(dbexisted, group_not_existed_exception, "Group ${name} does not exist.", ("name", name));
                break;
            }
            default: {
                EVT_ASSERT(false, authorizer_ref_type_exception, "Authorizer ref is not valid.");
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
        EVT_ASSERT(context.has_authorized(ndact.name, N128(.create)), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_domain(ndact.name), domain_exists_exception, "Domain ${name} already exists.", ("name",ndact.name));

        EVT_ASSERT(!ndact.name.empty(), domain_name_exception, "Domain name cannot be empty.");
        EVT_ASSERT(ndact.issue.name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",ndact.issue.name));
        EVT_ASSERT(ndact.issue.threshold > 0 && validate(ndact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
        EVT_ASSERT(ndact.transfer.name == "transfer", permission_type_exception, "Name ${name} does not match with the name of transfer permission.", ("name",ndact.transfer.name));
        EVT_ASSERT(ndact.transfer.threshold > 0 && validate(ndact.transfer), permission_type_exception, "Transfer permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(ndact.manage.name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",ndact.manage.name));
        EVT_ASSERT(validate(ndact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(ndact.issue, false);
        pchecker(ndact.transfer, true);
        pchecker(ndact.manage, false);

        domain_def domain;
        domain.name        = ndact.name;
        domain.creator     = ndact.creator;
        domain.create_time = context.control.head_block_time();
        domain.issue       = std::move(ndact.issue);
        domain.transfer    = std::move(ndact.transfer);
        domain.manage      = std::move(ndact.manage);
        
        tokendb.add_domain(domain);       
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_issuetoken(apply_context& context) {
    auto itact = context.act.data_as<issuetoken>();
    try {
        EVT_ASSERT(context.has_authorized(itact.domain, N128(.issue)), action_authorize_exception, "Authorized information does not match.");
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_domain(itact.domain), domain_not_existed_exception, "Domain ${name} does not exist.", ("name", itact.domain));
        EVT_ASSERT(!itact.owner.empty(), token_owner_exception, "Owner cannot be empty.");

        auto check_name = [&](const auto& name) {
            const uint128_t reserved_flag = ((uint128_t)0x3f << (128-6));
            EVT_ASSERT(!name.empty() && (name.value & reserved_flag), token_name_exception, "Name starting with '.' is reserved for system usages.");
            EVT_ASSERT(!tokendb.exists_token(itact.domain, name), token_exists_exception, "Token ${domain}-${name} already exists.", ("domain",itact.domain)("name",name));
        };

        for(auto& n : itact.names) {
            check_name(n);
        }

        tokendb.issue_tokens(itact);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

namespace __internal {

public_key_type
get_reservered_public_key() {
    static public_key_type pkey;
    return pkey;
}

bool
check_token_destroy(const token_def& token) {
    if(token.owner.size() > 1) {
        return false;
    }
    return token.owner[0] == get_reservered_public_key();
}

}  // namespace __internal

void
apply_evt_transfer(apply_context& context) {
    using namespace __internal;

    auto ttact = context.act.data_as<transfer>();
    try {
        EVT_ASSERT(context.has_authorized(ttact.domain, ttact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        token_def token;
        tokendb.read_token(ttact.domain, ttact.name, token);

        EVT_ASSERT(!check_token_destroy(token), token_destoryed_exception, "Token is already destroyed.");

        token.owner = std::move(ttact.to);
        tokendb.update_token(token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_destroytoken(apply_context& context) {
    using namespace __internal;

    auto dtact = context.act.data_as<destroytoken>();
    try {
        EVT_ASSERT(context.has_authorized(dtact.domain, dtact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        token_def token;
        tokendb.read_token(dtact.domain, dtact.name, token);

        EVT_ASSERT(!check_token_destroy(token), token_destoryed_exception, "Token is already destroyed.");

        token.owner = user_list{ get_reservered_public_key() };
        tokendb.update_token(token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_newgroup(apply_context& context) {
    using namespace __internal;

    auto ngact = context.act.data_as<newgroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(group), ngact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(ngact.name == ngact.group.name(), group_name_exception, "Names in action are not the same.");
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_group(ngact.name), group_exists_exception, "Group ${name} already exists.", ("name",ngact.name));
        EVT_ASSERT(validate(ngact.group), group_type_exception, "Input group is not valid.");

        tokendb.add_group(std::move(ngact.group));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_updategroup(apply_context& context) {
    using namespace __internal;

    auto ugact = context.act.data_as<updategroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(group), ugact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(ugact.name == ugact.group.name(), group_name_exception, "Names in action are not the same.");

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_group(ugact.name), group_not_existed_exception, "Group ${name} does not exist.", ("name",ugact.name));
        EVT_ASSERT(validate(ugact.group), group_type_exception, "Updated group is not valid.");

        tokendb.update_group(std::move(ugact.group));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_updatedomain(apply_context& context) {
    using namespace __internal;

    auto udact = context.act.data_as<updatedomain>();
    try {
        EVT_ASSERT(context.has_authorized(udact.name, N128(.update)), action_authorize_exception, "Authorized information does not match");

        auto& tokendb = context.token_db;

        domain_def domain;
        tokendb.read_domain(udact.name, domain);

        auto pchecker = make_permission_checker(tokendb);
        if(udact.issue.valid()) {
            EVT_ASSERT(udact.issue->name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",udact.issue->name));
            EVT_ASSERT(udact.issue->threshold > 0 && validate(*udact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
            pchecker(*udact.issue, false);

            domain.issue = std::move(*udact.issue);
        }
        if(udact.transfer.valid()) {
            EVT_ASSERT(udact.transfer->name == "transfer", permission_type_exception, "Name ${name} does not match with the name of transfer permission.", ("name",udact.transfer->name));
            EVT_ASSERT(udact.transfer->threshold > 0 && validate(*udact.transfer), permission_type_exception, "Transfer permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
            pchecker(*udact.transfer, true);

            domain.transfer = std::move(*udact.transfer);
        }
        if(udact.manage.valid()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(udact.manage->name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",udact.manage->name));
            EVT_ASSERT(validate(*udact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*udact.manage, false);

            domain.manage = std::move(*udact.manage);
        }

        tokendb.update_domain(domain);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_newfungible(apply_context& context) {
    using namespace __internal;

    auto nfact = context.act.data_as<newfungible>();
    try {
        EVT_ASSERT(context.has_authorized(N128(fungible), (fungible_name)nfact.sym.name()), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_fungible(nfact.sym), fungible_exists_exception, "Fungible with symbol: ${sym} already exists.", ("sym",nfact.sym.name()));
        EVT_ASSERT(nfact.sym == nfact.total_supply.get_symbol(), fungible_symbol_exception, "Symbols are not the same.");
        EVT_ASSERT(nfact.total_supply.get_amount() <= ASSET_MAX_SHARE_SUPPLY, fungible_supply_exception, "Supply exceeds the maximum allowed.");

        EVT_ASSERT(nfact.issue.name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",nfact.issue.name));
        EVT_ASSERT(nfact.issue.threshold > 0 && validate(nfact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(nfact.manage.name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",nfact.manage.name));
        EVT_ASSERT(validate(nfact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nfact.issue, false);
        pchecker(nfact.manage, false);

        fungible_def fungible;
        fungible.sym            = nfact.sym;
        fungible.creator        = nfact.creator;
        fungible.create_time    = context.control.head_block_time();
        fungible.issue          = std::move(nfact.issue);
        fungible.manage         = std::move(nfact.manage);
        fungible.total_supply   = nfact.total_supply;
        fungible.current_supply = asset(0, fungible.sym);

        tokendb.add_fungible(fungible);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_updfungible(apply_context& context) {
    using namespace __internal;

    auto ufact = context.act.data_as<updfungible>();
    try {
        EVT_ASSERT(context.has_authorized(N128(fungible), (fungible_name)ufact.sym.name()), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        fungible_def fungible;
        tokendb.read_fungible(ufact.sym, fungible);

        EVT_ASSERT(fungible.sym == ufact.sym, fungible_symbol_exception, "Symbols are not the same.");

        auto pchecker = make_permission_checker(tokendb);
        if(ufact.issue.valid()) {
            EVT_ASSERT(ufact.issue->name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",ufact.issue->name));
            EVT_ASSERT(ufact.issue->threshold > 0 && validate(*ufact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys or unordered keys.");
            pchecker(*ufact.issue, false);

            fungible.issue = std::move(*ufact.issue);
        }
        if(ufact.manage.valid()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(ufact.manage->name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",ufact.manage->name));
            EVT_ASSERT(validate(*ufact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.manage, false);

            fungible.manage = std::move(*ufact.manage);
        }

        tokendb.update_fungible(fungible);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_issuefungible(apply_context& context) {
    auto ifact = context.act.data_as<issuefungible>();

    try {
        auto sym = ifact.number.get_symbol();
        EVT_ASSERT(context.has_authorized(N128(fungible), (fungible_name)sym.name()), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        fungible_def fungible;
        tokendb.read_fungible(sym, fungible);

        decltype(ifact.number.get_amount()) rr;
        auto r = safemath::test_add(fungible.current_supply.get_amount(), ifact.number.get_amount(), rr);
        EVT_ASSERT(r, math_overflow_exception, "Operations resulted in overflows.");

        fungible.current_supply += ifact.number;
        if(fungible.total_supply.get_amount() > 0) {
            EVT_ASSERT(fungible.current_supply <= fungible.total_supply, fungible_supply_exception, "Total supply overflows.");
        }
        else {
            EVT_ASSERT(fungible.current_supply.get_amount() <= ASSET_MAX_SHARE_SUPPLY, fungible_supply_exception, "Current supply exceeds the maximum allowed.");
        }

        auto as = asset(0, sym);
        tokendb.read_asset_no_throw(ifact.address, sym, as);
        as += ifact.number;

        tokendb.update_fungible(fungible);
        tokendb.update_asset(ifact.address, as);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_transferft(apply_context& context) {
    auto tfact = context.act.data_as<transferft>();

    try {
        auto sym = tfact.number.get_symbol();
        EVT_ASSERT(context.has_authorized(N128(fungible), (fungible_name)sym.name()), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;
        
        auto facc = asset(0, sym);
        auto tacc = asset(0, sym);
        tokendb.read_asset(tfact.from, sym, facc);
        tokendb.read_asset_no_throw(tfact.to, sym, tacc);


        EVT_ASSERT(facc >= tfact.number, balance_exception, "Address does not have enough balance left.");

        bool r1, r2;
        decltype(facc.get_amount()) r;
        r1 = safemath::test_sub(facc.get_amount(), tfact.number.get_amount(), r);
        r2 = safemath::test_add(tacc.get_amount(), tfact.number.get_amount(), r);
        EVT_ASSERT(r1 && r2, math_overflow_exception, "Opeartions resulted in overflows.");
        
        facc -= tfact.number;
        tacc += tfact.number;

        tokendb.update_asset(tfact.from, facc);
        tokendb.update_asset(tfact.to, tacc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
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
            const auto& name = ref.get_group();
            group_def group;
            tokendb.read_group(name, group);
            return check_involved_node(group, group.root(), key);
        }
        }  // switch
    }
    return false;
};

auto check_involved_domain = [](const auto& tokendb, const auto& domain, auto pname, const auto& key) {
    switch(pname) {
    case N(issue): {
        return check_involved_permission(tokendb, domain.issue, key);
    }
    case N(transfer): {
        return check_involved_permission(tokendb, domain.transfer, key);
    }
    case N(manage): {
        return check_involved_permission(tokendb, domain.manage, key);
    }
    }  // switch
    return false;
};

auto check_involved_fungible = [](const auto& tokendb, const auto& fungible, auto pname, const auto& key) {
    switch(pname) {
    case N(manage): {
        return check_involved_permission(tokendb, fungible.manage, key);
    }
    }  // switch
    return false;
};

auto check_involved_group = [](const auto& group, const auto& key) {
    if(group.key() == key) {
        return true;
    }
    return false;
};

auto check_involved_owner = [](const auto& token, const auto& key) {
    if(std::find(token.owner.cbegin(), token.owner.cend(), key) != token.owner.cend()) {
        return true;
    }
    return false;
};

template<typename T>
bool
check_duplicate_meta(const T& v, const meta_key& key) {
    if(std::find_if(v.metas.cbegin(), v.metas.cend(), [&](const auto& meta) { return meta.key == key; }) != v.metas.cend()) {
        return true;
    }
    return false;
}

template<>
bool
check_duplicate_meta<group_def>(const group_def& v, const meta_key& key) {
    if(std::find_if(v.metas_.cbegin(), v.metas_.cend(), [&](const auto& meta) { return meta.key == key; }) != v.metas_.cend()) {
        return true;
    }
    return false;  
}

}  // namespace __internal

void
apply_evt_addmeta(apply_context& context) {
    using namespace __internal;

    const auto& act   = context.act;
    auto        amact = context.act.data_as<addmeta>();
    try {
        auto& tokendb = context.token_db;

        if(act.domain == N128(group)) {
            group_def group;
            tokendb.read_group(act.key, group);

            EVT_ASSERT(!check_duplicate_meta(group, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only group manager(aka. group key) can add meta
            EVT_ASSERT(check_involved_group(group, amact.creator), meta_involve_exception, "Creator is not involved in group ${name}.", ("name",act.key));

            group.metas_.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_group(group);
        }
        else if(act.domain == N128(fungible)) {
            fungible_def fungible;
            tokendb.read_fungible(act.key, fungible);

            EVT_ASSERT(!check_duplicate_meta(fungible, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only group manager(aka. group key) can add meta
            EVT_ASSERT(check_involved_fungible(tokendb, fungible, N(manage), amact.creator), meta_involve_exception, "Creator is not involved in group ${name}.", ("name",act.key));

            fungible.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_fungible(fungible);
        }
        else if(act.key == N128(.meta)) {
            domain_def domain;
            tokendb.read_domain(act.domain, domain);

            EVT_ASSERT(!check_duplicate_meta(domain, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only person involved in `manage` permission can add meta
            EVT_ASSERT(check_involved_domain(tokendb, domain, N(manage), amact.creator), meta_involve_exception, "Creator is not involved in domain ${name}.", ("name",act.key));

            domain.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_domain(domain);
        }
        else {
            token_def token;
            tokendb.read_token(act.domain, act.key, token);

            EVT_ASSERT(!check_token_destroy(token), token_destoryed_exception, "Token is already destroyed.");
            EVT_ASSERT(!check_duplicate_meta(token, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));

            domain_def domain;
            tokendb.read_domain(act.domain, domain);

            // check involved, only person involved in `issue` and `transfer` permissions or `owners` can add meta
            auto involved = check_involved_owner(token, amact.creator)
                || check_involved_domain(tokendb, domain, N(issue), amact.creator)
                || check_involved_domain(tokendb, domain, N(transfer), amact.creator);
            EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in token ${domain}-${name}.", ("domain",act.domain)("name",act.key));

            token.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_token(token);
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_newdelay(apply_context& context) {
    using namespace __internal;

    auto ndact = context.act.data_as<newdelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), ndact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;
        EVT_ASSERT(!ndact.name.empty(), proposal_name_exception, "Proposal name cannot be empty.");
        EVT_ASSERT(!tokendb.exists_delay(ndact.name), delay_exists_exception, "Delay ${name} already exists.", ("name",ndact.name));

        delay_def delay;
        delay.name     = ndact.name;
        delay.proposer = ndact.proposer;
        delay.status   = delay_status::proposed;
        delay.trx      = std::move(ndact.trx);

        tokendb.add_delay(delay);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_approvedelay(apply_context& context) {
    using namespace __internal;

    auto adact = context.act.data_as<approvedelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), adact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        delay_def delay;
        tokendb.read_delay(adact.name, delay);
        EVT_ASSERT(delay.status == delay_status::proposed, delay_status_exception, "Delay is not in proper status.");

        auto signed_keys = delay.trx.get_signature_keys(adact.signatures, context.control.get_chain_id());
        for(auto it = signed_keys.cbegin(); it != signed_keys.cend(); it++) {
            EVT_ASSERT(delay.signed_keys.find(*it) == delay.signed_keys.end(), delay_duplicate_key_exception, "Public key ${key} is already signed this delay transaction", ("key",*it)); 
        }

        delay.signatures.reserve(delay.signatures.size() + signed_keys.size());
        delay.signatures.insert(delay.signatures.end(), adact.signatures.cbegin(), adact.signatures.cend());
 
        delay.signed_keys.merge(signed_keys);
        
        tokendb.update_delay(delay);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_canceldelay(apply_context& context) {
    using namespace __internal;

    auto cdact = context.act.data_as<canceldelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), cdact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        delay_def delay;
        tokendb.read_delay(cdact.name, delay);
        EVT_ASSERT(delay.status == delay_status::proposed, delay_status_exception, "Delay is not in proper status.");

        delay.status = delay_status::cancelled;
        tokendb.update_delay(delay);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

void
apply_evt_executedelay(apply_context& context) {
    auto edact = context.act.data_as<executedelay>();
    try {
        EVT_ASSERT(context.has_authorized(N128(delay), edact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        delay_def delay;
        tokendb.read_delay(edact.name, delay);

        auto now = context.control.head_block_time();
        EVT_ASSERT(delay.status == delay_status::proposed, delay_status_exception, "Delay is not in proper status.");
        EVT_ASSERT(delay.trx.expiration > now, delay_expired_tx_exception, "Delay transaction is expired at ${expir}, now is ${now}", ("expir",delay.trx.expiration)("now",now));

        auto strx = signed_transaction(delay.trx, delay.signatures);
        auto mtrx = std::make_shared<transaction_metadata>(strx);
        auto trace = context.control.push_delay_transaction(mtrx, now);
        bool transaction_failed = trace && trace->except;
        if(transaction_failed) {
            delay.status = delay_status::failed;
            context.console_append(trace->except->to_string());
        }
        else {
            delay.status = delay_status::executed;
        }
        tokendb.update_delay(delay);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

}}} // namespace evt::chain::contracts
