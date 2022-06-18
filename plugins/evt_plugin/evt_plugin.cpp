/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <jmzk/jmzk_plugin/jmzk_plugin.hpp>

#include <fc/container/flat.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <jmzk/chain/types.hpp>
#include <jmzk/chain/asset.hpp>
#include <jmzk/chain/address.hpp>
#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/token_database.hpp>
#include <jmzk/chain/token_database_cache.hpp>
#include <jmzk/chain/contracts/jmzk_contract_abi.hpp>

namespace jmzk {

static appbase::abstract_plugin& _jmzk_plugin = app().register_plugin<jmzk_plugin>();

using namespace jmzk;
using namespace jmzk::chain;

class jmzk_plugin_impl {
public:
    jmzk_plugin_impl(controller& db)
        : db_(db) {}

public:
    controller& db_;
};

jmzk_plugin::jmzk_plugin() {}
jmzk_plugin::~jmzk_plugin() {}

void jmzk_plugin::set_program_options(options_description& cli, options_description& cfg) {}

void jmzk_plugin::plugin_initialize(const variables_map& options) {}

void
jmzk_plugin::plugin_startup() {
    this->my_.reset(new jmzk_plugin_impl(app().get_plugin<chain_plugin>().chain()));
}

void jmzk_plugin::plugin_shutdown() {}

jmzk_apis::read_only
jmzk_plugin::get_read_only_api() const {
    return jmzk_apis::read_only(my_->db_);
}

jmzk_apis::read_write
jmzk_plugin::get_read_write_api() {
    return jmzk_apis::read_write();
}

namespace jmzk_apis {

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...)      \
    try {                                                                   \
        using vtype = typename decltype(VPTR)::element_type;                \
        VPTR = tokendb_cache.template read_token<vtype>(TYPE, PREFIX, KEY); \
    }                                                                       \
    catch(token_database_exception&) {                                      \
        jmzk_THROW2(EXCEPTION, FORMAT, __VA_ARGS__);                         \
    }
    
#define MAKE_PROPERTY(AMOUNT, SYM) \
    property {                     \
        .amount = AMOUNT,          \
        .frozen_amount = 0,   \
        .sym = SYM,                \
        .created_at = 0,           \
        .created_index = 0         \
    }
   
#define READ_DB_ASSET(ADDR, SYM, VALUEREF)                                                         \
    try {                                                                                          \
        auto str = std::string();                                                                  \
        tokendb.read_asset(ADDR, SYM.id(), str);                                                   \
                                                                                                   \
        extract_db_value(str, VALUEREF);                                                           \
    }                                                                                              \
    catch(token_database_exception&) {                                                             \
        jmzk_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM); \
    }

#define READ_DB_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                         \
    {                                                                       \
        auto str = std::string();                                           \
        if(!tokendb.read_asset(ADDR, SYM.id(), str, true /* no throw */)) { \
            VALUEREF = MAKE_PROPERTY(0, SYM);                               \
        }                                                                   \
        else {                                                              \
            extract_db_value(str, VALUEREF);                                \
        }                                                                   \
    }

#define DECLARE_TOKEN_DB()                     \
    auto& tokendb = db_.token_db();            \
    auto& tokendb_cache = db_.token_db_cache();

enum psvbonus_type { kPsvBonus = 0, kPsvBonusSlim };

name128
get_psvbonus_db_key(symbol_id_type id, uint64_t nonce) {
    uint128_t v = nonce;
    v |= ((uint128_t)id << 64);
    return v;
}

fc::variant
read_only::get_domain(const read_only::get_domain_params& params) {
    DECLARE_TOKEN_DB();

    auto var    = variant();
    auto domain = make_empty_cache_ptr<domain_def>();
    READ_DB_TOKEN(token_type::domain, std::nullopt, params.name, domain, unknown_domain_exception, "Cannot find domain: {}", params.name);

    fc::to_variant(*domain, var);

    auto mvar = fc::mutable_variant_object(var);
    mvar["address"] = address(N(.domain), params.name, 0);
    return mvar;
}

fc::variant
read_only::get_group(const read_only::get_group_params& params) {
    DECLARE_TOKEN_DB();

    auto var   = variant();
    auto group = make_empty_cache_ptr<group_def>();
    READ_DB_TOKEN(token_type::group, std::nullopt, params.name, group, unknown_group_exception, "Cannot find group: {}", params.name);

    fc::to_variant(*group, var);

    auto mvar = fc::mutable_variant_object(var);
    mvar["address"] = address(N(.group), params.name, 0);
    return mvar;
}

fc::variant
read_only::get_token(const read_only::get_token_params& params) {
    DECLARE_TOKEN_DB();

    auto var   = variant();
    auto token = make_empty_cache_ptr<token_def>();
    READ_DB_TOKEN(token_type::token, params.domain, params.name, token, unknown_token_exception, "Cannot find token: {} in {}", params.name, params.domain);

    fc::to_variant(*token, var);
    return var;
}

fc::variant
read_only::get_tokens(const get_tokens_params& params) {
    DECLARE_TOKEN_DB();

    auto vars = fc::variants();
    int s = 0, t = 10;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        jmzk_ASSERT(t <= 100, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 100 per query");
    }

    int i = 0;
    tokendb.read_tokens_range(token_type::token, params.domain, s, [&](auto& key, auto&& value) {
        auto var = fc::variant();

        token_def token;
        extract_db_value(value, token);

        fc::to_variant(token, var);
        vars.emplace_back(std::move(var));

        if(++i == t) {
            return false;
        }
        return true;
    });

    return vars;
}

fc::variant
read_only::get_fungible(const get_fungible_params& params) {
    DECLARE_TOKEN_DB();

    auto var      = variant();
    auto fungible = make_empty_cache_ptr<fungible_def>();
    READ_DB_TOKEN(token_type::fungible, std::nullopt, params.id, fungible, unknown_fungible_exception, "Cannot find fungible with sym id: {}", params.id);

    fc::to_variant(*fungible, var);

    auto mvar = fc::mutable_variant_object(var);
    auto addr = address(N(.fungible), name128::from_number(params.id), 0);

    property prop;
    READ_DB_ASSET_NO_THROW(addr, fungible->sym, prop);

    mvar["current_supply"] = fungible->total_supply - asset(prop.amount, fungible->sym);
    mvar["address"]        = addr;
    return mvar;
}

fc::variant
read_only::get_fungible_balance(const get_fungible_balance_params& params) {
    DECLARE_TOKEN_DB();

    auto vars = variants();
    if(params.sym_id.has_value()) {
        auto fungible = make_empty_cache_ptr<fungible_def>();
        READ_DB_TOKEN(token_type::fungible, std::nullopt, *params.sym_id, fungible,
            unknown_fungible_exception, "Cannot find fungible with sym id: {}", *params.sym_id);

        property prop;
        READ_DB_ASSET_NO_THROW(params.address, fungible->sym, prop);

        auto as  = asset(prop.amount, prop.sym);
        auto var = variant();
        fc::to_variant(as, var);

        vars.emplace_back(std::move(var));
        return vars;
    }
    jmzk_THROW(unsupported_feature, "Read all the balance of fungibles tokens within one address is not supported in jmzk_plugin anymore, please refer to the history_plugin");
}

fc::variant
read_only::get_fungible_psvbonus(const get_fungible_psvbonus_params& params) {
    DECLARE_TOKEN_DB();

    auto pb   = make_empty_cache_ptr<passive_bonus>();
    auto dkey = get_psvbonus_db_key(params.id, kPsvBonus);
    READ_DB_TOKEN(token_type::psvbonus, std::nullopt, dkey, pb, unknown_bonus_exception,
        "Cannot find passive bonus registered for fungible token with sym id: {}.", params.id);

    auto var = variant();
    fc::to_variant(*pb, var);

    auto mvar = fc::mutable_variant_object(var);
    auto addr = address(N(.psvbonus), name128::from_number(params.id), 0);
    mvar["address"] = addr;

    return mvar;
}

fc::variant
read_only::get_suspend(const get_suspend_params& params) {
    DECLARE_TOKEN_DB();

    auto var     = variant();
    auto suspend = make_empty_cache_ptr<suspend_def>();
    READ_DB_TOKEN(token_type::suspend, std::nullopt, params.name, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", params.name);

    db_.get_abi_serializer().to_variant(*suspend, var, db_.get_execution_context());
    return var;
}

fc::variant
read_only::get_lock(const get_lock_params& params) {
    DECLARE_TOKEN_DB();

    auto var  = variant();
    auto lock = make_empty_cache_ptr<lock_def>();
    READ_DB_TOKEN(token_type::lock, std::nullopt, params.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", params.name);

    fc::to_variant(*lock, var);
    return var;
}

fc::variant
read_only::get_stakepool(const get_stakepool_params& params) {
    DECLARE_TOKEN_DB();

    auto var  = variant();
    auto pool = make_empty_cache_ptr<stakepool_def>();
    READ_DB_TOKEN(token_type::stakepool, std::nullopt, params.sym_id, pool, unknown_stakepool_exception, "Cannot find stakepool with sym id: {}", params.sym_id);

    fc::to_variant(*pool, var);
    return var;
}

fc::variant
read_only::get_validator(const get_validator_params& params) {
    DECLARE_TOKEN_DB();

    auto var  = variant();
    auto validator = make_empty_cache_ptr<validator_def>();
    READ_DB_TOKEN(token_type::validator, std::nullopt, params.name, validator, unknown_validator_exception, "Cannot find validator: {}", params.name);
    fc::to_variant(*validator, var);

    property prop;
    READ_DB_ASSET_NO_THROW(address(N(.validator), validator->name, jmzk_SYM_ID), symbol(5,jmzk_SYM_ID), prop);

    auto mvar = fc::mutable_variant_object(var);
    mvar["profit"] = asset(prop.amount, prop.sym);

    return mvar;
}

fc::variant
read_only::get_staking_shares(const get_staking_shares_params& params) {
    DECLARE_TOKEN_DB();

    auto var  = variant();
    auto prop = property_stakes();
    READ_DB_ASSET(params.address, jmzk_sym(), prop);

    fc::to_variant(prop, var);
    return var;
}

read_only::get_jmzklink_signed_keys_result
read_only::get_jmzklink_signed_keys(const get_jmzklink_signed_keys_params& params) const {
    if(params.link_id.size() != sizeof(link_id_type)) {
        jmzk_THROW(jmzk_link_id_exception, "jmzk-Link id is not in proper length");
    }

    auto link_id = link_id_type();
    memcpy(&link_id, params.link_id.data(), sizeof(link_id));

    auto result        = get_jmzklink_signed_keys_result();
    result.signed_keys = db_.get_jmzklink_signed_keys(link_id);
    return result;
}

fc::variant
read_only::get_script(const get_script_params& params) const {
    DECLARE_TOKEN_DB();

    auto var  = variant();
    auto script = make_empty_cache_ptr<script_def>();
    READ_DB_TOKEN(token_type::script, std::nullopt, params.name, script, unknown_script_exception, "Cannot find script: {}", params.name);
    to_variant(*script, var);

    return var;
}

}  // namespace jmzk_apis

}  // namespace jmzk
