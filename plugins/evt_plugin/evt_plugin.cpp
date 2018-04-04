/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/evt_plugin/evt_plugin.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace eosio {

static appbase::abstract_plugin& _evt_plugin = app().register_plugin<evt_plugin>();

using namespace eosio;
using namespace eosio::chain;

class evt_plugin_impl {
public:
    evt_plugin_impl(chain_controller& db) : db_(db) {}

public:
    chain_controller& db_;
};

evt_plugin::evt_plugin(){}
evt_plugin::~evt_plugin(){}

void
evt_plugin::set_program_options(options_description& cli, options_description& cfg) {

}

void
evt_plugin::plugin_initialize(const variables_map& options) {
    this->my_.reset(new evt_plugin_impl(app().get_plugin<chain_plugin>().chain()));
}

void
evt_plugin::plugin_startup() {

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
    const auto& db = db_.get_tokendb();
    variant var;
    auto r = db.read_domain(params.name, [&](const auto& d) {
        fc::to_variant(d, var);
    });
    FC_ASSERT(r == 0, "Cannot find domain: ${name}", ("name", params.name));
    return var;
}

fc::variant
read_only::get_group(const read_only::get_group_params& params) {
    const auto& db = db_.get_tokendb();
    variant var;
    auto r = db.read_group(params.id, [&](const auto& g) {
        fc::to_variant(g, var);
    });
    FC_ASSERT(r == 0, "Cannot find group: ${id}", ("id", params.id));
    return var;
}

fc::variant
read_only::get_token(const read_only::get_token_params& params) {
    const auto& db = db_.get_tokendb();
    variant var;
    auto r = db.read_token(params.domain, params.name, [&](const auto& t) {
        fc::to_variant(t, var);
    });
    FC_ASSERT(r == 0, "Cannot find token: ${domain}-${name}", ("domain", params.domain)("name", params.name));
    return var;
}

read_write::new_domain_result
read_write::new_domain(const newdomain& params) {
    return read_write::new_domain_result();
}

read_write::issue_tokens_result
read_write::issue_tokens(const issuetoken& params) {
    return read_write::issue_tokens_result();
}

read_write::transfer_result
read_write::transfer_token(const transfer& params) {
    return read_write::transfer_result();
}

}  // namespace evt_apis

}  // namespace eosio
