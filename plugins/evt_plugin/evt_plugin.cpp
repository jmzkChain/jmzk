/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/evt_plugin/evt_plugin.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/evt_contract.hpp>

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

void
evt_plugin::set_program_options(options_description& cli, options_description& cfg) {
}

void
evt_plugin::plugin_initialize(const variables_map& options) {
}

void
evt_plugin::plugin_startup() {
    this->my_.reset(new evt_plugin_impl(app().get_plugin<chain_plugin>().chain()));
}

void
evt_plugin::plugin_shutdown() {
}

evt_apis::read_only
evt_plugin::get_read_only_api() const {
    return evt_apis::read_only(my_->db_);
}

evt_apis::read_write
evt_plugin::get_read_write_api() {
    return evt_apis::read_write(my_->db_);
}

namespace evt_apis {

fc::variant
read_only::get_domain(const read_only::get_domain_params& params) {
    const auto& db = db_.token_db();
    variant    var;
    domain_def domain;
    db.read_domain(params.name, domain);
    fc::to_variant(domain, var);

    auto mvar = fc::mutable_variant_object(var);
    mvar["address"] = address(N(domain), domain.name, 0);
    return mvar;
}

fc::variant
read_only::get_group(const read_only::get_group_params& params) {
    const auto& db = db_.token_db();
    variant   var;
    group_def group;
    db.read_group(params.name, group);
    fc::to_variant(group, var);
    return var;
}

fc::variant
read_only::get_token(const read_only::get_token_params& params) {
    const auto& db = db_.token_db();
    variant   var;
    token_def token;
    db.read_token(params.domain, params.name, token);
    fc::to_variant(token, var);
    return var;
}

fc::variant
read_only::get_fungible(const get_fungible_params& params) {
    const auto& db = db_.token_db();
    variant      var;
    fungible_def fungible;
    db.read_fungible(params.id, fungible);
    fc::to_variant(fungible, var);

    auto mvar = fc::mutable_variant_object(var);
    auto addr = address(N(fungible), (fungible_name)std::to_string(params.id), 0);

    asset as;
    db_.token_db().read_asset_no_throw(addr, fungible.sym, as);
    mvar["current_supply"] = fungible.total_supply - as;
    mvar["address"] = addr;
    return mvar;
}

fc::variant
read_only::get_fungible_balance(const get_fungible_balance_params& params) {
    const auto& db = db_.token_db();

    variants vars;
    if(params.sym_id.valid()) {
        fungible_def fungible;
        db.read_fungible(*params.sym_id, fungible);

        variant var;
        asset   as;
        db.read_asset_no_throw(params.address, fungible.sym, as);
        fc::to_variant(as, var);
        vars.emplace_back(std::move(var));
        return vars;
    }
    else {
        db.read_all_assets(params.address, [&vars](const auto& as) {
            variant var;
            fc::to_variant(as, var);
            vars.emplace_back(std::move(var));
            return true;
        });
        return vars;
    }
}

fc::variant
read_only::get_suspend(const get_suspend_params& params) {
    const auto& db = db_.token_db();
    variant     var;
    suspend_def suspend;
    db.read_suspend(params.name, suspend);
    db_.get_abi_serializer().to_variant(suspend, var);
    return var;
}

fc::variant
read_only::get_lock(const get_lock_params& params) {
    const auto& db = db_.token_db();
    variant  var;
    lock_def lock;
    db.read_lock(params.name, lock);
    fc::to_variant(lock, var);
    return var;
}

}  // namespace evt_apis

}  // namespace evt
