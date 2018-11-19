#include <evt/history_plugin/history_plugin.hpp>

#include <mutex>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/exception/query_exception.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/history_plugin/evt_pg_query.hpp>

namespace evt {

static appbase::abstract_plugin& _history_plugin = app().register_plugin<history_plugin>();

using namespace evt;
using namespace evt::chain;
using std::string;
using std::vector;
using fc::flat_set;
using fc::variant;
using fc::optional;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::array;
using history_apis::direction;

class history_plugin_impl {
public:
    history_plugin_impl()
        : chain_(app().get_plugin<chain_plugin>().chain())
        , abi_(chain_.get_abi_serializer())
        , pg_query_(app().get_io_service()) {
        auto& uri = app().get_plugin<mongo_db_plugin>().uri();
        
        client_ = mongocxx::client(uri);
        if(uri.database().empty()) {
            db_ = client_["EVT"];
        }
        else {
            db_ = client_[uri.database()];
        }

        blocks_col_    = db_["Blocks"];
        trxs_col_      = db_["Transactions"];
        actions_col_   = db_["Actions"];
        domains_col_   = db_["Domains"];
        tokens_col_    = db_["Tokens"];
        groups_col_    = db_["Groups"];
        fungibles_col_ = db_["Fungibles"];

        pg_query_.connect(app().get_plugin<postgres_plugin>().connstr());
        pg_query_.prepare_stmts();
        pg_query_.begin_poll_read();
    }

public:
    variant get_tokens_by_public_keys(const vector<public_key_type>& pkeys,
                                      const optional<domain_name>&   domain,
                                      const optional<token_name>&    name);
    flat_set<string> get_domains_by_public_keys(const vector<public_key_type>& pkeys);
    flat_set<string> get_groups_by_public_keys(const vector<public_key_type>& pkeys);
    flat_set<symbol_id_type> get_fungibles_by_public_keys(const vector<public_key_type>& pkeys);

    variant get_actions(const domain_name&             domain,
                        const optional<domain_key>&    key,
                        const std::vector<action_name> names,
                        const direction                dire, 
                        const optional<int>            skip,
                        const optional<int>            take);

    variant get_fungible_actions(const symbol_id_type        sym_id,
                                 const fc::optional<address> addr,
                                 const direction             dire, 
                                 const optional<int>         skip,
                                 const optional<int>         take);

    variant get_transaction(const transaction_id_type&      trx_id);
    variant get_transactions(const vector<public_key_type>& pkeys,
                             const direction                dire,
                             const optional<int>            skip,
                             const optional<int>            take);

private:
    block_id_type get_block_id_by_trx_id(const transaction_id_type& trx_id);
    string get_bson_string_value(const mongocxx::cursor::iterator& it, const std::string& key);
    string get_date_string_value(const mongocxx::cursor::iterator& it, const std::string& key);
    int get_int_value(const mongocxx::cursor::iterator& it, const std::string& key);
    variant transaction_to_variant(const packed_transaction& ptrx);

public:
    mongocxx::client   client_;
    mongocxx::database db_;
    
    const controller&     chain_;
    const abi_serializer& abi_;

    pg_query pg_query_;

private:
    mongocxx::collection blocks_col_;
    mongocxx::collection trxs_col_;
    mongocxx::collection actions_col_;
    mongocxx::collection domains_col_;
    mongocxx::collection tokens_col_;
    mongocxx::collection groups_col_;
    mongocxx::collection fungibles_col_;
};

string
history_plugin_impl::get_bson_string_value(const mongocxx::cursor::iterator& it, const std::string& key) {
    auto v = (bsoncxx::stdx::string_view)(*it)[key].get_utf8();
    return string(v.data(), v.size());
}

string
history_plugin_impl::get_date_string_value(const mongocxx::cursor::iterator& it, const std::string& key) {
    auto date = (*it)[key].get_date();
    auto tp = fc::time_point(fc::milliseconds(date.to_int64()));
    return (std::string)tp;
}

int
history_plugin_impl::get_int_value(const mongocxx::cursor::iterator& it, const std::string& key) {
    auto v = (*it)[key].get_int32();
    return v;
}

fc::variant
history_plugin_impl::transaction_to_variant(const packed_transaction& ptrx) {
    fc::variant pretty_output;
    abi_.to_variant(ptrx, pretty_output);
    return pretty_output;
}


variant
history_plugin_impl::get_tokens_by_public_keys(const vector<public_key_type>& pkeys,
                                               const optional<domain_name>&   domain,
                                               const optional<token_name>&    name) {
    auto results = fc::mutable_variant_object();

    auto keys = array();
    for(auto& pkey : pkeys) {
        keys << (string)pkey;
    }

    auto find = document();
    find << "owner" << open_document << "$in" << keys << close_document;

    if(domain.valid()) {
        find << "domain" << (string)*domain;
    }
    if(name.valid()) {
        find << "name" << (string)*name;
    }

    auto cursor = tokens_col_.find(find.view());
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto domain = get_bson_string_value(it, "domain");
            auto name   = get_bson_string_value(it, "name");

            if(results.find(domain) == results.end()) {
                results.set(domain, fc::variants());
            }
            results[domain].get_array().emplace_back(std::move(name));
        }
    }
    catch(mongocxx::query_exception e) {}

    return results;
}

flat_set<string>
history_plugin_impl::get_domains_by_public_keys(const vector<public_key_type>& pkeys) {
    flat_set<string> results;

    auto keys = array();
    for(auto& pkey : pkeys) {
        keys << (string)pkey;
    }

    auto find = document();
    find << "creator" << open_document << "$in" << keys << close_document;

    auto cursor = domains_col_.find(find.view());
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto name = get_bson_string_value(it, "name");
            results.insert(string(name.data(), name.size()));
        }
    }
    catch(mongocxx::query_exception e) {}

    return results;
}

flat_set<string>
history_plugin_impl::get_groups_by_public_keys(const vector<public_key_type>& pkeys) {
    flat_set<string> results;

    auto keys = array();
    for(auto& pkey : pkeys) {
        keys << (string)pkey;
    }

    auto find = document();
    find << "def.key" << open_document << "$in" << keys << close_document;

    auto cursor = groups_col_.find(find.view());
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto name = get_bson_string_value(it, "name");
            results.insert(string(name.data(), name.size()));
        }
    }
    catch(mongocxx::query_exception e) {}

    return results;
}

flat_set<symbol_id_type>
history_plugin_impl::get_fungibles_by_public_keys(const vector<public_key_type>& pkeys) {
    flat_set<symbol_id_type> results;

    auto keys = array();
    for(auto& pkey : pkeys) {
        keys << (string)pkey;
    }

    auto find = document();
    find << "creator" << open_document << "$in" << keys << close_document;

    auto cursor = fungibles_col_.find(find.view());
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto id = (*it)["sym_id"].get_int64();
            results.insert((symbol_id_type)id);
        }
    }
    catch(mongocxx::query_exception e) {}

    return results;
}

variant
history_plugin_impl::get_actions(const domain_name&             domain,
                                 const optional<domain_key>&    key,
                                 const std::vector<action_name> names,
                                 const direction                dire,
                                 const optional<int>            skip,
                                 const optional<int>            take) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;

    fc::variants result;

    int s = 0, t = 10;
    if(skip.valid()) {
        s = *skip;
    }
    if(take.valid()) {
        t = *take;
    }

    document match{};
    match << "domain" << (string)domain;
    if(key.valid()) {
        match << "key" << (string)*key;
    }
    if(!names.empty()) {
        auto ns = bsoncxx::builder::stream::array();
        for(auto& name : names) {
            ns << (std::string)name;
        }
        match << "name" << open_document << "$in" << ns << close_document;
    }

    document sort{};
    if(dire == direction::desc) {
        sort << "_id" << -1;
    }
    else {
        sort << "_id" << 1;
    }

    auto pipeline = mongocxx::pipeline();
    pipeline.match(match.view()).sort(sort.view()).skip(s).limit(t);

    auto cursor = actions_col_.aggregate(pipeline);
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto block_num = get_int_value(it, "block_num");
            auto block     = chain_.fetch_block_by_number(block_num);

            auto v = fc::mutable_variant_object();
            v["name"] = get_bson_string_value(it, "name");
            v["domain"] = get_bson_string_value(it, "domain");
            v["key"] = get_bson_string_value(it, "key");
            v["trx_id"] = get_bson_string_value(it, "trx_id");
            v["block_num"] = block_num;
            v["data"] = fc::json::from_string(bsoncxx::to_json((*it)["data"].get_document().view()));
            v["created_at"] = block->timestamp;
            v["timestamp"] = block->timestamp;

            result.emplace_back(std::move(v));
        }
    }
    catch(mongocxx::query_exception e) {
        return variant();
    }
    return variant(std::move(result));
}

variant
history_plugin_impl::get_fungible_actions(const symbol_id_type        sym_id,
                                          const fc::optional<address> addr,
                                          const direction             dire,
                                          const optional<int>         skip,
                                          const optional<int>         take) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;

    fc::variants result;

    int s = 0, t = 10;
    if(skip.valid()) {
        s = *skip;
    }
    if(take.valid()) {
        t = *take;
    }

    document match{};
    match << "domain" << ".fungible" << "key" << std::to_string(sym_id);

    static std::once_flag                  flag;
    static bsoncxx::builder::stream::array ns;

    std::call_once(flag, [] {
        ns << "issuefungible" << "transferft" << "recycleft" << "evt2pevt" << "everipay" << "paycharge";
    });

    match << "name" << open_document << "$in" << ns << close_document;

    auto pipeline = mongocxx::pipeline();
    pipeline.match(match.view());

    auto addr_match = document();
    if(addr.valid()) {
        auto saddr = (std::string)(*addr);

        addr_match << "$or" << open_array
                   << open_document << "data.address"   << saddr << close_document  // issue & recycleft
                   << open_document << "data.from"      << saddr << close_document  // transfer & evt2pevt
                   << open_document << "data.to"        << saddr << close_document  // transfer & evt2pevt
                   << open_document << "data.payee"     << saddr << close_document  // everiPay
                   << open_document << "data.link.keys" << saddr << close_document  // everiPay
                   << open_document << "data.payer"     << saddr << close_document  // paycharge
                   << close_array;
        pipeline.match(addr_match.view());
    }

    auto sort = document();
    if(dire == direction::desc) {
        sort << "_id" << -1;
    }
    else {
        sort << "_id" << 1;
    }

    pipeline.sort(sort.view()).skip(s).limit(t);

    auto cursor = actions_col_.aggregate(pipeline);
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto block_num = get_int_value(it, "block_num");
            auto block     = chain_.fetch_block_by_number(block_num);

            auto v = fc::mutable_variant_object();
            v["name"] = get_bson_string_value(it, "name");
            v["domain"] = get_bson_string_value(it, "domain");
            v["key"] = get_bson_string_value(it, "key");
            v["trx_id"] = get_bson_string_value(it, "trx_id");
            v["block_num"] = block_num;
            v["data"] = fc::json::from_string(bsoncxx::to_json((*it)["data"].get_document().view()));
            v["created_at"] = block->timestamp;
            v["timestamp"] = block->timestamp;

            result.emplace_back(std::move(v));
        }
    }
    catch(mongocxx::query_exception e) {
        return variant();
    }
    return variant(std::move(result));
}

block_id_type
history_plugin_impl::get_block_id_by_trx_id(const transaction_id_type& trx_id) {
    document find{};
    find << "trx_id" << (string)trx_id;

    auto cursor = trxs_col_.find(find.view());
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto bid = get_bson_string_value(it, "block_id");
            return block_id_type(bid);
        }
    }
    catch(...) {}
    FC_THROW_EXCEPTION(unknown_transaction_exception, "Cannot find transaction");
}

variant
history_plugin_impl::get_transaction(const transaction_id_type& trx_id) {
    auto block_id = get_block_id_by_trx_id(trx_id);
    auto block = chain_.fetch_block_by_id(block_id);
    for(auto& tx : block->transactions) {
        if(tx.trx.id() == trx_id) {
            auto mv = fc::mutable_variant_object(transaction_to_variant(tx.trx));
            mv["block_num"] = block->block_num();
            return mv;
        }
    }
    FC_THROW_EXCEPTION(unknown_transaction_exception, "Cannot find transaction");
}

variant
history_plugin_impl::get_transactions(const vector<public_key_type>& pkeys,
                                      const direction                dire,
                                      const optional<int>            skip,
                                      const optional<int>            take) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;

    int s = 0, t = 10;
    if(skip.valid()) {
        s = *skip;
    }
    if(take.valid()) {
        t = *take;
    }

    auto match = document();
    auto keys  = bsoncxx::builder::stream::array();

    for(auto& pkey : pkeys) {
        keys << (string)pkey;
    }
    match << "keys" << open_document << "$in" << keys << close_document;

    document sort{};
    if(dire == direction::desc) {
        sort << "_id" << -1;
    }
    else {
        sort << "_id" << 1;
    }

    document project{};
    project << "trx_id" << 1;

    auto pipeline = mongocxx::pipeline();
    pipeline.match(match.view()).project(project.view()).sort(sort.view()).skip(s).limit(t);

    auto cursor = trxs_col_.aggregate(pipeline);

    auto vars = fc::variants();
    auto tids = vector<transaction_id_type>();
    try {
        for(auto it = cursor.begin(); it != cursor.end(); it++) {
            auto tid = get_bson_string_value(it, "trx_id");
            tids.emplace_back((transaction_id_type)tid);
        }
    }
    catch(mongocxx::query_exception e) {
        return vars;
    }
    
    for(auto& tid : tids) {
        vars.emplace_back(get_transaction(tid));
    }
    return vars;
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
    if(app().get_plugin<mongo_db_plugin>().enabled()) {
        my_.reset(new history_plugin_impl());
    }
    else {
        wlog("evt::mongo_db_plugin configured, but no --mongodb-uri specified.");
        wlog("history_plugin disabled.");
    }
}

void
history_plugin::plugin_shutdown() {
}

namespace history_apis {

void
read_only::get_tokens_async(int id, const get_tokens_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    plugin_.my_->pg_query_.get_tokens_async(id, params);
}

void
read_only::get_domains_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    plugin_.my_->pg_query_.get_domains_async(id, params);
}

void
read_only::get_groups_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    plugin_.my_->pg_query_.get_groups_async(id, params);
}

void
read_only::get_fungibles_async(int id, const get_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    plugin_.my_->pg_query_.get_fungibles_async(id, params);
}

void
read_only::get_actions_async(int id, const get_actions_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    plugin_.my_->pg_query_.get_actions_async(id, params);
}

fc::variant
read_only::get_fungible_actions(const get_fungible_actions_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    auto dire = (params.dire.valid() && *params.dire == direction::asc) ? direction::asc : direction::desc;
    return plugin_.my_->get_fungible_actions(params.sym_id, params.addr, dire, params.skip, params.take);
}

fc::variant
read_only::get_transaction(const get_transaction_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    return plugin_.my_->get_transaction(params.id);
}

fc::variant
read_only::get_transactions(const get_transactions_params& params) {
    EVT_ASSERT(plugin_.my_, mongodb_plugin_not_enabled_exception, "Mongodb plugin is not enabled.");

    auto dire = (params.dire.valid() && *params.dire == direction::asc) ? direction::asc : direction::desc;
    return plugin_.my_->get_transactions(params.keys, dire, params.skip, params.take);
}

}  // namespace history_apis

}  // namespace evt