/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/mongo_db_plugin/wallet_query.hpp>
#include <string_view>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/exception/query_exception.hpp>

namespace evt {

const std::string domains_col  = "Domains";
const std::string tokens_col   = "Tokens";
const std::string groups_col   = "Groups";
const std::string accounts_col = "Accounts";

fc::flat_set<std::string>
wallet_query::get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys) {
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
wallet_query::get_domains_by_public_keys(const std::vector<public_key_type>& pkeys) {
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
wallet_query::get_groups_by_public_keys(const std::vector<public_key_type>& pkeys) {
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

}  // namespace evt
