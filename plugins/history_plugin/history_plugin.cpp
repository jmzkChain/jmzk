#include <evt/history_plugin/history_plugin.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/history_plugin/evt_pg_query.hpp>

namespace evt {

static appbase::abstract_plugin& _history_plugin = app().register_plugin<history_plugin>();

class history_plugin_impl {
public:
    history_plugin_impl()
        : pg_query_(app().get_io_service(), app().get_plugin<chain_plugin>().chain()) {
        pg_query_.connect(app().get_plugin<postgres_plugin>().connstr());
        pg_query_.prepare_stmts();
        pg_query_.begin_poll_read();
    }

    ~history_plugin_impl() {
        pg_query_.close();
    }

public:
    pg_query pg_query_;
};

history_plugin::history_plugin() {}
history_plugin::~history_plugin() {}

void
history_plugin::set_program_options(options_description& cli, options_description& cfg) {
}

void
history_plugin::plugin_initialize(const variables_map& options) {
}

void
history_plugin::plugin_startup() {
    if(app().get_plugin<postgres_plugin>().enabled()) {
        my_.reset(new history_plugin_impl());
    }
    else {
        wlog("evt::postgres_plugin configured, but no --postgres-uri specified.");
        wlog("history_plugin disabled.");
    }
}

void
history_plugin::plugin_shutdown() {
}

namespace history_apis {

void
read_only::get_tokens_async(int id, const get_tokens_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_tokens_async(id, params);
}

void
read_only::get_domains_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_domains_async(id, params);
}

void
read_only::get_groups_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_groups_async(id, params);
}

void
read_only::get_fungibles_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_fungibles_async(id, params);
}

void
read_only::get_actions_async(int id, const get_actions_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_actions_async(id, params);
}

void
read_only::get_fungible_actions_async(int id, const get_fungible_actions_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_fungible_actions_async(id, params);
}

void
read_only::get_transaction_async(int id, const get_transaction_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_transaction_async(id, params);
}

void
read_only::get_transactions_async(int id, const get_transactions_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_transactions_async(id, params);
}

void
read_only::get_fungible_ids_async(int id, const get_fungible_ids_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_fungible_ids_async(id, params);
}

void
read_only::get_transaction_actions_async(int id, const get_transaction_actions_params& params) {
    EVT_ASSERT(plugin_.my_, chain::postgres_not_enabled_exception, "Postgres plugin is not enabled.");

    plugin_.my_->pg_query_.get_transaction_actions_async(id, params);
}

}}  // namespace evt::history_apis
