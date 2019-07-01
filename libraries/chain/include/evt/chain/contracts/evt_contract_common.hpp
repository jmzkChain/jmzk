/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <algorithm>
#include <string>
#include <variant>

#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>

// This fixes the issue in safe_numerics in boost 1.69
#include <evt/chain/workaround/boost/safe_numerics/exception.hpp>
#include <boost/safe_numerics/checked_default.hpp>
#include <boost/safe_numerics/checked_integer.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/city.hpp>

#include <evt/chain/apply_context.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/token_database_cache.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/dense_hash.hpp>
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

#define EVT_ACTION_VER() ACT::get_version()

namespace internal {

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
                EVT_ASSERT(allowed_owner, permission_type_exception,
                    "Owner group does not show up in ${name} permission, and it only appears in Transfer.", ("name", p.name));
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
        auto p = addr.get_prefix();
        if(p.reserved()) {
            if(p == N(.domain) || p == N(.fungible) || p == N(.group)) {
                return;
            }
        }
        EVT_THROW(address_reserved_exception, "Address is reserved and cannot be used here");
    }
    }  // switch
}

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

enum psvbonus_type { kPsvBonus = 0, kPsvBonusSlim };

name128
get_psvbonus_db_key(symbol_id_type id, uint64_t nonce) {
    uint128_t v = nonce;
    v |= ((uint128_t)id << 64);
    return v;
}

template<>
name128
get_db_key<passive_bonus>(const passive_bonus& pb) {
    return get_psvbonus_db_key(pb.sym_id, kPsvBonus);
}

template<>
name128
get_db_key<passive_bonus_slim>(const passive_bonus_slim& pbs) {
    return get_psvbonus_db_key(pbs.sym_id, kPsvBonusSlim);
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

#define ADD_DB_TOKEN(TYPE, VALUE)                                                                      \
    {                                                                                                  \
        tokendb_cache.put_token(TYPE, action_op::add, get_db_prefix(VALUE), get_db_key(VALUE), VALUE); \
    }

#define UPD_DB_TOKEN(TYPE, VALUE)                                                                         \
    {                                                                                                     \
        tokendb_cache.put_token(TYPE, action_op::update, get_db_prefix(VALUE), get_db_key(VALUE), VALUE); \
    }

#define PUT_DB_TOKEN(TYPE, VALUE)                                                                      \
    {                                                                                                  \
        tokendb_cache.put_token(TYPE, action_op::put, get_db_prefix(VALUE), get_db_key(VALUE), VALUE); \
    }

#define PUT_DB_ASSET(ADDR, VALUE)                                             \
    while(1) {                                                                \
        if(VALUE.sym.id() == EVT_SYM_ID) {                                    \
            if constexpr(std::is_same_v<decltype(VALUE), property>) {         \
                auto ps = property_stakes(VALUE);                             \
                auto dv = make_db_value(ps);                                  \
                tokendb.put_asset(ADDR, VALUE.sym.id(), dv.as_string_view()); \
                break;                                                        \
            }                                                                 \
        }                                                                     \
        auto dv = make_db_value(VALUE);                                       \
        tokendb.put_asset(ADDR, VALUE.sym.id(), dv.as_string_view());         \
        break;                                                                \
    }

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...)      \
    try {                                                                   \
        using vtype = typename decltype(VPTR)::element_type;                \
        VPTR = tokendb_cache.template read_token<vtype>(TYPE, PREFIX, KEY); \
    }                                                                       \
    catch(token_database_exception&) {                                      \
        EVT_THROW2(EXCEPTION, FORMAT, __VA_ARGS__);                         \
    }
    
#define READ_DB_TOKEN_NO_THROW(TYPE, PREFIX, KEY, VPTR)                                          \
    {                                                                                            \
        using vtype = typename decltype(VPTR)::element_type;                                     \
        VPTR = tokendb_cache.template read_token<vtype>(TYPE, PREFIX, KEY, true /* no throw */); \
    }

#define MAKE_PROPERTY(AMOUNT, SYM)                                            \
    property {                                                                \
        .amount = AMOUNT,                                                     \
        .sym = SYM,                                                           \
        .created_at = context.control.pending_block_time().sec_since_epoch(), \
        .created_index = context.get_index_of_trx()                           \
    }

#define MAKE_PROPERTY_STAKES(AMOUNT, SYM)                                     \
    property_stakes(property {                                                \
        .amount = AMOUNT,                                                     \
        .sym = SYM,                                                           \
        .created_at = context.control.pending_block_time().sec_since_epoch(), \
        .created_index = context.get_index_of_trx(),                          \
    })

#define CHECK_SYM(VALUEREF, PROVIDED) \
    EVT_ASSERT2(VALUEREF.sym == PROVIDED, asset_symbol_exception, "Provided symbol({}) is invalid, expected: {}", PROVIDED, VALUEREF.sym);

#define READ_DB_ASSET(ADDR, SYM, VALUEREF)                                                              \
    try {                                                                                               \
        auto str = std::string();                                                                       \
        tokendb.read_asset(ADDR, SYM.id(), str);                                                        \
                                                                                                        \
        extract_db_value(str, VALUEREF);                                                                \
    }                                                                                                   \
    catch(token_database_exception&) {                                                                  \
        EVT_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM.id()); \
    }                                                                                                   \
    CHECK_SYM(VALUEREF, SYM);

#define READ_DB_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                         \
    {                                                                       \
        auto str = std::string();                                           \
        if(!tokendb.read_asset(ADDR, SYM.id(), str, true /* no throw */)) { \
            if constexpr(std::is_same_v<decltype(VALUEREF), property>) {    \
                VALUEREF = MAKE_PROPERTY(0, SYM);                           \
            }                                                               \
            else {                                                          \
                VALUEREF = MAKE_PROPERTY_STAKES(0, SYM);                    \
            }                                                               \
            context.add_new_ft_holder(                                      \
                ft_holder { .addr = ADDR, .sym_id = SYM.id() });            \
        }                                                                   \
        else {                                                              \
            extract_db_value(str, VALUEREF);                                \
            CHECK_SYM(VALUEREF, SYM);                                       \
        }                                                                   \
    }

#define READ_DB_ASSET_NO_THROW_NO_NEW(ADDR, SYM, VALUEREF)                  \
    {                                                                       \
        auto str = std::string();                                           \
        if(!tokendb.read_asset(ADDR, SYM.id(), str, true /* no throw */)) { \
            if constexpr(std::is_same_v<decltype(VALUEREF), property>){     \
                VALUEREF = MAKE_PROPERTY(0, SYM);                           \
            }                                                               \
            else {                                                          \
                VALUEREF = MAKE_PROPERTY_STAKES(0, SYM);                    \
            }                                                               \
        }                                                                   \
        else {                                                              \
            extract_db_value(str, VALUEREF);                                \
            CHECK_SYM(VALUEREF, SYM);                                       \
        }                                                                   \
    }

#define DECLARE_TOKEN_DB()                       \
    auto& tokendb = context.token_db;            \
    auto& tokendb_cache = context.token_db_cache;

} // namespace internal


namespace internal {

address
get_fungible_address(symbol sym) {
    return address(N(.fungible), name128::from_number(sym.id()), 0);
}

// for round  0: collect address
// for round >0: distrubute address
address
get_psvbonus_address(symbol_id_type sym_id, uint32_t round) {
    return address(N(.psvbonus), name128::from_number(sym_id), round);
}

// from, bonus
std::pair<int64_t, int64_t>
calculate_passive_bonus(token_database_cache& tokendb_cache,
                        symbol_id_type        sym_id,
                        int64_t               amount,
                        action_name           act) {
    auto pbs = make_empty_cache_ptr<passive_bonus_slim>();
    READ_DB_TOKEN_NO_THROW(token_type::psvbonus, std::nullopt, get_psvbonus_db_key(sym_id, kPsvBonusSlim), pbs);
    
    if(pbs == nullptr) {
        return std::make_pair(amount, 0l);
    }

    auto bonus = pbs->base_charge;
    bonus += (int64_t)boost::multiprecision::floor(pbs->rate.value() * amount);  // add trx fees
    if(pbs->minimum_charge.has_value()) {
        bonus = std::max(*pbs->minimum_charge, bonus);    // >= minimum
    }
    if(pbs->charge_threshold.has_value()) {
        bonus = std::min(*pbs->charge_threshold, bonus);  // <= threshold
    }

    auto method = passive_method_type::within_amount;

    auto it = std::find_if(pbs->methods.cbegin(), pbs->methods.cend(), [act](auto& m) { return m.action == act; });
    if(it != pbs->methods.cend()) {
        method = (passive_method_type)it->method;
    }

    switch(method) {
    case passive_method_type::within_amount: {
        bonus = std::min(amount, bonus);  // make sure amount >= bonus
        return std::make_pair(amount, bonus);
    }
    case passive_method_type::outside_amount: {
        return std::make_pair(amount + bonus, bonus);
    }
    default: {
        assert(false);
    }
    }  // switch

    return std::make_pair(amount, 0l);
}

void
transfer_fungible(apply_context& context,
                  const address& from,
                  const address& to,
                  const asset&   total,
                  action_name    act,
                  bool           pay_bonus = true) {
    using namespace boost::safe_numerics;
    DECLARE_TOKEN_DB()

    property pfrom, pto;

    auto sym = total.sym();
    if(sym == pevt_sym()) {
        // special process the situciation where sym is pevt_sym()
        // evt2pevt action
        READ_DB_ASSET(from, evt_sym(), pfrom);
        READ_DB_ASSET_NO_THROW(to, pevt_sym(), pto);
    }
    else {
        READ_DB_ASSET(from, sym, pfrom);
        READ_DB_ASSET_NO_THROW(to, sym, pto);
    }

    // fast path check
    EVT_ASSERT2(pfrom.amount >= total.amount(), balance_exception,
        "Address: {} does not have enough balance({}) left.", from, total);

    int64_t actual_amount = total.amount(), receive_amount = total.amount(), bonus_amount = 0;
    // evt and pevt cannot have passive bonus
    if(sym.id() > PEVT_SYM_ID && pay_bonus) {
        // check and calculate if fungible has passive bonus settings
        std::tie(actual_amount, bonus_amount) = calculate_passive_bonus(tokendb_cache, sym.id(), total.amount(), act);
        receive_amount = actual_amount - bonus_amount;
    }

    EVT_ASSERT2(pfrom.amount >= actual_amount, balance_exception,
        "There's not enough balance({}) within address: {}.", asset(actual_amount, sym), from);

    auto r1 = checked::subtract<int64_t>(pfrom.amount, actual_amount);
    auto r2 = checked::add<int64_t>(pto.amount, receive_amount);
    EVT_ASSERT(!r1.exception() && !r2.exception(), math_overflow_exception, "Opeartions resulted in overflows.");
    
    // update payee and payer
    pfrom.amount -= actual_amount;
    pto.amount   += receive_amount;

    PUT_DB_ASSET(to, pto);
    PUT_DB_ASSET(from, pfrom);

    // update bonus if needed
    if(bonus_amount > 0) {
        auto addr = get_psvbonus_address(sym.id(), 0);

        property pbonus;
        READ_DB_ASSET_NO_THROW(addr, sym, pbonus);

        auto r = checked::add<int64_t>(pbonus.amount, bonus_amount);
        EVT_ASSERT2(!r.exception(), math_overflow_exception, "Opeartions resulted in overflows.");

        pbonus.amount += bonus_amount;
        PUT_DB_ASSET(addr, pbonus);

        auto pbact = paybonus {
            .payer  = from,
            .amount = asset(bonus_amount, sym)
        };
        context.add_generated_action(action(N128(.fungible), name128::from_number(sym.id()), pbact))
            .set_index(context.exec_ctx.index_of<paybonus>());
    }
}

void
freeze_fungible(apply_context& context, const address& addr, asset total) {
    DECLARE_TOKEN_DB()

    auto sym = total.sym();

    property prop;
    READ_DB_ASSET(addr, sym, prop);

    EVT_ASSERT2(prop.amount >= total.amount(), balance_exception,
        "Address: {} does not have enough balance({}) left.", addr, total);

    prop.amount -= total.amount();
    prop.forzen_amount += total.amount();

    PUT_DB_ASSET(addr, prop);
}

void
unfreeze_fungible(apply_context& context, const address& addr, asset total) {
    DECLARE_TOKEN_DB()

    auto sym = total.sym();

    property prop;
    READ_DB_ASSET(addr, sym, prop);

    EVT_ASSERT2(prop.forzen_amount >= total.amount(), balance_exception,
        "Address: {} does not have enough forzen balance({}) left.", addr, total);

    prop.amount += total.amount();
    prop.forzen_amount -= total.amount();

    PUT_DB_ASSET(addr, prop);
}

}  // namespace internal

}}} // namespace evt::chain::contracts
