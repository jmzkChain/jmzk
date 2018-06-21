#include <evt/history_plugin/history_plugin.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/query_exception.hpp>

namespace evt {

static appbase::abstract_plugin& _history_plugin = app().register_plugin<history_plugin>();

using namespace evt;
using namespace evt::chain;

class history_plugin_impl {
public:
    const std::string blocks_col        = "Blocks";
    const std::string trans_col         = "Transactions";
    const std::string actions_col       = "Actions";
    const std::string action_traces_col = "ActionTraces";
    const std::string domains_col       = "Domains";
    const std::string tokens_col        = "Tokens";
    const std::string groups_col        = "Groups";
    const std::string accounts_col      = "Accounts";

public:
    history_plugin_impl(const mongocxx::database& db, const controller& chain)
        : db_(db), chain_(chain) {}

public:
    std::vector<public_key_type> recover_keys(const std::vector<std::string>& signatures);

    fc::flat_set<std::string> get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_domains_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_groups_by_public_keys(const std::vector<public_key_type>& pkeys);

public:
    const mongocxx::database& db_;
    const controller& chain_;
};

std::vector<public_key_type>
history_plugin_impl::recover_keys(const std::vector<std::string>& signatures) {
    std::vector<public_key_type> results;

    for(auto& s : signatures) {
        auto sig = signature_type(s);
        auto key = public_key_type(sig, chain_.get_chain_id());
        
        results.emplace_back(key);
    }
    return results;
}

fc::flat_set<std::string>
history_plugin_impl::get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys) {
    fc::flat_set<std::string> results;

    auto tokens = db_[tokens_col];
    for(auto& pkey : pkeys) {
        using bsoncxx::builder::stream::document;
        document find{};
        find << "owner" << (std::string)pkey;
        auto cursor = tokens.find(find.view());
        try {
            for(auto it = cursor.begin(); it != cursor.end(); it++) {
                auto id = (bsoncxx::stdx::string_view)(*it)["token_id"].get_utf8();
                results.insert(std::string(id.data(), id.size()));
            }
        }
        catch(mongocxx::query_exception e) {
            continue;
        }
    }
    return results;
}

fc::flat_set<std::string>
history_plugin_impl::get_domains_by_public_keys(const std::vector<public_key_type>& pkeys) {
    fc::flat_set<std::string> results;

    auto domains = db_[domains_col];
    for(auto& pkey : pkeys) {
        using bsoncxx::builder::stream::document;
        document find{};
        find << "issuer" << (std::string)pkey;
        auto cursor = domains.find(find.view());
        try {
            for(auto it = cursor.begin(); it != cursor.end(); it++) {
                auto name = (bsoncxx::stdx::string_view)(*it)["name"].get_utf8();;
                results.insert(std::string(name.data(), name.size()));
            }
        }
        catch(mongocxx::query_exception e) {
            continue;
        }
    }
    return results;
}

fc::flat_set<std::string>
history_plugin_impl::get_groups_by_public_keys(const std::vector<public_key_type>& pkeys) {
    fc::flat_set<std::string> results;

    auto groups = db_[groups_col];
    for(auto& pkey : pkeys) {
        using bsoncxx::builder::stream::document;
        document find{};
        find << "def.key" << (std::string)pkey;
        auto cursor = groups.find(find.view());
        try {
            for(auto it = cursor.begin(); it != cursor.end(); it++) {
                auto name = (bsoncxx::stdx::string_view)(*it)["name"].get_utf8();
                results.insert(std::string(name.data(), name.size()));
            }
        }
        catch(mongocxx::query_exception e) {
            continue;
        }
    }
    return results;
}

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
    this->my_.reset(new history_plugin_impl(
        app().get_plugin<mongo_db_plugin>().db(),
        app().get_plugin<chain_plugin>().chain()
        ));
}

void
history_plugin::plugin_shutdown() {
}

namespace history_apis {

namespace __internal {

std::vector<public_key_type>
recover_keys(const chain_id_type& chain_id, const std::vector<std::string>& signatures) {
    std::vector<public_key_type> results;
    for(auto& s : signatures) {
        auto sig = signature_type(s);
        auto key = public_key_type(sig, chain_id);
        
        results.emplace_back(key);
    }
    return results;
}

}  // namespace __internal

fc::variant
read_only::get_my_tokens(const get_my_params& params) {
    using namespace __internal;

    auto tokens = plugin_.my_->get_tokens_by_public_keys(plugin_.my_->recover_keys(params.signatures));
    fc::variant result;
    fc::to_variant(tokens, result);
    return result;
}

fc::variant
read_only::get_my_domains(const get_my_params& params) {
    using namespace __internal;

    auto domains = plugin_.my_->get_domains_by_public_keys(plugin_.my_->recover_keys(params.signatures));
    fc::variant result;
    fc::to_variant(domains, result);
    return result;
}

fc::variant
read_only::get_my_groups(const get_my_params& params) {
    using namespace __internal;

    auto groups = plugin_.my_->get_groups_by_public_keys(plugin_.my_->recover_keys(params.signatures));
    fc::variant result;
    fc::to_variant(groups, result);
    return result;
}

}  // namespace history_apis

}  // namespace evt