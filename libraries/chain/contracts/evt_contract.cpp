/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/evt_contract.hpp>

#include <algorithm>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <eosio/chain/contracts/chain_initializer.hpp>

#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain { namespace contracts {

namespace __internal {

inline bool 
validate(const permission_def &permission) {
    uint32_t total_weight = 0;
    const contracts::group_weight* prev = nullptr;
    for(const auto& gw : permission.groups) {
        if(!prev) {
            prev = &gw;
        }
        else if(prev->id < gw.id) {
            return false;
        }
        if(gw.weight == 0) {
            return false;
        }
        total_weight += gw.weight;
    }
    return total_weight >= permission.threshold;
}

inline group_id
get_groupid(const group_key& gkey) {
    auto sha256 = fc::sha256::hash(gkey);
    auto ripemd160 = fc::ripemd160::hash(sha256);
    auto id = *(group_id*)(ripemd160.data());
    return id;
}

template<typename T>
inline bool
validate(const T &group) {
    if(group.threshold == 0) {
        return false;
    }
    uint32_t total_weight = 0;
    const key_weight* prev = nullptr;
    for(const auto& kw : group.keys) {
        if(!prev) {
            prev = &kw;
        }
        else if(prev->key < kw.key) {
            return false;
        }
        if(kw.weight == 0) {
            return false;
        }
        total_weight += kw.weight;
    }
    return total_weight >= group.threshold;
}

} // namespace __internal

void
apply_eosio_newdomain(apply_context& context) {
    using namespace __internal;

    auto ndact = context.act.data_as<newdomain>();
    try {
        auto& tokendb = context.mutable_tokendb;
        FC_ASSERT(!tokendb.exists_domain(ndact.name), "Domain ${name} already existed", ("name",ndact.name));
        FC_ASSERT(context.trx_meta.signing_keys, "[EVT] Signing keys not avaiable");
        auto& keys = *context.trx_meta.signing_keys;
        auto found = std::find(keys.cbegin(), keys.cend(), ndact.issuer);
        FC_ASSERT(found != keys.cend(), "Issuer must sign his key");

        for(auto& g : ndact.groups) {
            FC_ASSERT(validate(g), "Group ${id} is not valid, eithor threshold is not valid or exist duplicate or unordered key", ("id", g.id));
            FC_ASSERT(g.id == get_groupid(g.key), "Group id and key are not match", ("id",g.id)("key",g.key)); 
        }
        FC_ASSERT(!ndact.name.empty(), "Domain name shouldn't be empty");
        FC_ASSERT(ndact.issue.threshold > 0 && validate(ndact.issue), "Issue permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
        FC_ASSERT(ndact.transfer.threshold > 0 && validate(ndact.transfer), "Transfer permission not valid, either threshold is not valid or exist duplicate or unordered keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        FC_ASSERT(validate(ndact.manage), "Manage permission not valid, maybe exist duplicate keys.");

        auto check_groups = [&](const auto& p, auto allowed_owner) {
            for(const auto& g : p.groups) {
                if(g.id == 0) {
                    // owner group
                    FC_ASSERT(allowed_owner, "Owner group is not allowed in ${name} permission", ("name", p.name));
                    continue;
                }
                auto dbexisted = tokendb.exists_group(g.id);
                auto defexisted = std::find_if(ndact.groups.cbegin(), ndact.groups.cend(), [&g](auto& gd) { return gd.id == g.id; }) != ndact.groups.cend();

                FC_ASSERT(dbexisted && defexisted, "Group ${id} already existed shouldn't be defined again", ("id", g.id));
                FC_ASSERT(!dbexisted && !defexisted, "Group ${id} need to be defined", ("id", g.id));
            }
        };
        check_groups(ndact.issue, false);
        check_groups(ndact.transfer, true);
        check_groups(ndact.manage, false);

        domain_def domain;
        domain.name = ndact.name;
        domain.issuer = ndact.issuer;
        domain.issue_time = context.controller.head_block_time();
        domain.issue = ndact.issue;
        domain.transfer = ndact.transfer;
        domain.manage = ndact.manage;
        
        auto r = tokendb.add_domain(domain);
        FC_ASSERT(r == 0, "[EVT] Add new domain ${name} failed", ("id", domain.name));

        for(auto& g : ndact.groups) {
            auto r2 = tokendb.add_group(g);
            FC_ASSERT(r2 == 0, "[EVT] Add new group ${id} failed", ("id", g.id));
        }
        
    }
    FC_CAPTURE_AND_RETHROW((ndact)) 
}

void
apply_eosio_issuetoken(apply_context& context) {
    auto itact = context.act.data_as<issuetoken>();
    try {
        FC_ASSERT(context.mutable_tokendb.exists_domain(itact.domain), "Domain ${name} not existed", ("name", itact.domain));
        FC_ASSERT(!itact.owner.empty(), "Owner cannot be empty");

        auto& tokendb = context.mutable_tokendb;
        for(auto& n : itact.names) {
            FC_ASSERT(!tokendb.exists_token(itact.domain, n), "Token ${domain}-${name} already existed", ("domain",itact.domain)("name",n));
        }

        auto r = tokendb.issue_tokens(itact);
        FC_ASSERT(r == 0, "[EVT] Issue tokens in domain ${domain} failed", ("domain", itact.domain));
    }
    FC_CAPTURE_AND_RETHROW((itact));
}

void
apply_eosio_transfertoken(apply_context& context) {
    auto ttact = context.act.data_as<transfertoken>();
    auto& tokendb = context.mutable_tokendb;
    FC_ASSERT(tokendb.exists_token(ttact.domain, ttact.name), "Token ${domain}-${name} not existed", ("domain",ttact.domain)("name",ttact.name));
    
    auto r = tokendb.update_token(ttact.domain, ttact.name, [&ttact](auto& t) {
        t.owner = ttact.to;
    });
    FC_ASSERT(r == 0, "[EVT] Transfer token ${domain}-${name} failed", ("domain",ttact.domain)("name",ttact.name));
}

void
apply_eosio_updategroup(apply_context& context) {
    using namespace __internal;

    auto ugact = context.act.data_as<updategroup>();
    try {
        auto& tokendb = context.mutable_tokendb;
        FC_ASSERT(tokendb.exists_group(ugact.id), "Group ${id} not existed", ("id",ugact.id));
        FC_ASSERT(ugact.keys.size() > 0, "Group must contains at least one key");
        FC_ASSERT(validate(ugact), "Updated group is not valid, eithor threshold is not valid or exist duplicate or unordered keys");

        auto r = tokendb.update_group(ugact.id, [&ugact](auto& g) {
            g.threshold = ugact.threshold;
            g.keys = ugact.keys;
        });
        FC_ASSERT(r == 0, "[EVT] Update group ${id} failed", ("id",ugact.id));
    }
    FC_CAPTURE_AND_RETHROW((ugact));
}

} } } // namespace eosio::chain::contracts
