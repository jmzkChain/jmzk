/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/evt_plugin/evt_plugin.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/evt_contract_abi.hpp>

#include <fc/container/flat.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace evt {

static appbase::abstract_plugin& _evt_plugin = app().register_plugin<evt_plugin>();

using namespace evt;
using namespace evt::chain;

class evt_plugin_impl {
public:
    evt_plugin_impl(controller& db)
        : db_(db) {}

public:
    controller& db_;
};

evt_plugin::evt_plugin() {}
evt_plugin::~evt_plugin() {}

void evt_plugin::set_program_options(options_description& cli, options_description& cfg) {}

void evt_plugin::plugin_initialize(const variables_map& options) {}

void
evt_plugin::plugin_startup() {
    this->my_.reset(new evt_plugin_impl(app().get_plugin<chain_plugin>().chain()));
}

void evt_plugin::plugin_shutdown() {}

evt_apis::read_only
evt_plugin::get_read_only_api() const {
    return evt_apis::read_only(my_->db_);
}

evt_apis::read_write
evt_plugin::get_read_write_api() {
    return evt_apis::read_write();
}

namespace evt_apis {

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VALUEREF, EXCEPTION, FORMAT, ...)  \
    try {                                                                   \
        auto str = std::string();                                           \
        db.read_token(TYPE, PREFIX, KEY, str);                              \
                                                                            \
        extract_db_value(str, VALUEREF);                                    \
    }                                                                       \
    catch(token_database_exception&) {                                      \
        EVT_THROW2(EXCEPTION, FORMAT, __VA_ARGS__);                         \
    }

#define MAKE_PROPERTY(AMOUNT, SYM) \
    property {                     \
        .amount = AMOUNT,          \
        .sym = SYM,                \
        .created_at = 0,           \
        .created_index = 0         \
    }
   
#define READ_DB_ASSET(ADDR, SYM, VALUEREF)                                                         \
    try {                                                                                          \
        auto str = std::string();                                                                  \
        db.read_asset(ADDR, SYM.id(), str);                                                        \
                                                                                                   \
        extract_db_value(str, VALUEREF);                                                           \
    }                                                                                              \
    catch(token_database_exception&) {                                                             \
        EVT_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM); \
    }

#define READ_DB_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                    \
    {                                                                  \
        auto str = std::string();                                      \
        if(!db.read_asset(ADDR, SYM.id(), str, true /* no throw */)) { \
            VALUEREF = MAKE_PROPERTY(0, SYM);                          \
        }                                                              \
        else {                                                         \
            extract_db_value(str, VALUEREF);                           \
        }                                                              \
    }

// for bonus_id, 0 is always stand for passive bonus
// otherwise it's active bonus
name128
get_bonus_db_key(uint64_t sym_id, uint64_t bonus_id) {
    uint128_t v = bonus_id;
    v |= ((uint128_t)sym_id << 64);
    return v;
}

fc::variant
read_only::get_domain(const read_only::get_domain_params& params) {
    const auto& db = db_.token_db();

    variant    var;
    domain_def domain;
    READ_DB_TOKEN(token_type::domain, std::nullopt, params.name, domain, unknown_domain_exception, "Cannot find domain: {}", params.name);

    fc::to_variant(domain, var);

    auto mvar = fc::mutable_variant_object(var);
    mvar["address"] = address(N(.domain), domain.name, 0);
    return mvar;
}

fc::variant
read_only::get_group(const read_only::get_group_params& params) {
    const auto& db = db_.token_db();

    variant   var;
    group_def group;
    READ_DB_TOKEN(token_type::group, std::nullopt, params.name, group, unknown_group_exception, "Cannot find group: {}", params.name);

    fc::to_variant(group, var);
    return var;
}

fc::variant
read_only::get_token(const read_only::get_token_params& params) {
    const auto& db = db_.token_db();

    variant   var;
    token_def token;
    READ_DB_TOKEN(token_type::token, params.domain, params.name, token, unknown_token_exception, "Cannot find token: {} in {}", params.name, params.domain);

    fc::to_variant(token, var);
    return var;
}

fc::variant
read_only::get_tokens(const get_tokens_params& params) {
    const auto& db = db_.token_db();

    auto vars = fc::variants();
    int s = 0, t = 10;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        EVT_ASSERT(t <= 100, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 100 per query");
    }

    int i = 0;
    db.read_tokens_range(token_type::token, params.domain, s, [&](auto& key, auto&& value) {
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
    const auto& db = db_.token_db();

    variant      var;
    fungible_def fungible;
    READ_DB_TOKEN(token_type::fungible, std::nullopt, params.id, fungible, unknown_fungible_exception, "Cannot find fungible with sym id: {}", params.id);

    fc::to_variant(fungible, var);

    auto mvar = fc::mutable_variant_object(var);
    auto addr = address(N(.fungible), name128::from_number(params.id), 0);

    property prop;
    READ_DB_ASSET(addr, fungible.sym, prop);

    mvar["current_supply"] = fungible.total_supply - asset(prop.amount, fungible.sym);
    mvar["address"]        = addr;
    return mvar;
}

fc::variant
read_only::get_fungible_balance(const get_fungible_balance_params& params) {
    const auto& db = db_.token_db();

    variants vars;
    if(params.sym_id.has_value()) {
        fungible_def fungible;
        READ_DB_TOKEN(token_type::fungible, std::nullopt, *params.sym_id, fungible,
            unknown_fungible_exception, "Cannot find fungible with sym id: {}", *params.sym_id);

        property prop;
        READ_DB_ASSET_NO_THROW(params.address, fungible.sym, prop);

        auto as  = asset(prop.amount, prop.sym);
        auto var = variant();
        fc::to_variant(as, var);

        vars.emplace_back(std::move(var));
        return vars;
    }
    EVT_THROW(unsupported_feature, "Read all the balance of fungibles tokens within one address is not supported in evt_plugin anymore, please refer to the history_plugin");
}

fc::variant
read_only::get_fungible_psvbonus(const get_fungible_psvbonus_params& params) {
    const auto& db = db_.token_db();

    auto pb   = passive_bonus();
    auto dkey = get_bonus_db_key(params.id, 0);
    READ_DB_TOKEN(token_type::bonus, std::nullopt, dkey, pb, unknown_bonus_exception,
        "Cannot find passive bonus registered for fungible with sym id: {}.", params.id);

    auto var = variant();
    fc::to_variant(pb, var);

    auto mvar = fc::mutable_variant_object(var);
    auto addr = address(N(.bonus), name128::from_number(params.id), 0);
    mvar["address"] = addr;

    return mvar;
}

fc::variant
read_only::get_suspend(const get_suspend_params& params) {
    const auto& db = db_.token_db();
    variant     var;
    suspend_def suspend;
    READ_DB_TOKEN(token_type::suspend, std::nullopt, params.name, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", params.name);

    db_.get_abi_serializer().to_variant(suspend, var, db_.get_execution_context());
    return var;
}

fc::variant
read_only::get_lock(const get_lock_params& params) {
    const auto& db = db_.token_db();
    variant  var;
    lock_def lock;
    READ_DB_TOKEN(token_type::lock, std::nullopt, params.name, lock, unknown_lock_exception, "Cannot find lock proposal: {}", params.name);

    fc::to_variant(lock, var);
    return var;
}

}  // namespace evt_apis

}  // namespace evt
