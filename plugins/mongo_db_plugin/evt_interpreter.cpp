/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/mongo_db_plugin/evt_interpreter.hpp>
#include <evt/chain/contracts/types.hpp>

#include <fc/io/json.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>

namespace evt {

using namespace evt::chain::contracts;
using mongocxx::bulk_write;

class interpreter_impl {
public:
    void initialize_db(const mongocxx::database& db);
    void process_trx(const transaction_trace& trx_trace);

private:
    void process_newdomain(const newdomain& nd);
    void process_updatedomain(const updatedomain& ud);
    void process_issuetoken(const issuetoken& it);
    void process_transfer(const transfer& tt);
    void process_newgroup(const newgroup& ng);
    void process_updategroup(const updategroup& ug);
    void process_newfungible(const newfungible& nf);
    void process_updfungible(const updfungible& uf);
    void process_issuefungible(const issuefungible& ifact);

private:
    mongocxx::database    db_;
    mongocxx::collection  domains_collection_;
    mongocxx::collection  tokens_collection_;
    mongocxx::collection  groups_collection_;
    mongocxx::collection  fungibles_collection_;

public:
    static const std::string domains_col;
    static const std::string tokens_col;
    static const std::string groups_col;
    static const std::string fungibles_col;
};

const std::string interpreter_impl::domains_col  = "Domains";
const std::string interpreter_impl::tokens_col   = "Tokens";
const std::string interpreter_impl::groups_col   = "Groups";
const std::string interpreter_impl::fungibles_col = "Fungibles";

void
interpreter_impl::initialize_db(const mongocxx::database& db)
{
    db_ = db;
    domains_collection_   = db_[domains_col];
    tokens_collection_    = db_[tokens_col];
    groups_collection_    = db_[groups_col];
    fungibles_collection_ = db_[fungibles_col];
}

#define CASE_N_CALL(name)                        \
    case N(name): {                              \
        process_##name(act.data_as<name>()); \
        break;                                   \
    }

void
interpreter_impl::process_trx(const transaction_trace& trx_trace) {
    for(auto& act_trace : trx_trace.action_traces) {
        auto& act = act_trace.act;
        switch((uint64_t)act.name) {
            CASE_N_CALL(newdomain)
            CASE_N_CALL(updatedomain)
            CASE_N_CALL(issuetoken)
            CASE_N_CALL(transfer)
            CASE_N_CALL(newgroup)
            CASE_N_CALL(updategroup)
            CASE_N_CALL(newfungible)
            CASE_N_CALL(updfungible)
            CASE_N_CALL(issuefungible)
            default: break;
        }
    }
}

namespace __internal {

auto
find_domain(mongocxx::collection& domains, const std::string& name) {
    using bsoncxx::builder::stream::document;
    document find{};
    find << "name" << name;
    auto domain = domains.find_one(find.view());
    if(!domain) {
        FC_THROW("Unable to find domain ${name}", ("name", name));
    }
    return *domain;
}

auto
find_token(mongocxx::collection& tokens, const std::string& domain, const std::string& name) {
    using bsoncxx::builder::stream::document;
    document find{};
    find << "token_id" << domain + "-" + name;
    auto token = tokens.find_one(find.view());
    if(!token) {
        FC_THROW("Unable to find token ${domain}-${name}", ("domain",domain)("name", name));
    }
    return *token;
}

auto
find_group(mongocxx::collection& groups, const std::string& name) {
    using bsoncxx::builder::stream::document;
    document find{};
    find << "name" << name;
    auto group = groups.find_one(find.view());
    if(!group) {
        FC_THROW("Unable to find group ${name}", ("name", name));
    }
    return *group;
}

auto
find_fungible(mongocxx::collection& fungibles, const std::string& sym) {
    using bsoncxx::builder::stream::document;
    document find{};
    find << "sym" << sym;
    auto fungible = fungibles.find_one(find.view());
    if(!fungible) {
        FC_THROW("Unable to find fungible assets ${sym}", ("sym", sym));
    }
    return *fungible;
}

}  // namespace __internal

void
interpreter_impl::process_newdomain(const newdomain& nd) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    auto oid = bsoncxx::oid{};
    auto doc = bsoncxx::builder::basic::document{};

    fc::variant issue, transfer, manage;
    fc::to_variant(nd.issue, issue);
    fc::to_variant(nd.transfer, transfer);
    fc::to_variant(nd.manage, manage);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    doc.append(kvp("_id", oid),
               kvp("name", (std::string)nd.name),
               kvp("creator", (std::string)nd.creator),
               kvp("issue", bsoncxx::from_json(fc::json::to_string(issue))),
               kvp("transfer", bsoncxx::from_json(fc::json::to_string(transfer))),
               kvp("manage", bsoncxx::from_json(fc::json::to_string(manage))));
    doc.append(kvp("created_at", b_date{now}));

    if(!domains_collection_.insert_one(doc.view())) {
        elog("Failed to insert domain ${name}", ("name", nd.name));
    }
}

void
interpreter_impl::process_updatedomain(const updatedomain& ud) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto name = (std::string)ud.name;
    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto d = find_domain(domains_collection_, name);

    document update{};
    update << "$set" << open_document;
    if(ud.issue.valid()) {
        fc::variant u;
        fc::to_variant(*ud.issue, u);
        update << "issue" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(ud.transfer.valid()) {
        fc::variant u;
        fc::to_variant(*ud.transfer, u);
        update << "transfer" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(ud.manage.valid()) {
        fc::variant u;
        fc::to_variant(*ud.manage, u);
        update << "manage" << bsoncxx::from_json(fc::json::to_string(u));
    }

    update << "updated_at" << b_date{now}
           << close_document;

    if(!domains_collection_.update_one(document{} << "_id" << d.view()["_id"].get_oid() << finalize, update.view())) {
        elog("Failed to update domain ${name}", ("name", ud.name));
    }
}

void
interpreter_impl::process_issuetoken(const issuetoken& it) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    mongocxx::bulk_write bulk{};
    auto d = (std::string)it.domain;
    for(auto& n : it.names) {
        auto doc = bsoncxx::builder::basic::document{};
        auto oid = bsoncxx::oid{};
        auto name = (std::string)n;
        doc.append(kvp("_id", oid),
                   kvp("token_id", d + "-" + name),
                   kvp("domain", d),
                   kvp("name", name),
                   /// TODO: Cache owner list to improve performance
                   kvp("owner", [&it](bsoncxx::builder::basic::sub_array subarr) {
                       for(const auto& o : it.owner) {
                           subarr.append((std::string)o);
                       }
                   }));
        doc.append(kvp("created_at", b_date{now}));
        mongocxx::model::insert_one insert_op{doc.view()};
        bulk.append(insert_op);
    }

    if(!tokens_collection_.bulk_write(bulk)) {
        elog("Bulk insert tokens failed for domain: ${name}", ("name", d));
    }
}

void
interpreter_impl::process_transfer(const transfer& tt) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto t = find_token(tokens_collection_, (std::string)tt.domain, (std::string)tt.name);

    auto update = bsoncxx::builder::basic::document{};
    update.append(kvp("owner", [&tt](bsoncxx::builder::basic::sub_array subarr) {
           for(const auto& o : tt.to) {
               subarr.append((std::string)o);
           }
       }));

    update.append(kvp("updated_at", b_date{now}));
    if(!tokens_collection_.update_one(document{} << "_id" << t.view()["_id"].get_oid() << finalize, update.view())) {
        elog("Failed to update token: ${domain}-${name}", ("domain",tt.domain)("name", tt.name));
    }
}

void
interpreter_impl::process_newgroup(const newgroup& ng) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    auto oid = bsoncxx::oid{};
    auto doc = bsoncxx::builder::basic::document{};

    fc::variant def;
    fc::to_variant(ng.group, def);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    doc.append(kvp("_id", oid),
               kvp("name", (std::string)ng.name),
               kvp("def", bsoncxx::from_json(fc::json::to_string(def))));
    doc.append(kvp("created_at", b_date{now}));

    if(!groups_collection_.insert_one(doc.view())) {
        elog("Failed to insert group ${name}", ("name", ng.name));
    }
}

void
interpreter_impl::process_updategroup(const updategroup& ug) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto name = (std::string)ug.name;
    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto g = find_group(groups_collection_, name);

    document update{};
    update << "$set" << open_document;
    fc::variant u;
    fc::to_variant(ug.group, u);
    update << "def" << bsoncxx::from_json(fc::json::to_string(u));
    update << "updated_at" << b_date{now}
           << close_document;

    if(!groups_collection_.update_one(document{} << "_id" << g.view()["_id"].get_oid() << finalize, update.view())) {
        elog("Failed to update group ${name}", ("name", ug.name));
    }
}

void
interpreter_impl::process_newfungible(const newfungible& nf) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    auto oid = bsoncxx::oid{};
    auto doc = bsoncxx::builder::basic::document{};

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    fc::variant issue, manage;
    fc::to_variant(nf.issue, issue);
    fc::to_variant(nf.manage, manage);

    auto cp = evt::chain::asset(0, nf.total_supply.get_symbol());

    doc.append(kvp("_id", oid),
               kvp("sym", (std::string)nf.sym),
               kvp("creator", (std::string)nf.creator),
               kvp("issue", bsoncxx::from_json(fc::json::to_string(issue))),
               kvp("manage", bsoncxx::from_json(fc::json::to_string(manage))),
               kvp("total_supply", (std::string)nf.total_supply),
               kvp("current_supply", (std::string)cp));
    doc.append(kvp("created_at", b_date{now}));

    if(!fungibles_collection_.insert_one(doc.view())) {
        elog("Failed to insert fungible assets ${sym}", ("sym", nf.sym));
    }
}

void
interpreter_impl::process_updfungible(const updfungible& uf) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto sym = (std::string)uf.sym;
    auto f   = find_fungible(fungibles_collection_, sym);

    document update{};
    update << "$set" << open_document;
    if(uf.issue.valid()) {
        fc::variant u;
        fc::to_variant(*uf.issue, u);
        update << "issue" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(uf.manage.valid()) {
        fc::variant u;
        fc::to_variant(*uf.manage, u);
        update << "manage" << bsoncxx::from_json(fc::json::to_string(u));
    }

    update << "updated_at" << b_date{now}
           << close_document;

    if(!fungibles_collection_.update_one(document{} << "_id" << f.view()["_id"].get_oid() << finalize, update.view())) {
        elog("Failed to update fungible assets ${sym}", ("sym", sym));
    }
}

namespace __internal {

std::string
get_bson_string_value(const bsoncxx::document::view& view, const std::string& key) {
    auto v = (bsoncxx::stdx::string_view)view[key].get_utf8();
    return std::string(v.data(), v.size());
}

}  // namespace __internal

void
interpreter_impl::process_issuefungible(const issuefungible& ifact) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto sym = (std::string)ifact.number.get_symbol();
    auto f   = find_fungible(fungibles_collection_, sym);

    auto cs = get_bson_string_value(f.view(), "current_supply");
    auto csasset = evt::chain::asset::from_string(cs);
    csasset += ifact.number;

    document update{};
    update << "$set" << open_document
           << "current_supply" << (std::string)csasset
           << "updated_at" << b_date{now}
           << close_document;

    if(!fungibles_collection_.update_one(document{} << "_id" << f.view()["_id"].get_oid() << finalize, update.view())) {
        elog("Failed to update fungible assets ${sym}", ("sym", sym));
    }
}

evt_interpreter::evt_interpreter() : my_(new interpreter_impl()) {}

void
evt_interpreter::initialize_db(const mongocxx::database& db) {
    my_->initialize_db(db);
}

void
evt_interpreter::process_trx(const transaction_trace& trx_trace) {
    my_->process_trx(trx_trace);
}

}  // namespace evt