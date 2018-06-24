/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/evt_plugin/evt_plugin.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/token_database.hpp>

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
    return var;
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
read_only::get_account(const get_account_params& params) {
    const auto& db = db_.token_db();
    variant     var;
    account_def account;
    db.read_account(params.name, account);
    fc::to_variant(account, var);
    return var;
}

}  // namespace evt_apis

}  // namespace evt
