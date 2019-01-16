/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <algorithm>
#include <string>
#include <variant>

#include <boost/noncopyable.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/at_key.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/tuple.hpp>

// This fixes the issue in safe_numerics in boost 1.69
#include <evt/chain/workaround/boost/safe_numerics/exception.hpp>
#include <boost/safe_numerics/checked_default.hpp>
#include <boost/safe_numerics/checked_integer.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <evt/chain/apply_context.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

namespace evt { namespace chain { namespace contracts {

#define EVT_ACTION_IMPL_BEGIN(name)                   \
    template<>                                        \
    struct apply_action<N(name)> {                    \
        template<typename ACT>                        \
        static void invoke(apply_context& context)

#define EVT_ACTION_IMPL_END() };

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
                auto dbexisted = tokendb.exists_token(token_type::group, std::nullopt, name);
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

inline void
check_address_reserved(const address& addr) {
    switch(addr.type()) {
    case address::reserved_t: {
        EVT_THROW(address_reserved_exception, "Address is reserved and cannot be used here");
    }
    case address::public_key_t: {
        return;
    }
    case address::generated_t: {
        EVT_ASSERT(!addr.get_prefix().reserved(), address_reserved_exception, "Address is reserved and cannot be used here");
    }
    }  // switch
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
            return optional<std::string>{ p.value };
        }
    }
    return optional<std::string>();
};

template<typename T>
name128
get_db_key(const T& v) {
    return v.name;
}

template<>
name128
get_db_key<group_def>(const group_def& v) {
    return v.name();
}

template<>
name128
get_db_key<fungible_def>(const fungible_def& v) {
    return v.sym.id();
}

template<>
name128
get_db_key<evt_link_object>(const evt_link_object& v) {
    return v.link_id;
}

template<typename T>
std::optional<name128>
get_db_prefix(const T& v) {
    return std::nullopt;
}

template<>
std::optional<name128>
get_db_prefix<token_def>(const token_def& v) {
    return v.domain;
}

struct db_value {
public:
    db_value(const db_value&) = default;
    db_value(db_value&&) = default;

    template<typename T>
    db_value(const T& v) : var_(std::in_place_index_t<0>()) {
        auto sz = fc::raw::pack_size(v);
        if(sz <= 1024 * 1024) {
            auto& arr = std::get<0>(var_);
            auto  ds  = fc::datastream<char*>(arr.data(), arr.size());
            fc::raw::pack(ds, v);

            view_ = std::string_view(arr.data(), sz);
        }
        else {
            auto& str = std::get<1>(var_);
            str.resize(sz);

            auto ds = fc::datastream<char*>((char*)str.data(), str.size());
            fc::raw::pack(ds, v);

            view_ = std::string_view(str.data(), sz);
        }
    }

public:
    std::string_view as_string_view() const { return view_; }

private:
    std::variant<std::array<char, 1024 * 1024>, std::string> var_;
    std::string_view                                         view_;
};

#define ADD_DB_TOKEN(TYPE, VALUE)                                                                              \
    {                                                                                                          \
        auto dv = db_value(VALUE);                                                                             \
        tokendb.put_token(TYPE, action_op::add, get_db_prefix(VALUE), get_db_key(VALUE), dv.as_string_view()); \
    }

#define UPD_DB_TOKEN(TYPE, VALUE)                                                                                 \
    {                                                                                                             \
        auto dv = db_value(VALUE);                                                                                \
        tokendb.put_token(TYPE, action_op::update, get_db_prefix(VALUE), get_db_key(VALUE), dv.as_string_view()); \
    }

#define PUT_DB_TOKEN(TYPE, VALUE)                                                                              \
    {                                                                                                          \
        auto dv = db_value(VALUE);                                                                             \
        tokendb.put_token(TYPE, action_op::put, get_db_prefix(VALUE), get_db_key(VALUE), dv.as_string_view()); \
    }

#define PUT_DB_ASSET(ADDR, VALUE)                                  \
    {                                                              \
        auto dv = db_value(VALUE);                                 \
        tokendb.put_asset(ADDR, VALUE.sym(), dv.as_string_view()); \
    }

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VALUEREF, EXCEPTION, FORMAT, ...)  \
    try {                                                                   \
        auto str = std::string();                                           \
        tokendb.read_token(TYPE, PREFIX, KEY, str);                         \
                                                                            \
        auto ds = fc::datastream<const char*>(str.data(), str.size());      \
        fc::raw::unpack(ds, VALUEREF);                                      \
    }                                                                       \
    EVT_RETHROW_EXCEPTIONS2(EXCEPTION, FORMAT, __VA_ARGS__);
    
#define READ_DB_TOKEN_NO_THROW(TYPE, PREFIX, KEY, VALUEREF)              \
    {                                                                    \
        auto str = std::string();                                        \
        tokendb.read_token(TYPE, PREFIX, KEY, str, true /* no throw */); \
                                                                         \
        auto ds = fc::datastream<const char*>(str.data(), str.size());   \
        fc::raw::unpack(ds, VALUEREF);                                   \
    }

#define READ_DB_ASSET(ADDR, SYM, VALUEREF)                                                              \
    try {                                                                                               \
        auto str = std::string();                                                                       \
        tokendb.read_asset(ADDR, SYM, str);                                                             \
                                                                                                        \
        auto ds = fc::datastream<const char*>(str.data(), str.size());                                  \
        fc::raw::unpack(ds, VALUEREF);                                                                  \
    }                                                                                                   \
    EVT_RETHROW_EXCEPTIONS2(balance_exception, "There's no balance left in {} with sym: {}", ADDR, SYM);

#define READ_DB_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                        \
    {                                                                      \
        auto str = std::string();                                          \
        if(!tokendb.read_asset(ADDR, SYM, str, true /* no throw */)) {     \
            VALUEREF = asset(0, SYM);                                      \
        }                                                                  \
        else {                                                             \
            auto ds = fc::datastream<const char*>(str.data(), str.size()); \
            fc::raw::unpack(ds, VALUEREF);                                 \
        }                                                                  \
    }

} // namespace __internal

EVT_ACTION_IMPL_BEGIN(newdomain) {
    using namespace __internal;

    auto ndact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(ndact.name, N128(.create)), action_authorize_exception, "Authorized information does not match.");

        check_name_reserved(ndact.name);

        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_token(token_type::domain, std::nullopt, ndact.name), domain_duplicate_exception, "Domain ${name} already exists.", ("name",ndact.name));

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
    using namespace __internal;

    auto itact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(itact.domain, N128(.issue)), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!itact.owner.empty(), token_owner_exception, "Owner cannot be empty.");
        for(auto& o : itact.owner) {
            check_address_reserved(o);
        }

        auto& tokendb = context.token_db;
        EVT_ASSERT2(tokendb.exists_token(token_type::domain, std::nullopt, itact.domain), unknown_domain_exception, "Cannot find domain: {}.", itact.domain);

        auto check_name = [&](const auto& name) {
            check_name_reserved(name);
            EVT_ASSERT2(!tokendb.exists_token(token_type::token, itact.domain, name), token_duplicate_exception, "Token: {} in {} is already exists.", name, itact.domain);
        };

        auto values  = small_vector<db_value, 4>();
        auto data    = small_vector<std::string_view, 4>();
        auto token   = token_def();
        token.domain = itact.domain;
        token.owner  = itact.owner;

        for(auto& n : itact.names) {
            check_name(n);

            token.name = n;
            values.emplace_back(token);
            data.emplace_back(values.back().as_string_view());
        }

        tokendb.put_tokens(token_type::token, action_op::add, itact.domain, std::move(itact.names), data);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

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

EVT_ACTION_IMPL_BEGIN(transfer) {
    using namespace __internal;

    auto ttact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(ttact.domain, ttact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!ttact.to.empty(), token_owner_exception, "New owner cannot be empty.");
        for(auto& addr : ttact.to) {
            check_address_reserved(addr);
        }

        auto& tokendb = context.token_db;

        token_def token;
        READ_DB_TOKEN(token_type::token, ttact.domain, ttact.name, token, unknown_token_exception, "Cannot find token: {} in {}", ttact.name, ttact.domain);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Destroyed token cannot be transfered.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be transfered.");

        token.owner = std::move(ttact.to);
        UPD_DB_TOKEN(token_type::token, token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(destroytoken) {
    using namespace __internal;

    auto dtact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(dtact.domain, dtact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        domain_def domain;
        READ_DB_TOKEN(token_type::domain, std::nullopt, dtact.domain, domain, unknown_domain_exception, "Cannot find domain: {}", dtact.domain);       

        auto dd = get_metavalue(domain, get_metakey<(int)reserved_meta_key::disable_destroy>(domain_metas));
        if(dd.has_value() && *dd == "true") {
            EVT_THROW(token_cannot_destroy_exception, "Token in this domain: ${d} cannot be destroyed", ("d",dtact.domain));
        }

        token_def token;
        READ_DB_TOKEN(token_type::token, dtact.domain, dtact.name, token, unknown_token_exception, "Cannot find token: {} in {}", dtact.name, dtact.domain);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Token is already destroyed.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be destroyed.");

        token.owner = address_list{ address() };
        UPD_DB_TOKEN(token_type::token, token);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(newgroup) {
    using namespace __internal;

    auto ngact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.group), ngact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!ngact.group.key().is_generated(), group_key_exception, "Group key cannot be generated key");
        EVT_ASSERT(ngact.name == ngact.group.name(), group_name_exception, "Group name not match, act: ${n1}, group: ${n2}", ("n1",ngact.name)("n2",ngact.group.name()));
        
        check_name_reserved(ngact.name);
        
        auto& tokendb = context.token_db;
        EVT_ASSERT(!tokendb.exists_token(token_type::group, std::nullopt, ngact.name), group_duplicate_exception, "Group ${name} already exists.", ("name",ngact.name));
        EVT_ASSERT(validate(ngact.group), group_type_exception, "Input group is not valid.");

        ADD_DB_TOKEN(token_type::group, ngact.group);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updategroup) {
    using namespace __internal;

    auto ugact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.group), ugact.name), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(ugact.name == ugact.group.name(), group_name_exception, "Names in action are not the same.");

        auto& tokendb = context.token_db;
        
        group_def group;
        READ_DB_TOKEN(token_type::group, std::nullopt, ugact.name, group, unknown_group_exception, "Cannot find group: {}", ugact.name);
        
        EVT_ASSERT(!group.key().is_reserved(), group_key_exception, "Reserved group key cannot be used to udpate group");
        EVT_ASSERT(validate(ugact.group), group_type_exception, "Updated group is not valid.");

        UPD_DB_TOKEN(token_type::group, ugact.group);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updatedomain) {
    using namespace __internal;

    auto udact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(udact.name, N128(.update)), action_authorize_exception, "Authorized information does not match");

        auto& tokendb = context.token_db;

        domain_def domain;
        READ_DB_TOKEN(token_type::domain, std::nullopt, udact.name, domain, unknown_domain_exception, "Cannot find domain: {}", udact.name);

        auto pchecker = make_permission_checker(tokendb);
        if(udact.issue.has_value()) {
            EVT_ASSERT(udact.issue->name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",udact.issue->name));
            EVT_ASSERT(udact.issue->threshold > 0 && validate(*udact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
            pchecker(*udact.issue, false);

            domain.issue = std::move(*udact.issue);
        }
        if(udact.transfer.has_value()) {
            EVT_ASSERT(udact.transfer->name == "transfer", permission_type_exception, "Name ${name} does not match with the name of transfer permission.", ("name",udact.transfer->name));
            EVT_ASSERT(validate(*udact.transfer), permission_type_exception, "Transfer permission is not valid, which may be caused by duplicated keys.");
            pchecker(*udact.transfer, true);

            domain.transfer = std::move(*udact.transfer);
        }
        if(udact.manage.has_value()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(udact.manage->name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",udact.manage->name));
            EVT_ASSERT(validate(*udact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*udact.manage, false);

            domain.manage = std::move(*udact.manage);
        }

        UPD_DB_TOKEN(token_type::domain, domain);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

namespace __internal {

address
get_fungible_address(symbol sym) {
    return address(N(.fungible), name128::from_number(sym.id()), 0);
}

void
transfer_fungible(asset& from, asset& to, uint64_t total) {
    using namespace boost::safe_numerics;

    auto r1 = checked::subtract<uint64_t>(from.amount(), total);
    auto r2 = checked::add<uint64_t>(to.amount(), total);
    EVT_ASSERT(!r1.exception() && !r2.exception(), math_overflow_exception, "Opeartions resulted in overflows.");
    
    from -= asset(total, from.sym());
    to += asset(total, to.sym());
}

}  // namespace __internal

EVT_ACTION_IMPL_BEGIN(newfungible) {
    using namespace __internal;

    auto nfact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(nfact.sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(!nfact.name.empty(), fungible_name_exception, "Fungible name cannot be empty");
        EVT_ASSERT(!nfact.sym_name.empty(), fungible_symbol_exception, "Fungible symbol name cannot be empty");
        EVT_ASSERT(nfact.sym.id() > 0, fungible_symbol_exception, "Fungible symbol id should be larger than zero");
        EVT_ASSERT(nfact.total_supply.sym() == nfact.sym, fungible_symbol_exception, "Symbols in `total_supply` and `sym` are not match.");
        EVT_ASSERT(nfact.total_supply.amount() > 0, fungible_supply_exception, "Supply cannot be zero");
        EVT_ASSERT(nfact.total_supply.amount() <= asset::max_amount, fungible_supply_exception, "Supply exceeds the maximum allowed.");

        auto& tokendb = context.token_db;

        EVT_ASSERT(!tokendb.exists_token(token_type::fungible, std::nullopt, nfact.sym.id()), fungible_duplicate_exception, "Fungible with symbol id: ${s} is already existed", ("s",nfact.sym.id()));

        EVT_ASSERT(nfact.issue.name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",nfact.issue.name));
        EVT_ASSERT(nfact.issue.threshold > 0 && validate(nfact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
        // manage permission's threshold can be 0 which means no one can update permission later.
        EVT_ASSERT(nfact.manage.name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",nfact.manage.name));
        EVT_ASSERT(validate(nfact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");

        auto pchecker = make_permission_checker(tokendb);
        pchecker(nfact.issue, false);
        pchecker(nfact.manage, false);

        auto fungible = fungible_def();
        
        fungible.name         = nfact.name;
        fungible.sym_name     = nfact.sym_name;
        fungible.sym          = nfact.sym;
        fungible.creator      = nfact.creator;
        // NOTICE: we should use pending_block_time() below
        // but for historical mistakes, we use head_block_time()
        fungible.create_time  = context.control.head_block_time();
        fungible.issue        = std::move(nfact.issue);
        fungible.manage       = std::move(nfact.manage);
        fungible.total_supply = nfact.total_supply;

        ADD_DB_TOKEN(token_type::fungible, fungible);

        auto addr = get_fungible_address(fungible.sym);
        PUT_DB_ASSET(addr, fungible.total_supply);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updfungible) {
    using namespace __internal;

    auto ufact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(ufact.sym_id)), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        fungible_def fungible;
        READ_DB_TOKEN(token_type::fungible, std::nullopt, ufact.sym_id, fungible, unknown_fungible_exception, "Cannot find fungible with sym id: {}", ufact.sym_id);

        auto pchecker = make_permission_checker(tokendb);
        if(ufact.issue.has_value()) {
            EVT_ASSERT(ufact.issue->name == "issue", permission_type_exception, "Name ${name} does not match with the name of issue permission.", ("name",ufact.issue->name));
            EVT_ASSERT(ufact.issue->threshold > 0 && validate(*ufact.issue), permission_type_exception, "Issue permission is not valid, which may be caused by invalid threshold, duplicated keys.");
            pchecker(*ufact.issue, false);

            fungible.issue = std::move(*ufact.issue);
        }
        if(ufact.manage.has_value()) {
            // manage permission's threshold can be 0 which means no one can update permission later.
            EVT_ASSERT(ufact.manage->name == "manage", permission_type_exception, "Name ${name} does not match with the name of manage permission.", ("name",ufact.manage->name));
            EVT_ASSERT(validate(*ufact.manage), permission_type_exception, "Manage permission is not valid, which may be caused by duplicated keys.");
            pchecker(*ufact.manage, false);

            fungible.manage = std::move(*ufact.manage);
        }

        UPD_DB_TOKEN(token_type::fungible, fungible);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(issuefungible) {
    using namespace __internal;

    auto& ifact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = ifact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception, "Authorized information does not match.");
        check_address_reserved(ifact.address);

        auto& tokendb = context.token_db;
        EVT_ASSERT(tokendb.exists_token(token_type::fungible, std::nullopt, sym.id()), fungible_duplicate_exception, "{sym} fungible tokens doesn't exist", ("sym",sym));

        auto addr = get_fungible_address(sym);
        EVT_ASSERT(addr != ifact.address, fungible_address_exception, "From and to are the same address");

        asset from, to;
        READ_DB_ASSET(addr, sym, from);
        READ_DB_ASSET_NO_THROW(ifact.address, sym, to);

        EVT_ASSERT(from >= ifact.number, fungible_supply_exception, "Exceeds total supply of ${sym} fungible tokens.", ("sym",sym));

        transfer_fungible(from, to, ifact.number.amount());

        PUT_DB_ASSET(ifact.address, to);
        PUT_DB_ASSET(addr, from);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(transferft) {
    using namespace __internal;

    auto& tfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = tfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(tfact.from != tfact.to, fungible_address_exception, "From and to are the same address");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be transfered");
        check_address_reserved(tfact.to);

        auto& tokendb = context.token_db;
        
        asset facc, tacc;
        READ_DB_ASSET(tfact.from, sym, facc);
        READ_DB_ASSET_NO_THROW(tfact.to, sym, tacc);

        EVT_ASSERT(facc >= tfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, tfact.number.amount());

        PUT_DB_ASSET(tfact.to, tacc);
        PUT_DB_ASSET(tfact.from, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(recycleft) {
    using namespace __internal;

    auto& rfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = rfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be recycled");

        auto& tokendb = context.token_db;

        auto addr = get_fungible_address(sym);
        
        asset facc, tacc;
        READ_DB_ASSET(rfact.address, sym, facc);
        READ_DB_ASSET_NO_THROW(addr, sym, tacc);

        EVT_ASSERT(facc >= rfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, rfact.number.amount());

        PUT_DB_ASSET(addr, tacc);
        PUT_DB_ASSET(rfact.address, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(destroyft) {
    using namespace __internal;

    auto& rfact = context.act.data_as<add_clr_t<ACT>>();

    try {
        auto sym = rfact.number.sym();
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(sym.id())), action_authorize_exception, "Authorized information does not match.");
        EVT_ASSERT(sym != pevt_sym(), fungible_symbol_exception, "Pinned EVT cannot be destroyed");

        auto& tokendb = context.token_db;

        auto addr = address();

        asset facc, tacc;
        READ_DB_ASSET(rfact.address, sym, facc);
        READ_DB_ASSET_NO_THROW(addr, sym, tacc);

        EVT_ASSERT(facc >= rfact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, rfact.number.amount());

        PUT_DB_ASSET(addr, tacc);
        PUT_DB_ASSET(rfact.address, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(evt2pevt) {
    using namespace __internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();

    try {
        EVT_ASSERT(epact.number.sym() == evt_sym(), fungible_symbol_exception, "Only EVT tokens can be converted to Pinned EVT tokens");
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(evt_sym().id())), action_authorize_exception, "Authorized information does not match.");
        check_address_reserved(epact.to);

        auto& tokendb = context.token_db;
        
        asset facc, tacc;
        READ_DB_ASSET(epact.from, evt_sym(), facc);
        READ_DB_ASSET_NO_THROW(epact.to, pevt_sym(), tacc);

        EVT_ASSERT(facc >= epact.number, balance_exception, "Address does not have enough balance left.");

        transfer_fungible(facc, tacc, epact.number.amount());

        PUT_DB_ASSET(epact.to, tacc);
        PUT_DB_ASSET(epact.from, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

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
                READ_DB_TOKEN(token_type::group, std::nullopt, name, group, unknown_group_exception, "Cannot find group: {}", name);
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

EVT_ACTION_IMPL_BEGIN(addmeta) {
    using namespace __internal;
    using namespace hana;

    const auto& act   = context.act;
    auto&       amact = context.act.data_as<add_clr_t<ACT>>();
    try {
        auto& tokendb = context.token_db;

        if(act.domain == N128(.group)) {  // group
            check_meta_key_reserved(amact.key);

            group_def group;
            READ_DB_TOKEN(token_type::group, std::nullopt, act.key, group, unknown_group_exception, "Cannot find group: {}", act.key);

            EVT_ASSERT(!check_duplicate_meta(group, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            if(amact.creator.is_group_ref()) {
                EVT_ASSERT(amact.creator.get_group() == group.name_, meta_involve_exception, "Only group itself can add its own metadata");
            }
            else {
                // check involved, only group manager(aka. group key) can add meta
                EVT_ASSERT(check_involved_group(group, amact.creator.get_account()), meta_involve_exception, "Creator is not involved in group: ${name}.", ("name",act.key));
            }
            group.metas_.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::group, group);
        }
        else if(act.domain == N128(.fungible)) {  // fungible
            check_meta_key_reserved(amact.key);

            fungible_def fungible;
            READ_DB_TOKEN(token_type::fungible, std::nullopt, (symbol_id_type)std::stoul((std::string)act.key), fungible, unknown_fungible_exception, "Cannot find fungible with symbol id: {}", act.key);

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
            UPD_DB_TOKEN(token_type::fungible, fungible);
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
            READ_DB_TOKEN(token_type::domain, std::nullopt, amact.key, domain, unknown_domain_exception, "Cannot find domain: {}", amact.key);

            EVT_ASSERT(!check_duplicate_meta(domain, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only person involved in `manage` permission can add meta
            EVT_ASSERT(check_involved_domain(tokendb, domain, N(manage), amact.creator), meta_involve_exception, "Creator is not involved in domain: ${name}.", ("name",act.key));

            domain.metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::domain, domain);
        }
        else {  // token
            check_meta_key_reserved(amact.key);

            token_def token;
            READ_DB_TOKEN(token_type::token, act.domain, act.key, token, unknown_token_exception, "Cannot find token: {} in {}", act.key, act.domain);

            EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Metadata cannot be added on destroyed token.");
            EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Metadata cannot be added on locked token.");
            EVT_ASSERT(!check_duplicate_meta(token, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));

            domain_def domain;
            READ_DB_TOKEN(token_type::domain, std::nullopt, act.domain, domain, unknown_domain_exception, "Cannot find domain: {}", amact.key);

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
            UPD_DB_TOKEN(token_type::token, token);
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(newsuspend) {
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
        EVT_ASSERT(!tokendb.exists_token(token_type::suspend, std::nullopt, nsact.name), suspend_duplicate_exception, "Suspend ${name} already exists.", ("name",nsact.name));

        auto suspend     = suspend_def();
        suspend.name     = nsact.name;
        suspend.proposer = nsact.proposer;
        suspend.status   = suspend_status::proposed;
        suspend.trx      = std::move(nsact.trx);

        PUT_DB_TOKEN(token_type::suspend, suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(aprvsuspend) {
    using namespace __internal;

    auto& aeact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), aeact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        READ_DB_TOKEN(token_type::suspend, std::nullopt, aeact.name, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", aeact.name);

        EVT_ASSERT(suspend.status == suspend_status::proposed, suspend_status_exception, "Suspend transaction is not in 'proposed' status.");

        auto signed_keys = suspend.trx.get_signature_keys(aeact.signatures, context.control.get_chain_id());
        auto required_keys = context.control.get_suspend_required_keys(suspend.trx, signed_keys);
        EVT_ASSERT(signed_keys == required_keys, suspend_not_required_keys_exception, "Provided keys are not required in this suspend transaction, provided keys: ${keys}", ("keys",signed_keys));
       
        for(auto it = signed_keys.cbegin(); it != signed_keys.cend(); it++) {
            EVT_ASSERT(suspend.signed_keys.find(*it) == suspend.signed_keys.end(), suspend_duplicate_key_exception, "Public key ${key} is already signed this suspend transaction", ("key",*it)); 
        }

        suspend.signed_keys.merge(signed_keys);
        suspend.signatures.insert(suspend.signatures.cend(), aeact.signatures.cbegin(), aeact.signatures.cend());
        
        UPD_DB_TOKEN(token_type::suspend, suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(cancelsuspend) {
    using namespace __internal;

    auto& csact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), csact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        READ_DB_TOKEN(token_type::suspend, std::nullopt, csact.name, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", csact.name);
        
        EVT_ASSERT(suspend.status == suspend_status::proposed, suspend_status_exception, "Suspend transaction is not in 'proposed' status.");
        suspend.status = suspend_status::cancelled;

        UPD_DB_TOKEN(token_type::suspend, suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(execsuspend) {
    using namespace __internal;

    auto& esact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.suspend), esact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.token_db;

        suspend_def suspend;
        READ_DB_TOKEN(token_type::suspend, std::nullopt, esact.name, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", esact.name);

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
            fmt::format_to(context.get_console_buffer(), "{}", trace->except->to_string());
        }
        else {
            suspend.status = suspend_status::executed;
        }
        UPD_DB_TOKEN(token_type::suspend, suspend);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(paycharge) {
    using namespace __internal;
    
    auto& pcact = context.act.data_as<add_clr_t<ACT>>();
    try {
        auto& tokendb = context.token_db;

        asset evt, pevt;
        READ_DB_ASSET_NO_THROW(pcact.payer, pevt_sym(), pevt);
        auto paid = std::min(pcact.charge, (uint32_t)pevt.amount());
        if(paid > 0) {
            pevt -= asset(paid, pevt_sym());
            PUT_DB_ASSET(pcact.payer, pevt);
        }

        if(paid < pcact.charge) {
            READ_DB_ASSET_NO_THROW(pcact.payer, evt_sym(), evt);
            auto remain = pcact.charge - paid;
            if(evt.amount() < (int64_t)remain) {
                EVT_THROW(charge_exceeded_exception, "There are ${e} EVT and ${p} Pinned EVT left, but charge is ${c}", ("e",evt)("p",pevt)("c",pcact.charge));
            }
            evt -= asset(remain, evt_sym());
            PUT_DB_ASSET(pcact.payer, evt);
        }

        auto  pbs  = context.control.pending_block_state();
        auto& prod = pbs->get_scheduled_producer(pbs->header.timestamp).block_signing_key;

        asset prodasset;
        READ_DB_ASSET_NO_THROW(prod, evt_sym(), prodasset);
        // give charge to producer
        prodasset += asset(pcact.charge, evt_sym());
        PUT_DB_ASSET(prod, prodasset);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(everipass) {
    using namespace __internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
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
        READ_DB_TOKEN(token_type::token, d, t, token, unknown_token_exception, "Cannot find token: {} in {}", t, d);

        EVT_ASSERT(!check_token_destroy(token), token_destroyed_exception, "Destroyed token cannot be destroyed during everiPass.");
        EVT_ASSERT(!check_token_locked(token), token_locked_exception, "Locked token cannot be destroyed during everiPass.");

        if(flags & evt_link::destroy) {
            auto dt   = destroytoken();
            dt.domain = d;
            dt.name   = t;

            auto dtact = action(dt.domain, dt.name, dt);
            context.control.check_authorization(keys, dtact);

            token.owner = address_list{ address() };
            UPD_DB_TOKEN(token_type::token, token);
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
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(everipay) {
    using namespace __internal;

    auto& epact = context.act.data_as<add_clr_t<ACT>>();
    try {
        check_address_reserved(epact.payee);

        auto& tokendb = context.token_db;

        auto& link  = epact.link;
        auto  flags = link.get_header();

        EVT_ASSERT(flags & evt_link::version1, evt_link_version_exception, "EVT-Link version is not expected, current supported version is Versoin-1");
        EVT_ASSERT(flags & evt_link::everiPay, evt_link_type_exception, "Not a everiPay link");

        auto& lsym_id = *link.get_segment(evt_link::symbol_id).intv;
        EVT_ASSERT(context.has_authorized(N128(.fungible), name128::from_number(lsym_id)), action_authorize_exception, "Authorized information does not match.");

        if(!context.control.loadtest_mode()) {
            auto  ts    = *link.get_segment(evt_link::timestamp).intv;
            auto  since = std::abs((context.control.pending_block_time() - fc::time_point_sec(ts)).to_seconds());
            auto& conf  = context.control.get_global_properties().configuration;
            if(since > conf.evt_link_expired_secs) {
                EVT_THROW(evt_link_expiration_exception, "EVT-Link is expired, now: ${n}, timestamp: ${t}", ("n",context.control.pending_block_time())("t",fc::time_point_sec(ts)));
            }
        }

        auto& link_id = link.get_link_id();
        EVT_ASSERT(!tokendb.exists_token(token_type::evtlink, std::nullopt, link_id), evt_link_dupe_exception, "Duplicate EVT-Link ${id}", ("id", fc::to_hex((char*)&link_id, sizeof(link_id))));

        auto link_obj = evt_link_object {
            .link_id   = link_id,
            .block_num = context.control.pending_block_state()->block->block_num(),
            .trx_id    = context.trx_context.trx.id
        };
        ADD_DB_TOKEN(token_type::evtlink, link_obj);

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

        asset facc, tacc;
        READ_DB_ASSET(payer, sym, facc);
        READ_DB_ASSET_NO_THROW(epact.payee, sym, tacc);

        EVT_ASSERT(facc >= epact.number, everipay_exception, "Payer does not have enough balance left.");

        transfer_fungible(facc, tacc, epact.number.amount());

        PUT_DB_ASSET(epact.payee, tacc);
        PUT_DB_ASSET(payer, facc);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(prodvote) {
    using namespace __internal;

    auto& pvact = context.act.data_as<add_clr_t<ACT>>();
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
        EVT_ASSERT(pkey.has_value(), prodvote_producer_exception, "${p} is not a valid producer", ("p",pvact.producer));

        auto map = flat_map<public_key_type, int64_t>();
        READ_DB_TOKEN_NO_THROW(token_type::prodvote, std::nullopt, pvact.key, map);

        auto it = map.emplace(*pkey, pvact.value);
        if(it.second == false) {
            // existed
            it.first->second = pvact.value;
        }

        auto dv = db_value(map);
        tokendb.put_token(token_type::prodvote, action_op::put, std::nullopt, pvact.key, dv.as_string_view());

        auto is_prod = [&](auto& pk) {
            for(auto& p : sche.producers) {
                if(p.block_signing_key == pk) {
                    return true;
                }
            }
            return false;
        };

        auto values = std::vector<int64_t>();
        for(auto& it : map) {
            if(is_prod(it.first)) {
                values.emplace_back(it.second);
            }
        }

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
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updsched) {
    auto usact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.prodsched), N128(.update)), action_authorize_exception, "Authorized information does not match.");
        context.control.set_proposed_producers(std::move(usact.producers));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(newlock) {
    using namespace __internal;

    auto nlact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), nlact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();
        EVT_ASSERT(!tokendb.exists_token(token_type::lock, std::nullopt, nlact.name), lock_duplicate_exception, "Lock assets with same name: ${n} is already existed", ("n",nlact.name));

        auto now = context.control.pending_block_time();
        EVT_ASSERT(nlact.unlock_time > now, lock_unlock_time_exception, "Now is ahead of unlock time, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));
        EVT_ASSERT(nlact.deadline > now && nlact.deadline > nlact.unlock_time, lock_unlock_time_exception,
            "Now is ahead of unlock time or deadline, unlock time is ${u}, now is ${n}", ("u",nlact.unlock_time)("n",now));

        // check condition
        switch(nlact.condition.type()) {
        case lock_type::cond_keys: {
            auto& lck = nlact.condition.template get<lock_condkeys>();
            EVT_ASSERT(lck.threshold > 0 && lck.cond_keys.size() >= lck.threshold, lock_condition_exception, "Conditional keys for lock should not be empty or threshold should not be zero");
        }    
        }  // switch
        
        // check succeed and fiil addresses(shouldn't be reserved)
        for(auto& addr : nlact.succeed) {
            check_address_reserved(addr);
        }
        for(auto& addr : nlact.failed) {
            check_address_reserved(addr);
        }

        // check assets(need to has authority)
        EVT_ASSERT(nlact.assets.size() > 0, lock_assets_exception, "Assets for lock should not be empty");

        auto has_fungible = false;
        auto keys         = context.trx_context.trx.recover_keys(context.control.get_chain_id());
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.template get<locknft_def>();
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
                auto& fungible = la.template get<lockft_def>();

                EVT_ASSERT(fungible.amount.sym().id() != PEVT_SYM_ID, lock_assets_exception, "Pinned EVT cannot be used to be locked.");
                has_fungible = true;

                auto tf   = transferft();
                tf.from   = fungible.from;
                tf.number = fungible.amount;

                auto tfact = action(N128(.fungible), name128::from_number(fungible.amount.sym().id()), tf);
                context.control.check_authorization(keys, tfact);
                break;
            }
            }  // switch
        }

        // check succeed and fail addresses(size should be match)
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
        auto laddr = address(N(.lock), N128(nlact.name), 0);
        for(auto& la : nlact.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.template get<locknft_def>();

                for(auto& tn : tokens.names) {
                    token_def token;
                    READ_DB_TOKEN(token_type::token, tokens.domain, tn, token, unknown_token_exception, "Cannot find token: {} in {}", tn, tokens.domain);
                    token.owner = { laddr };

                    UPD_DB_TOKEN(token_type::token, token);
                }
                break;
            }
            case asset_type::fungible: {
                auto& fungible = la.template get<lockft_def>();

                asset fass, tass;
                READ_DB_ASSET(fungible.from, fungible.amount.sym(), fass);
                READ_DB_ASSET_NO_THROW(laddr, fungible.amount.sym(), tass);
                
                EVT_ASSERT(fass >= fungible.amount, lock_assets_exception, "From address donn't have enough balance left.");
                transfer_fungible(fass, tass, fungible.amount.amount());

                PUT_DB_ASSET(fungible.from, fass);
                PUT_DB_ASSET(laddr, tass);
                break;
            }
            }  // switch
        }

        // add locl proposal to token database
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

        ADD_DB_TOKEN(token_type::lock, lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(aprvlock) {
    using namespace __internal;

    auto& alact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), alact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();

        lock_def lock;
        READ_DB_TOKEN(token_type::lock, std::nullopt, alact.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", alact.name);

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
        UPD_DB_TOKEN(token_type::lock, lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(tryunlock) {
    using namespace __internal;

    auto& tuact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.lock), tuact.name), action_authorize_exception, "Authorized information does not match.");

        auto& tokendb = context.control.token_db();

        lock_def lock;
        READ_DB_TOKEN(token_type::lock, std::nullopt, tuact.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", tuact.name);

        auto now = context.control.pending_block_time();
        EVT_ASSERT(lock.unlock_time < now, lock_not_reach_unlock_time, "Not reach unlock time, cannot unlock, unlock time is ${u}, now is ${n}", ("u",lock.unlock_time)("n",now));

        // std::add_pointer_t<decltype(lock.succeed)> pkeys = nullptr;
        fc::small_vector_base<address>* pkeys = nullptr;
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

        auto laddr = address(N(.lock), N128(nlact.name), 0);
        for(auto& la : lock.assets) {
            switch(la.type()) {
            case asset_type::tokens: {
                auto& tokens = la.get<locknft_def>();

                token_def token;
                for(auto& tn : tokens.names) {
                    READ_DB_TOKEN(token_type::token, tokens.domain, tn, token, unknown_token_exception, "Cannot find token: {} in {}", tn, tokens.domain);
                    token.owner = *pkeys;
                    UPD_DB_TOKEN(token_type::token, token);
                }
                break;
            }
            case asset_type::fungible: {
                FC_ASSERT(pkeys->size() == 1);

                auto& fungible = la.get<lockft_def>();
                auto& toaddr   = (*pkeys)[0];

                asset fass, tass;
                READ_DB_ASSET(laddr, fungible.amount.sym(), fass);
                READ_DB_ASSET_NO_THROW(toaddr, fungible.amount.sym(), tass);
                
                EVT_ASSERT(fass >= fungible.amount, lock_assets_exception, "From address donn't have enough balance left.");
                transfer_fungible(fass, tass, fungible.amount.amount());

                PUT_DB_ASSET(laddr, fass);
                PUT_DB_ASSET(toaddr, tass);
            }
            }  // switch
        }

        UPD_DB_TOKEN(token_type::lock, lock);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
