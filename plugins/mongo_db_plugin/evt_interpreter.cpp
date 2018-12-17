/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/mongo_db_plugin/evt_interpreter.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/address.hpp>

#include <mutex>

#include <fc/io/json.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
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
    void process_trx(const transaction& trx, write_context& write_ctx);

private:
    void process_newdomain(const newdomain& nd, write_context& write_ctx);
    void process_updatedomain(const updatedomain& ud, write_context& write_ctx);
    void process_issuetoken(const issuetoken& it, write_context& write_ctx);
    void process_transfer(const transfer& tt, write_context& write_ctx);
    void process_newgroup(const newgroup& ng, write_context& write_ctx);
    void process_updategroup(const updategroup& ug, write_context& write_ctx);
    void process_newfungible(const newfungible& nf, write_context& write_ctx);
    void process_updfungible(const updfungible& uf, write_context& write_ctx);
    void process_issuefungible(const issuefungible& ifact, write_context& write_ctx);
    void process_destroytoken(const destroytoken& dt, write_context& write_ctx);
    void process_everipass(const everipass& ep, write_context& write_ctx);
    void process_addmeta(const chain::action& act, write_context& write_ct);

private:
    mongocxx::database db_;
};

void
interpreter_impl::initialize_db(const mongocxx::database& db) {
    db_ = db;
}

#define CASE_N_CALL(name, write_ctx)                           \
    case N(name): {                                            \
        process_##name(act.data_as<const name&>(), write_ctx); \
        break;                                                 \
    }

void
interpreter_impl::process_trx(const transaction& trx, write_context& write_ctx) {
    for(auto& act : trx.actions) {
        switch((uint64_t)act.name) {
            CASE_N_CALL(newdomain, write_ctx)
            CASE_N_CALL(updatedomain, write_ctx)
            CASE_N_CALL(issuetoken, write_ctx)
            CASE_N_CALL(transfer, write_ctx)
            CASE_N_CALL(destroytoken, write_ctx)
            CASE_N_CALL(newgroup, write_ctx)
            CASE_N_CALL(updategroup, write_ctx)
            CASE_N_CALL(newfungible, write_ctx)
            CASE_N_CALL(updfungible, write_ctx)
            CASE_N_CALL(issuefungible, write_ctx)
            CASE_N_CALL(everipass, write_ctx)

            case N(addmeta): {
                process_addmeta(act, write_ctx);
                break;
            }
            default: break;
        }
    }
}

namespace __internal {

auto
find_domain(const std::string& name) {
    using namespace bsoncxx::builder::stream;

    document find{};
    find << "name" << name;

    return find;
}

auto
find_token(const std::string& domain, const std::string& name) {
    using namespace bsoncxx::builder::stream;

    document find{};
    find << "token_id" << domain + ":" + name;

    return find;
}

auto
find_group(const std::string& name) {
    using namespace bsoncxx::builder::stream;

    document find{};
    find << "name" << name;

    return find;
}

auto
find_fungible(const symbol_id_type sym_id) {
    using namespace bsoncxx::builder::stream;

    document find{};
    find << "sym_id" << (int64_t)sym_id;

    return find;
}

}  // namespace __internal

void
interpreter_impl::process_newdomain(const newdomain& nd, write_context& write_ctx) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
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

    write_ctx.get_domains().append(insert_one(doc.view()));
}

void
interpreter_impl::process_updatedomain(const updatedomain& ud, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;

    auto name = (std::string)ud.name;
    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    document update{};
    update << "$set" << open_document;
    if(ud.issue.has_value()) {
        fc::variant u;
        fc::to_variant(*ud.issue, u);
        update << "issue" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(ud.transfer.has_value()) {
        fc::variant u;
        fc::to_variant(*ud.transfer, u);
        update << "transfer" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(ud.manage.has_value()) {
        fc::variant u;
        fc::to_variant(*ud.manage, u);
        update << "manage" << bsoncxx::from_json(fc::json::to_string(u));
    }

    update << "updated_at" << b_date{now}
           << close_document;

    write_ctx.get_domains().append(update_one(find_domain((std::string)ud.name).view(), update.view()));
}

void
interpreter_impl::process_issuetoken(const issuetoken& it, write_context& write_ctx) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto d = (std::string)it.domain;

    auto owners = bsoncxx::builder::basic::array();
    for(const auto& o : it.owner) {
        owners.append((std::string)o);
    }

    for(auto& n : it.names) {
        auto doc = bsoncxx::builder::basic::document{};
        auto oid = bsoncxx::oid{};
        auto name = (std::string)n;
        doc.append(kvp("_id", oid),
                   kvp("token_id", d + ":" + name),
                   kvp("domain", d),
                   kvp("name", name),
                   kvp("owner", owners));
        doc.append(kvp("created_at", b_date{now}));

        write_ctx.get_tokens().append(insert_one(doc.view()));
    }
}

void
interpreter_impl::process_transfer(const transfer& tt, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto update = document();
    auto owners = bsoncxx::builder::stream::array();

    for(const auto& o : tt.to) {
        owners << (std::string)o;
    }

    update << "$set" << open_document 
           << "owner" << owners
           << "updated_at" << b_date{now}
           << close_document;

    write_ctx.get_tokens().append(
        update_one(find_token((std::string)tt.domain, (std::string)tt.name).view(), update.view()));
}

void
interpreter_impl::process_destroytoken(const destroytoken& dt, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto update = document();
    auto owners = bsoncxx::builder::stream::array();

    owners << evt::chain::address().to_string();

    update << "$set" << open_document 
           << "owner" << owners
           << "updated_at" << b_date{now}
           << close_document;

    write_ctx.get_tokens().append(
        update_one(find_token((std::string)dt.domain, (std::string)dt.name).view(), update.view()));
}

void
interpreter_impl::process_newgroup(const newgroup& ng, write_context& write_ctx) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
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

    write_ctx.get_groups().append(insert_one(doc.view()));
}

void
interpreter_impl::process_updategroup(const updategroup& ug, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto name = (std::string)ug.name;
    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    document update{};
    update << "$set" << open_document;
    fc::variant u;
    fc::to_variant(ug.group, u);
    update << "def" << bsoncxx::from_json(fc::json::to_string(u));
    update << "updated_at" << b_date{now}
           << close_document;

    write_ctx.get_groups().append(update_one(find_group(name).view(), update.view()));
}

void
interpreter_impl::process_newfungible(const newfungible& nf, write_context& write_ctx) {
    using namespace evt::chain;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto oid = bsoncxx::oid{};
    auto doc = bsoncxx::builder::basic::document{};

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    fc::variant issue, manage;
    fc::to_variant(nf.issue, issue);
    fc::to_variant(nf.manage, manage);

    doc.append(kvp("_id", oid),
               kvp("name", (std::string)nf.name),
               kvp("sym_name", (std::string)nf.sym_name),
               kvp("sym", (std::string)nf.sym),
               kvp("sym_id", (int64_t)nf.sym.id()),
               kvp("creator", (std::string)nf.creator),
               kvp("issue", bsoncxx::from_json(fc::json::to_string(issue))),
               kvp("manage", bsoncxx::from_json(fc::json::to_string(manage))),
               kvp("total_supply", (std::string)nf.total_supply));
    doc.append(kvp("created_at", b_date{now}));

    write_ctx.get_fungibles().append(insert_one(doc.view()));
}

void
interpreter_impl::process_updfungible(const updfungible& uf, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;

    auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto id = uf.sym_id;

    document update{};
    update << "$set" << open_document;
    if(uf.issue.has_value()) {
        fc::variant u;
        fc::to_variant(*uf.issue, u);
        update << "issue" << bsoncxx::from_json(fc::json::to_string(u));
    }
    if(uf.manage.has_value()) {
        fc::variant u;
        fc::to_variant(*uf.manage, u);
        update << "manage" << bsoncxx::from_json(fc::json::to_string(u));
    }

    update << "updated_at" << b_date{now}
           << close_document;

    write_ctx.get_fungibles().append(update_one(find_fungible(id).view(), update.view()));
}

void
interpreter_impl::process_everipass(const everipass& ep, write_context& write_ctx){
    auto  link  = ep.link;
    auto  flags = link.get_header();
    auto& d     = *link.get_segment(evt_link::domain).strv;
    auto& t     = *link.get_segment(evt_link::token).strv;

    if(flags & evt_link::destroy) {
        auto dt   = destroytoken();
        dt.domain = d;
        dt.name   = t;

        process_destroytoken(dt, write_ctx);
    }
}

void
interpreter_impl::process_addmeta(const chain::action& act, write_context& write_ctx) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::open_document;

    auto am = act.data_as<const addmeta&>();

    document meta{};
    meta << (std::string)am.key << am.value;

    document update{};
    update << "$set" << open_document;
    update << "metas" << meta;
    update << close_document;

    if(act.domain == N128(.group)) {
        write_ctx.get_groups().append(update_one(find_group((std::string)act.key).view(), update.view()));
    }
    else if(act.domain == N128(.fungible)) {
        auto id = (uint64_t)std::stoull((std::string)act.key);
        write_ctx.get_fungibles().append(update_one(find_fungible(id).view(), update.view()));
    }
    else if(act.key == N128(.meta)) {
        write_ctx.get_domains().append(update_one(find_domain((std::string)act.domain).view(), update.view()));
    }
    else {
        write_ctx.get_tokens().append(update_one(find_token((std::string)act.domain, (std::string)act.key).view(), update.view()));
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
interpreter_impl::process_issuefungible(const issuefungible& ifact, write_context& write_ctx) {
    return;
}

evt_interpreter::evt_interpreter() : my_(new interpreter_impl()) {}

void
evt_interpreter::initialize_db(const mongocxx::database& db) {
    my_->initialize_db(db);
}

void
evt_interpreter::process_trx(const transaction& trx, write_context& write_ctx) {
    my_->process_trx(trx, write_ctx);
}

}  // namespace evt