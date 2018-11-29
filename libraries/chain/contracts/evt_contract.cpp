/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_contract.hpp>

#include <algorithm>
#include <string>

#include <boost/hana.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <evt/chain/apply_context.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>
#include <evt/utilities/safemath.hpp>

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Winstantiation-after-specialization"
#endif

namespace evt { namespace chain { namespace contracts {

namespace hana = boost::hana;

#define EVT_ACTION_IMPL(name)                         \
    template<>                                        \
    struct apply_action<name> {                       \
        static void invoke(apply_context&);           \
    };                                                \
    template struct apply_action<name>;               \
                                                      \
    void                                              \
    apply_action<name>::invoke(apply_context& context)

namespace __internal {

inline bool 
validate(const permission_def& permission) {
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
                EVT_ASSERT(dbexisted, unknown_group_exception, "Group ${name} does not exist.", ("name", name));
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

inline void
check_name_reserved(const name128& name) {
    EVT_ASSERT(!name.empty() && !name.reserved(), name_reserved_exception, "Name starting with '.' is reserved for system usages.");
}

enum class reserved_meta_key {
    disable_destroy = 0
};

template<uint128_t i>
using uint128 = hana::integral_constant<uint128_t, i>;

template<uint128_t i>
constexpr uint128<i> uint128_c{};

auto domain_metas = hana::make_map(
    hana::make_pair(hana::int_c<(int)reserved_meta_key::disable_destroy>, hana::make_tuple(uint128_c<N128(.disable-destroy)>, hana::type_c<bool>))
);

template<int KeyType>
constexpr auto get_metakey = [](auto& metas) {
    return hana::at(hana::at_key(metas, hana::int_c<KeyType>), hana::int_c<0>);
};

auto get_metavalue = [](const auto& obj, auto k) {
    for(const auto& p : obj.metas) {
        if(p.key.value == k) {
            return fc::optional<std::string>{ p.value };
        }
    }
    return fc::optional<std::string>();
};

} // namespace __internal

EVT_ACTION_IMPL(newdomain) {
    using namespace __internal;

    auto ndact = context.act.data_as<newdomain>();
    try {
        EVT_ASSERT(context.has_authorized(ndact.name, N128(.create)), action_authorize_exception, "Authorized information does not match.");

        check_name_reserved(ndact.name);

        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_domain(ndact.name), domain_duplicate_exception, "Domain ${name} already exists.", ("name",ndact.name));

        EVT_ASSERT(ndact.issue.name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",ndact.issue.name));
        EVT_ASSERT(ndact.issue.threshold > 0 && validate(ndact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        EVT_ASSERT(ndact.transfer.name == "transfer", permission_type_exception, "Name ${name} does not match with the name of transfer permission.", ("name",ndact.transfer.name));
        EVT_ASSERT(validate(ndact.transfer), permission_type_exception, "Transfer permission is not valid, which may be caused by duplicated keys.");
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
        // NOTICE: we should use pending_block_time() below
        // but for historical mistakes, we use head_block_time()
        domain.create_time = context.control.head_block_time();
        domain.issue       = std::move(ndact.issue);
        domain.transfer    = std::move(ndact.transfer);
        domain.manage      = std::move(ndact.manage);
        
        tokendb.add_domain(domain);       
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(issuetoken) {
    using namespace __internal;

    auto itact = context.act.data_as<issuetoken>();
    try {
        EVT_ASSERT(context.has_authorized(itact.domain, N128(.issue)), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!itact.owner.empty(), token_owner_exception, "Owner cannot be empty.");

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_domain(itact.domain), unknown_domain_exception, "Domain ${name} does not exist.", ("name", itact.domain));

        auto check_name = [&](const auto& name) {
            check_name_reserved(name);
            EVT_ASSERT(!tokendb.exists_token(itact.domain, name), token_duplicate_exception, "Token ${domain}-${name} already exists.", ("domain",itact.domain)("name",name));
        };
        for(auto& n : itact.names) {
            check_name(n);
        }

        tokendb.issue_tokens(itact);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

namespace __internal {

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

}  // namespace __internal

EVT_ACTION_IMPL(transfer) {
    using namespace __internal;

    auto ttact = context.act.data_as<transfer>();
    try {
        EVT_ASSERT(context.has_authorized(ttact.domain, ttact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!ttact.to.empty(), token_owner_exception, "New owner cannot be empty.");

        auto& tokendb = context.token_db;

        token_def token;
        tokendb.read_token(ttact.domain, ttact.name, token);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Destroyed token cannot be transfered.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be transfered.");

        token.owner = std::move(ttact.to);
        tokendb.update_token(token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(destroytoken) {
    using namespace __internal;

    auto dtact = context.act.data_as<destroytoken>();
    try {
        EVT_ASSERT(context.has_authorized(dtact.domain, dtact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        domain_def domain;
        tokendb.read_domain(dtact.domain, domain);

        auto dd = get_metavalue(domain, get_metakey<(int)reserved_meta_key::disable_destroy>(domain_metas));
        if(dd.valid() && *dd == "true") {
            EVT_THROW(token_cannot_destroy_exception, "Token in this domain: ${d} cannot be destroyed", ("d",dtact.domain));
        }

        token_def token;
        tokendb.read_token(dtact.domain, dtact.name, token);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Token is already destroyed.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be destroyed.");

        token.owner = address_list{ address() };
        tokendb.update_token(token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(newgroup) {
    using namespace __internal;

    auto ngact = context.act.data_as<newgroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.group), ngact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!ngact.group.key().is_generated(), group_key_exception, "Group key cannot be generated key");
        EVT_ASSERT(ngact.name == ngact.group.name(), group_name_exception, "Group name not match, act: ${n1}, group: ${n2}", ("n1",ngact.name)("n2",ngact.group.name()));
        
        check_name_reserved(ngact.name);
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_group(ngact.name), group_duplicate_exception, "Group ${name} already exists.", ("name",ngact.name));
        EVT_ASSERT(validate(ngact.group), group_type_exception, "Input group is not valid.");

        tokendb.add_group(std::move(ngact.group));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(updategroup) {
    using namespace __internal;

    auto ugact = context.act.data_as<updategroup>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.group), ugact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(ugact.name == ugact.group.name(), group_name_exception, "Names in action are not the same.");

        auto& tokendb = context.token_db;
        
        group_def group;
        tokendb.read_group(ugact.name, group);
        
        EVT_ASSERT(!group.key().is_reserved(), group_key_exception, "Reserved group key cannot be used to udpate group");
        EVT_ASSERT(validate(ugact.group), group_type_exception, "Updated group is not valid.");

        tokendb.update_group(std::move(ugact.group));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(updatedomain) {
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
            EVT_ASSERT(udact.issue->threshold > 0 && validate(*udact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
            pchecker(*udact.issue, false);

            domain.issue = std::move(*udact.issue);
        }
        if(udact.transfer.valid()) {
            EVT_ASSERT(udact.transfer->name == "transfer", permission_type_exception, "Name ${name} does not match with the name of transfer permission.", ("name",udact.transfer->name));
            EVT_ASSERT(validate(*udact.transfer), permission_type_exception, "Transfer permission is not valid, which may be caused by duplicated keys.");
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

namespace __internal {

address
get_fungible_address(symbol sym) {
    return address(N(fungible), (fungible_name)std::to_string(sym.id()), 0);
}

void
transfer_fungible(asset& from, asset& to, uint64_t total) {
    bool r1, r2;
    decltype(from.amount()) r;

    r1 = safemath::test_sub(from.amount(), total, r);
    r2 = safemath::test_add(to.amount(), total, r);
    EVT_ASSERT(r1 && r2, math_overflow_exception, "Opeartions resulted in overflows.");
    
    from -= asset(total, from.sym());
    to += asset(total, to.sym());
}

}  // namespace __internal

EVT_ACTION_IMPL(newfungible) {
    using namespace __internal;

    auto nfact = context.act.data_as<newfungible>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(nfact.sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!nfact.name.empty(), fungible_name_exception, "Fungible name cannot be empty");
        EVT_ASSERT(!nfact.sym_name.empty(), fungible_symbol_exception, "Fungible symbol name cannot be empty");
        EVT_ASSERT(nfact.sym.id() > 0, fungible_symbol_exception, "Fungible symbol id should be larger than zero");
        EVT_ASSERT(nfact.total_supply.sym() == nfact.sym, fungible_symbol_exception, "Symbols in `total_supply` and `sym` are not match.");
        EVT_ASSERT(nfact.total_supply.amount() > 0, fungible_supply_exception, "Supply cannot be zero");
        EVT_ASSERT(nfact.total_supply.amount() <= asset::max_amount, fungible_supply_exception, "Supply exceeds the maximum allowed.");

        auto& tokendb = context.token_db;

        EVT_ASSERT(!tokendb.exists_fungible(nfact.sym), fungible_duplicate_exception, "Fungible with symbol id: ${s} is already existed", ("s",nfact.sym.id()));

        EVT_ASSERT(nfact.issue.name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",nfact.issue.name));
        EVT_ASSERT(nfact.issue.threshold > 0 && validate(nfact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(nfact.manage.name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",nfact.manage.name));
        EVT_ASSERT(validate(nfact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nfact.issue, false);
        pchecker(nfact.manage, false);

        fungible_def fungible;

        fungible.name           = nfact.name;
        fungible.sym_name       = nfact.sym_name;
        fungible.sym            = nfact.sym;
        fungible.creator        = nfact.creator;
        // NOTICE: we should use pending_block_time() below
        // but for historical mistakes, we use head_block_time()
        fungible.create_time    = context.control.head_block_time();
        fungible.issue          = std::move(nfact.issue);
        fungible.manage         = std::move(nfact.manage);
        fungible.total_supply   = nfact.total_supply;

        tokendb.add_fungible(fungible);

        auto addr = get_fungible_address(fungible.sym);
        tokendb.update_asset(addr, fungible.total_supply);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(updfungible) {
    using namespace __internal;

    auto ufact = context.act.data_as<updfungible>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(ufact.sym_id)), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        fungible_def fungible;
        tokendb.read_fungible(ufact.sym_id, fungible);

        auto pchecker = make_permission_checker(tokendb);
        if(ufact.issue.valid()) {
            EVT_ASSERT(ufact.issue->name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",ufact.issue->name));
            EVT_ASSERT(ufact.issue->threshold > 0 && validate(*ufact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
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

EVT_ACTION_IMPL(issuefungible) {
    using namespace __internal;

    auto ifact = context.act.data_as<issuefungible>();

    try {
        auto sym = ifact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!ifact.address.is_reserved(), fungible_address_exception, "Cannot issue fungible tokens to reserved address");

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_fungible(sym), fungible_duplicate_exception, "{sym} fungible tokens doesn't exist", ("sym",sym));

        auto addr = get_fungible_address(sym);
        EVT_ASSERT(addr != ifact.address, fungible_address_exception, "From and to are the same address");

        asset from, to;
        tokendb.read_asset(addr, sym, from);
        tokendb.read_asset_no_throw(ifact.address, sym, to);

        EVT_ASSERT(from >= ifact.number, fungible_supply_exception, "Exceeds total supply of ${sym} fungible tokens.", ("sym",sym));

        transfer_fungible(from, to, ifact.number.amount());

        tokendb.update_asset(ifact.address, to);
        tokendb.update_asset(addr, from);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(transferft) {
    using namespace __internal;

    auto tfact = context.act.data_as<const transferft&>();

    try {
        auto sym = tfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!tfact.to.is_reserved(), fungible_address_exception, "Cannot transfer fungible tokens to reserved address");
        EVT_ASSERT(tfact.from != tfact.to, fungible_address_exception, "From and to are the same address");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be transfered");

        auto& tokendb = context.token_db;
        
        auto facc = asset(0, sym);
        auto tacc = asset(0, sym);
        tokendb.read_asset(tfact.from, sym, facc);
        tokendb.read_asset_no_throw(tfact.to, sym, tacc);

        EVT_ASSERT(facc >= tfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, tfact.number.amount());

        tokendb.update_asset(tfact.to, tacc);
        tokendb.update_asset(tfact.from, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}


EVT_ACTION_IMPL(recycleft) {
    using namespace __internal;

    auto rfact = context.act.data_as<const recycleft&>();

    try {
        auto sym = rfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be recycled");

        auto& tokendb = context.token_db;

        auto addr = get_fungible_address(sym);
        auto facc = asset(0, sym);
        auto tacc = asset(0, sym);
        tokendb.read_asset(rfact.address, sym, facc);
        tokendb.read_asset_no_throw(addr, sym, tacc);

        EVT_ASSERT(facc >= rfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, rfact.number.amount());

        tokendb.update_asset(addr, tacc);
        tokendb.update_asset(rfact.address, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(destroyft) {
    using namespace __internal;

    auto rfact = context.act.data_as<const destroyft&>();

    try {
        auto sym = rfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be destroyed");

        auto& tokendb = context.token_db;

        auto addr = address();
        auto facc = asset(0, sym);
        auto tacc = asset(0, sym);
        tokendb.read_asset(rfact.address, sym, facc);
        tokendb.read_asset_no_throw(addr, sym, tacc);

        EVT_ASSERT(facc >= rfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, rfact.number.amount());

        tokendb.update_asset(addr, tacc);
        tokendb.update_asset(rfact.address, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(evt2pevt) {
    using namespace __internal;

    auto epact = context.act.data_as<evt2pevt>();

    try {
        EVT_ASSERT(epact.number.sym() == evt_sym(), fungible_symbol_exception, "Only EVT tokens can be converted to Pinned EVT tokens");
        EVT_ASSERT(context.has_authorized(N128(.fungible), (name128)std::to_string(evt_sym().id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!epact.to.is_reserved(), fungible_address_exception, "Cannot convert Pinned EVT tokens to reserved address");

        auto& tokendb = context.token_db;
        
        auto facc = asset(0, evt_sym());
        auto tacc = asset(0, pevt_sym());
        tokendb.read_asset(epact.from, evt_sym(), facc);
        tokendb.read_asset_no_throw(epact.to, pevt_sym(), tacc);

        EVT_ASSERT(facc >= epact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, epact.number.amount());

        tokendb.update_asset(epact.to, tacc);
        tokendb.update_asset(epact.from, facc);
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

auto check_involved_permission = [](const auto& tokendb, const auto& permission, const auto& creator) {
    for(auto& a : permission.authorizers) {
        auto& ref = a.ref;
        switch(ref.type()) {
        case authorizer_ref::account_t: {
            if(creator.is_account_ref() && ref.get_account() == creator.get_account()) {
                return true;
            }
            break;
        }
        case authorizer_ref::group_t: {
            const auto& name = ref.get_group();
            if(creator.is_account_ref()) {
                group_def group;
                tokendb.read_group(name, group);
                if(check_involved_node(group, group.root(), creator.get_account())) {
                    return true;
                }
            }
            else {
                if(name == creator.get_group()) {
                    return true;
                }
            }
        }
        }  // switch
    }
    return false;
};

auto check_involved_domain = [](const auto& tokendb, const auto& domain, auto pname, const auto& creator) {
    switch(pname) {
    case N(issue): {
        return check_involved_permission(tokendb, domain.issue, creator);
    }
    case N(transfer): {
        return check_involved_permission(tokendb, domain.transfer, creator);
    }
    case N(manage): {
        return check_involved_permission(tokendb, domain.manage, creator);
    }
    }  // switch
    return false;
};

auto check_involved_fungible = [](const auto& tokendb, const auto& fungible, auto pname, const auto& creator) {
    switch(pname) {
    case N(manage): {
        return check_involved_permission(tokendb, fungible.manage, creator);
    }
    }  // switch
    return false;
};

auto check_involved_group = [](const auto& group, const auto& key) {
    if(group.key().is_public_key() && group.key().get_public_key() == key) {
        return true;
    }
    return false;
};

auto check_involved_owner = [](const auto& token, const auto& key) {
    for(auto& addr : token.owner) {
        if(addr.is_public_key() && addr.get_public_key() == key) {
            return true;
        }
    }
    return false;
};

auto check_involved_creator = [](const auto& target, const auto& key) {
    return target.creator == key;
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

auto check_meta_key_reserved = [](const auto& key) {
    EVT_ASSERT(!key.reserved(), meta_key_exception, "Meta-key is reserved and cannot be used");
};

}  // namespace __internal

EVT_ACTION_IMPL(addmeta) {
    using namespace __internal;
    using namespace hana;

    const auto& act   = context.act;
    auto        amact = context.act.data_as<addmeta>();
    try {
        auto& tokendb = context.token_db;

        if(act.domain == N128(.group)) {  // group
            check_meta_key_reserved(amact.key);

            group_def group;
            tokendb.read_group(act.key, group);

            EVT_ASSERT(!check_duplicate_meta(group, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            if(amact.creator.is_group_ref()) {
                EVT_ASSERT(amact.creator.get_group() == group.name_, meta_involve_exception, "Only group itself can add its own metadata");
            }
            else {
                // check involved, only group manager(aka. group key) can add meta
                EVT_ASSERT(check_involved_group(group, amact.creator.get_account()), meta_involve_exception, "Creator is not involved in group: ${name}.", ("name",act.key));
            }
            group.metas_.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_group(group);
        }
        else if(act.domain == N128(.fungible)) {  // fungible
            check_meta_key_reserved(amact.key);

            fungible_def fungible;
            tokendb.read_fungible((symbol_id_type)std::stoul((std::string)act.key), fungible);

            EVT_ASSERT(!check_duplicate_meta(fungible, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            
            if(amact.creator.is_account_ref()) {
                // check involved, only creator or person in `manage` permission can add meta
                auto involved = check_involved_creator(fungible, amact.creator.get_account())
                    || check_involved_fungible(tokendb, fungible, N(manage), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in fungible: ${name}.", ("name",act.key));
            }
            else {
                // check involved, only group in `manage` permission can add meta
                EVT_ASSERT(check_involved_fungible(tokendb, fungible, N(manage), amact.creator), meta_involve_exception, "Creator is not involved in fungible: ${name}.", ("name",act.key));
            }
            fungible.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_fungible(fungible);
        }
        else if(act.key == N128(.meta)) {  // domain
            if(amact.key.reserved()) {
                bool pass = false;
                hana::for_each(hana::values(domain_metas), [&](const auto& m) {
                    if(amact.key.value == hana::at(m, int_c<0>)) {
                        if(hana::at(m, int_c<1>) == hana::type_c<bool>) {
                            if(amact.value == "true" || amact.value == "false") {
                                pass = true;
                            }
                            else {
                                EVT_THROW(meta_value_exception, "Meta-Value is not valid for `bool` type");
                            }
                        }
                    }
                });

                EVT_ASSERT(pass, meta_key_exception, "Meta-key is reserved and cannot be used");
            }

            domain_def domain;
            tokendb.read_domain(act.domain, domain);

            EVT_ASSERT(!check_duplicate_meta(domain, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only person involved in `manage` permission can add meta
            EVT_ASSERT(check_involved_domain(tokendb, domain, N(manage), amact.creator), meta_involve_exception, "Creator is not involved in domain: ${name}.", ("name",act.key));

            domain.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_domain(domain);
        }
        else {  // token
            check_meta_key_reserved(amact.key);

            token_def token;
            tokendb.read_token(act.domain, act.key, token);

            EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Metadata cannot be added on destroyed token.");
            EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Metadata cannot be added on locked token.");
            EVT_ASSERT(!check_duplicate_meta(token, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));

            domain_def domain;
            tokendb.read_domain(act.domain, domain);

            if(amact.creator.is_account_ref()) {
                // check involved, only person involved in `issue` and `transfer` permissions or `owners` can add meta
                auto involved = check_involved_owner(token, amact.creator.get_account())
                    || check_involved_domain(tokendb, domain, N(issue), amact.creator)
                    || check_involved_domain(tokendb, domain, N(transfer), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in token ${domain}-${name}.", ("domain",act.domain)("name",act.key));
            }
            else {
                // check involved, only group involved in `issue` and `transfer` permissions can add meta
                auto involved = check_involved_domain(tokendb, domain, N(issue), amact.creator)
                    || check_involved_domain(tokendb, domain, N(transfer), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in token ${domain}-${name}.", ("domain",act.domain)("name",act.key));
            }
            token.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            tokendb.update_token(token);
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(newsuspend) {
    using namespace __internal;

    auto nsact = context.act.data_as<newsuspend>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), nsact.name), action_authorize_exception, "Authorized information does not match.");

        auto now = context.control.pending_block_time();
        EVT_ASSERT(nsact.trx.expiration > now, suspend_expired_tx_exception, "Expiration of suspend transaction is ahead of now, expired is ${expir}, now is ${now}", ("expir",nsact.trx.expiration)("now",now));

        context.control.validate_tapos(nsact.trx);

        check_name_reserved(nsact.name);
        for(auto& act : nsact.trx.actions) {
            EVT_ASSERT(act.domain != N128(suspend), suspend_invalid_action_exception, "Actions in 'suspend' domain are not allowd deferred-signning");
            EVT_ASSERT(act.name != N(everipay), suspend_invalid_action_exception, "everiPay action is not allowd deferred-signning");
            EVT_ASSERT(act.name != N(everipass), suspend_invalid_action_exception, "everiPass action is not allowd deferred-signning");
        }

        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_suspend(nsact.name), suspend_duplicate_exception, "Suspend ${name} already exists.", ("name",nsact.name));

        suspend_def suspend;
        suspend.name     = nsact.name;
        suspend.proposer = nsact.proposer;
        suspend.status   = suspend_status::proposed;
        suspend.trx      = std::move(nsact.trx);

        tokendb.add_suspend(suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(aprvsuspend) {
    using namespace __internal;

    auto aeact = context.act.data_as<aprvsuspend>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), aeact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        tokendb.read_suspend(aeact.name, suspend);
        EVT_ASSERT(suspend.status == suspend_status::proposed, suspend_status_exception, "Suspend transaction is not in 'proposed' status.");

        auto signed_keys = suspend.trx.get_signature_keys(aeact.signatures, context.control.get_chain_id());
        auto required_keys = context.control.get_suspend_required_keys(suspend.trx, signed_keys);
        EVT_ASSERT(signed_keys == required_keys, suspend_not_required_keys_exception, "Provided keys are not required in this suspend transaction, provided keys: ${keys}", ("keys",signed_keys));
       
        for(auto it = signed_keys.cbegin(); it != signed_keys.cend(); it++) {
            EVT_ASSERT(suspend.signed_keys.find(*it) == suspend.signed_keys.end(), suspend_duplicate_key_exception, "Public key ${key} is already signed this suspend transaction", ("key",*it)); 
        }

        suspend.signed_keys.merge(signed_keys);
        suspend.signatures.insert(suspend.signatures.cend(), aeact.signatures.cbegin(), aeact.signatures.cend());
        
        tokendb.update_suspend(suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(cancelsuspend) {
    using namespace __internal;

    auto csact = context.act.data_as<cancelsuspend>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), csact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        tokendb.read_suspend(csact.name, suspend);
        EVT_ASSERT(suspend.status == suspend_status::proposed, suspend_status_exception, "Suspend transaction is not in 'proposed' status.");

        suspend.status = suspend_status::cancelled;
        tokendb.update_suspend(suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(execsuspend) {
    auto esact = context.act.data_as<execsuspend>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), esact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        tokendb.read_suspend(esact.name, suspend);

        EVT_ASSERT(suspend.signed_keys.find(esact.executor) != suspend.signed_keys.end(), suspend_executor_exception, "Executor hasn't sign his key on this suspend transaction");

        auto now = context.control.pending_block_time();
        EVT_ASSERT(suspend.status == suspend_status::proposed, suspend_status_exception, "Suspend transaction is not in 'proposed' status.");
        EVT_ASSERT(suspend.trx.expiration > now, suspend_expired_tx_exception, "Suspend transaction is expired at ${expir}, now is ${now}", ("expir",suspend.trx.expiration)("now",now));

        // instead of add signatures to transaction, check authorization and payer here
        context.control.check_authorization(suspend.signed_keys, suspend.trx);
        if(suspend.trx.payer.type() == address::public_key_t) {
            EVT_ASSERT(suspend.signed_keys.find(suspend.trx.payer.get_public_key()) != suspend.signed_keys.end(), payer_exception,
                "Payer ${pay} needs to sign this suspend transaction", ("pay",suspend.trx.payer));
        }

        auto strx = signed_transaction(suspend.trx, {});
        auto name = (std::string)esact.name;
        strx.transaction_extensions.emplace_back(std::make_pair((uint16_t)transaction_ext::suspend_name, vector<char>(name.cbegin(), name.cend())));

        auto mtrx = std::make_shared<transaction_metadata>(strx);

        auto trace = context.control.push_suspend_transaction(mtrx, fc::time_point::maximum());
        bool transaction_failed = trace && trace->except;
        if(transaction_failed) {
            suspend.status = suspend_status::failed;
            context.console_append(trace->except->to_string());
        }
        else {
            suspend.status = suspend_status::executed;
        }
        tokendb.update_suspend(suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(paycharge) {
    using namespace __internal;
    
    auto& pcact = context.act.data_as<const paycharge&>();
    try {
        auto& tokendb = context.token_db;

        asset evt, pevt;
        tokendb.read_asset_no_throw(pcact.payer, pevt_sym(), pevt);
        auto paid = std::min(pcact.charge, (uint32_t)pevt.amount());
        if(paid > 0) {
            pevt -= asset(paid, pevt_sym());
            tokendb.update_asset(pcact.payer, pevt);
        }

        if(paid < pcact.charge) {
            tokendb.read_asset_no_throw(pcact.payer, evt_sym(), evt);
            auto remain = pcact.charge - paid;
            if(evt.amount() < (int64_t)remain) {
                EVT_THROW(charge_exceeded_exception, "There are ${e} EVT and ${p} Pinned EVT left, but charge is ${c}", ("e",evt)("p",pevt)("c",pcact.charge));
            }
            evt -= asset(remain, evt_sym());
            tokendb.update_asset(pcact.payer, evt);
        }

        auto  pbs  = context.control.pending_block_state();
        auto& prod = pbs->get_scheduled_producer(pbs->header.timestamp).block_signing_key;

        asset prodasset;
        tokendb.read_asset_no_throw(prod, evt_sym(), prodasset);
        // give charge to producer
        prodasset += asset(pcact.charge, evt_sym());
        tokendb.update_asset(prod, prodasset);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(everipass) {
    using namespace __internal;

    auto& epact = context.act.data_as<const everipass&>();
    try {

        auto& tokendb = context.token_db;
        auto& db      = context.db;

        auto& link  = epact.link;
        auto  flags = link.get_header();

        EVT_ASSERT(flags & evt_link::version1, evt_link_version_exception, "EVT-Link version is not expected, current supported version is Versoin-1");
        EVT_ASSERT(flags & evt_link::everiPass, evt_link_type_exception, "Not a everiPass link");

        auto& d = *link.get_segment(evt_link::domain).strv;
        auto& t = *link.get_segment(evt_link::token).strv;

        EVT_ASSERT(context.has_authorized(name128(d), name128(t)), action_authorize_exception, "Authorized information does not match.");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(evt_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.evt_link_expired_secs) {
                EVT_THROW(evt_link_expiration_exception, "EVT-Link is expired, now: ${n}, timestamp: ${t}", ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        auto keys = link.restore_keys();

        token_def token;
        tokendb.read_token(d, t, token);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Destroyed token cannot be destroyed during everiPass.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be destroyed during everiPass.");

        if(flags & evt_link::destroy) {
            auto dt   = destroytoken();
            dt.domain = d;
            dt.name   = t;

            auto dtact = action(dt.domain, dt.name, dt);
            context.control.check_authorization(keys, dtact);

            token.owner = address_list{ address() };
            tokendb.update_token(token);
        }
        else {
            // only check owner
            EVT_ASSERT(token.owner.size() == keys.size(), everipass_exception, "Owner size and keys size don't match");
            for(auto& o : token.owner) {
                EVT_ASSERT(keys.find(o.get_public_key()) != keys.end(), everipass_exception, "Owner didn't sign");
            }
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(everipay) {
    using namespace __internal;

    auto& epact = context.act.data_as<const everipay&>();
    try {
        auto& tokendb = context.token_db;

        auto& link  = epact.link;
        auto  flags = link.get_header();

        EVT_ASSERT(flags & evt_link::version1, evt_link_version_exception, "EVT-Link version is not expected, current supported version is Versoin-1");
        EVT_ASSERT(flags & evt_link::everiPay, evt_link_type_exception, "Not a everiPay link");

        auto& lsym_id = *link.get_segment(evt_link::symbol_id).intv;
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128(std::to_string(lsym_id))), action_authorize_exception, "Authorized information does not match.");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(evt_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.evt_link_expired_secs) {
                EVT_THROW(evt_link_expiration_exception, "EVT-Link is expired, now: ${n}, timestamp: ${t}", ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        auto& link_id = link.get_link_id();
        EVT_ASSERT(!tokendb.exists_evt_link(link_id), evt_link_dupe_exception, "Duplicate EVT-Link ${id}", ("id", fc::to_hex((char*)&link_id, sizeof(link_id))));

        auto link_obj = evt_link_object {
            .link_id   = link_id,
            .block_num = context.control.pending_block_state()->block->block_num(),
            .trx_id    = context.trx_context.trx.id
        };
        tokendb.add_evt_link(link_obj);

        auto keys = link.restore_keys();
        EVT_ASSERT(keys.size() == 1, everipay_exception, "There're more than one signature on everiPay link, which is invalid");
        
        auto sym = epact.number.sym();
        EVT_ASSERT(lsym_id == sym.id(), everipay_exception, "Symbol ids don't match, provided: ${p}, expected: ${e}", ("p",lsym_id)("e",sym.id()));
        EVT_ASSERT(lsym_id != PEVT_SYM_ID, everipay_exception, "Pinned EVT cannot be used to be paid.");

        auto max_pay = uint32_t(0);
        if(link.has_segment(evt_link::max_pay)) {
            max_pay = *link.get_segment(evt_link::max_pay).intv;
        }
        else {
            max_pay = std::stoul(*link.get_segment(evt_link::max_pay_str).strv);
        }
        EVT_ASSERT(epact.number.amount() <= max_pay, everipay_exception, "Exceed max pay number: ${m}, expected: ${e}", ("m",max_pay)("e",epact.number.amount()));

        auto payer = address(*keys.begin());
        EVT_ASSERT(payer != epact.payee, everipay_exception, "Payer and payee shouldn't be the same one");

        auto facc = asset(0, sym);
        auto tacc = asset(0, sym);
        tokendb.read_asset(payer, sym, facc);
        tokendb.read_asset_no_throw(epact.payee, sym, tacc);

        EVT_ASSERT(facc >= epact.number, everipay_exception, "Payer does not have enough balance left.");

        transfer_fungible(facc, tacc, epact.number.amount());

        tokendb.update_asset(epact.payee, tacc);
        tokendb.update_asset(payer, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(prodvote) {
    auto pvact = context.act.data_as<const prodvote&>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.prodvote), pvact.key), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(pvact.value > 0 && pvact.value < 1'000'000, prodvote_value_exception, "Invalid prodvote value: ${v}", ("v",pvact.value));

        auto  conf    = context.control.get_global_properties().configuration;
        auto& sche    = context.control.active_producers();
        auto& tokendb = context.token_db;

        std::function<void(int64_t)> set_func;
        switch(pvact.key.value) {
        case N128(network-charge-factor): {
            set_func = [&](int64_t v) {
                conf.base_network_charge_factor = v;
            };
            break;
        }
        case N128(storage-charge-factor): {
            set_func = [&](int64_t v) {
                conf.base_storage_charge_factor = v;
            };
            break;
        }
        case N128(cpu-charge-factor): {
            set_func = [&](int64_t v) {
                conf.base_cpu_charge_factor = v;
            };
            break;
        }
        case N128(global-charge-factor): {
            set_func = [&](int64_t v) {
                conf.global_charge_factor = v;
            };
            break;
        }
        default: {
            EVT_THROW(prodvote_key_exception, "Configuration key: ${k} is not valid", ("k",pvact.key));
        }
        } // switch

        auto pkey = sche.get_producer_key(pvact.producer);
        EVT_ASSERT(pkey.valid(), prodvote_producer_exception, "${p} is not a valid producer", ("p",pvact.producer));

        tokendb.update_prodvote(pvact.key, *pkey, pvact.value);

        auto is_prod = [&](auto& pk) {
            for(auto& p : sche.producers) {
                if(p.block_signing_key == pk) {
                    return true;
                }
            }
            return false;
        };

        auto values = std::vector<int64_t>();
        tokendb.read_prodvotes_no_throw(pvact.key, [&](const auto& pk, auto v) {
            if(is_prod(pk)) {
                values.emplace_back(v);
            }
            return true;
        });

        if(values.size() >= ::ceil(2.0 * sche.producers.size() / 3.0)) {
            int64_t nv = 0;

            // find median
            if(values.size() % 2 == 0) {
                auto it1 = values.begin() + values.size() / 2 - 1;
                auto it2 = values.begin() + values.size() / 2;

                std::nth_element(values.begin(), it1 , values.end());
                std::nth_element(values.begin(), it2 , values.end());

                nv = ::floor((*it1 + *it2) / 2);
            }
            else {
                auto it = values.begin() + values.size() / 2;
                std::nth_element(values.begin(), it , values.end());

                nv = *it;
            }

            set_func(nv);
            context.control.set_chain_config(conf);
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(updsched) {
    auto usact = context.act.data_as<updsched>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.prodsched), N128(.update)), action_authorize_exception, "Authorized information does not match.");
        context.control.set_proposed_producers(std::move(usact.producers));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(newlock) {
    using namespace __internal;

    auto nlact = context.act.data_as<newlock>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), nlact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();
        EVT_ASSERT(!tokendb.exists_lock(nlact.name), lock_duplicate_exception, "Lock assets with same name: ${n} is already existed", ("n",nlact.name));

        auto now = context.control.pending_block_time();
        EVT_ASSERT(nlact.unlock_time > now, lock_unlock_time_exception, "Now is ahead of unlock time, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));
        EVT_ASSERT(nlact.deadline > now && nlact.deadline > nlact.unlock_time, lock_unlock_time_exception,
            "Now is ahead of unlock time or deadline, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));

        switch(nlact.condition.type()) {
        case lock_type::cond_keys: {
            auto& lck = nlact.condition.get<lock_condkeys>();
            EVT_ASSERT(lck.threshold > 0 && lck.cond_keys.size() >= lck.threshold, lock_condition_exception, "Conditional keys for lock should not be empty or threshold should not be zero");
        }    
        }  // switch
        
        EVT_ASSERT(nlact.assets.size() > 0, lock_assets_exception, "Assets for lock should not be empty");

        auto has_fungible = false;
        auto keys         = context.trx_context.trx.recover_keys(context.control.get_chain_id());
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.get<locknft_def>();
                EVT_ASSERT(tokens.names.size() > 0, lock_assets_exception, "NFT assets should be provided.");

                auto tt   = transfer();
                tt.domain = tokens.domain;
                for(auto& tn : tokens.names) {
                    tt.name = tn;

                    auto ttact = action(tt.domain, tt.name, tt);
                    context.control.check_authorization(keys, ttact);
                }
                break;
            }
            case asset_type::fungible: {
                auto& fungible = la.get<lockft_def>();

                EVT_ASSERT(fungible.amount.sym().id() != PEVT_SYM_ID, lock_assets_exception, "Pinned EVT cannot be used to be locked.");
                has_fungible = true;

                auto tf   = transferft();
                tf.from   = fungible.from;
                tf.number = fungible.amount;

                auto tfact = action(N128(.fungible), name128(std::to_string(fungible.amount.sym().id())), tf);
                context.control.check_authorization(keys, tfact);
                break;
            }
            }  // switch
        }

        if(has_fungible) {
            // because fungible assets cannot be transfer to multiple addresses.
            EVT_ASSERT(nlact.succeed.size() == 1, lock_address_exception, "Size of address for succeed situation should be only one when there's fungible assets needs to lock");
            EVT_ASSERT(nlact.failed.size() == 1, lock_address_exception, "Size of address for failed situation should be only one when there's fungible assets needs to lock");
        }
        else {
            EVT_ASSERT(nlact.succeed.size() > 0, lock_address_exception, "Size of address for succeed situation should not be empty");
            EVT_ASSERT(nlact.failed.size() > 0, lock_address_exception, "Size of address for failed situation should not be empty");
        }

        // transfer assets to lock address
        auto laddr = address(N(lock), N128(nlact.name), 0);
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.get<locknft_def>();

                for(auto& tn : tokens.names) {
                    token_def token;
                    tokendb.read_token(tokens.domain, tn, token);
                    token.owner = { laddr };

                    tokendb.update_token(token);
                }
                break;
            }
            case asset_type::fungible: {
                auto& fungible = la.get<lockft_def>();

                asset fass, tass;
                tokendb.read_asset(fungible.from, fungible.amount.sym(), fass);
                tokendb.read_asset_no_throw(laddr, fungible.amount.sym(), tass);
                
                EVT_ASSERT(fass >= fungible.amount, lock_assets_exception, "From address donn't have enough balance left.");
                transfer_fungible(fass, tass, fungible.amount.amount());

                tokendb.update_asset(fungible.from, fass);
                tokendb.update_asset(laddr, tass);
                break;
            }
            }  // switch
        }

        auto lock        = lock_def();
        lock.name        = nlact.name;
        lock.proposer    = nlact.proposer;
        lock.status      = lock_status::proposed;
        lock.unlock_time = nlact.unlock_time;
        lock.deadline    = nlact.deadline;
        lock.assets      = std::move(nlact.assets);
        lock.condition   = std::move(nlact.condition);
        lock.succeed     = std::move(nlact.succeed);
        lock.failed      = std::move(nlact.failed);

        tokendb.add_lock(lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(aprvlock) {
    using namespace __internal;

    auto alact = context.act.data_as<const aprvlock&>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), alact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();

        lock_def lock;
        tokendb.read_lock(alact.name, lock);

        auto now = context.control.pending_block_time();
        EVT_ASSERT(lock.unlock_time > now, lock_expired_exception, "Now is ahead of unlock time, cannot approve anymore, unlock time is ${u}, now is ${n}", ("u",lock.unlock_time)("n",now));

        switch(lock.condition.type()) {
        case lock_type::cond_keys: {
            EVT_ASSERT(alact.data.type() == lock_aprv_type::cond_key, lock_aprv_data_exception, "Type of approve data is not conditional key");
            auto& lck = lock.condition.get<lock_condkeys>();

            EVT_ASSERT(std::find(lck.cond_keys.cbegin(), lck.cond_keys.cend(), alact.approver) != lck.cond_keys.cend(), lock_aprv_data_exception, "Approver is not valid");
            EVT_ASSERT(lock.signed_keys.find(alact.approver) == lock.signed_keys.cend(), lock_duplicate_key_exception, "Approver is already signed this lock assets proposal");
        }
        }  // switch

        lock.signed_keys.emplace(alact.approver);
        tokendb.update_lock(lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

EVT_ACTION_IMPL(tryunlock) {
    using namespace __internal;

    auto tuact = context.act.data_as<const tryunlock&>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), tuact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();

        lock_def lock;
        tokendb.read_lock(tuact.name, lock);

        auto now = context.control.pending_block_time();
        EVT_ASSERT(lock.unlock_time < now, lock_not_reach_unlock_time, "Not reach unlock time, cannot unlock, unlock time is ${u}, now is ${n}", ("u",lock.unlock_time)("n",now));

        std::vector<address>* pkeys = nullptr;
        switch(lock.condition.type()) {
        case lock_type::cond_keys: {
            auto& lck = lock.condition.get<lock_condkeys>();
            if(lock.signed_keys.size() >= lck.threshold) {
                pkeys       = &lock.succeed;
                lock.status = lock_status::succeed;
            }
            break;
        }
        }  // switch

        if(pkeys == nullptr) {
            // not succeed
            EVT_ASSERT(lock.deadline < now, lock_not_reach_deadline, "Not reach deadline and conditions are not satisfied, proposal is still avaiable.");
            pkeys       = &lock.failed;
            lock.status = lock_status::failed;
        }

        auto laddr = address(N(lock), N128(nlact.name), 0);
        for(auto& la : lock.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.get<locknft_def>();

                token_def token;
                for(auto& tn : tokens.names) {
                    tokendb.read_token(tokens.domain, tn, token);
                    token.owner = *pkeys;
                    tokendb.update_token(token);
                }
                break;
            }
            case asset_type::fungible: {
                FC_ASSERT(pkeys->size() == 1);

                auto& fungible = la.get<lockft_def>();
                auto& toaddr   = (*pkeys)[0];

                asset fass, tass;
                tokendb.read_asset(laddr, fungible.amount.sym(), fass);
                tokendb.read_asset_no_throw(toaddr, fungible.amount.sym(), tass);
                
                EVT_ASSERT(fass >= fungible.amount, lock_assets_exception, "From address donn't have enough balance left.");
                transfer_fungible(fass, tass, fungible.amount.amount());

                tokendb.update_asset(laddr, fass);
                tokendb.update_asset(toaddr, tass);
            }
            }  // switch
        }

        tokendb.update_lock(lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}

}}} // namespace evt::chain::contracts
