/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <boost/foreach.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/token_database.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>
#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <rocksdb/sst_file_manager.h>

namespace evt { namespace chain {

using namespace evt::chain;

namespace __internal {

const size_t PKEY_SIZE = sizeof(public_key_type::storage_type::type_at<0>);

template <typename T>
struct db_key {
    db_key(const char* prefix, const T& t)
        : prefix(prefix)
        , slice((const char*)this, 16 + sizeof(T)) {
        static_assert(sizeof(name128) == 16, "Not valid prefix size");
        static_assert(sizeof(T) == 16, "Not valid value type");

        memcpy(data, &t, sizeof(T));
    }

    db_key(name128 prefix, const T& t)
        : prefix(prefix)
        , slice((const char*)this, 16 + sizeof(T)) {
        memcpy(data, &t, sizeof(T));
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    name128 prefix;
    char    data[sizeof(T)];

    rocksdb::Slice slice;
};

struct db_asset_key {
    db_asset_key(const address& addr, symbol symbol)
        : slice((const char*)this, sizeof(buf)) {
        addr.to_bytes(buf, PKEY_SIZE);
        memcpy(buf + PKEY_SIZE, &symbol, sizeof(symbol));
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    char buf[PKEY_SIZE + sizeof(symbol)];

    rocksdb::Slice slice;
};

db_key<domain_name>
get_domain_key(const domain_name& name) {
    return db_key<domain_name>(N128(.domain), name);
}

db_key<token_name>
get_token_key(const domain_name& domain, const token_name& name) {
    return db_key<token_name>(domain, name);
}

db_key<group_name>
get_group_key(const group_name& name) {
    return db_key<group_name>(N128(.group), name);
}

db_key<proposal_name>
get_suspend_key(const proposal_name& suspend) {
    return db_key<proposal_name>(N128(.suspend), suspend);
}

db_key<fungible_name>
get_fungible_key(const symbol sym) {
    return db_key<fungible_name>(N128(.fungible), (fungible_name)sym.name());
}

db_key<fungible_name>
get_fungible_key(const fungible_name& sym_name) {
    return db_key<fungible_name>(N128(.fungible), sym_name);
}

db_asset_key
get_asset_key(const address& addr, const asset& asset) {
    return db_asset_key(addr, asset.get_symbol());
}

db_asset_key
get_asset_key(const address& addr, const symbol symbol) {
    return db_asset_key(addr, symbol);
}

rocksdb::Slice
get_asset_prefix_key(const address& addr) {
    return rocksdb::Slice((const char*)&addr.get_public_key(), PKEY_SIZE);
}

template <typename T>
std::string
get_value(const T& v) {
    auto value = std::string();
    auto        size = fc::raw::pack_size(v);
    value.resize(size);
    auto ds = fc::datastream<char*>((char*)value.data(), size);
    fc::raw::pack(ds, v);
    return value;
}

template <typename T, typename V>
T
read_value(const V& value) {
    T    v;
    auto ds = fc::datastream<const char*>(value.data(), value.size());
    fc::raw::unpack(ds, v);
    return v;
}

enum dbaction_type {
    kNone = 0,
    kNewDomain,
    kIssueToken,
    kNewGroup,
    kNewSuspend,
    kNewFungible,

    kUpdateDomain,
    kUpdateGroup,
    kUpdateToken,
    kUpdateSuspend,
    kUpdateFungible,
    kUpdateAsset
};

struct sp_domain {
    domain_name name;
};

struct sp_group {
    group_name name;
};

struct sp_token {
    domain_name domain;
    token_name  name;
};

struct sp_suspend {
    proposal_name name;
};

struct sp_fungible {
    symbol sym;
};

struct sp_asset {
    char key[PKEY_SIZE + sizeof(symbol)];
};

struct sp_issuetoken {
    domain_name domain;
    size_t      size;
    token_name  names[0];
};

}  // namespace __internal

token_database::token_database(const fc::path& dbpath)
    : token_database() {
    initialize(dbpath);
}

token_database::~token_database() {
    if(db_ != nullptr) {
        if(tokens_handle_ != nullptr) {
            delete tokens_handle_;
            tokens_handle_ = nullptr;
        }
        if(assets_handle_ != nullptr) {
            delete assets_handle_;
            assets_handle_ = nullptr;
        }

        delete db_;
        db_ = nullptr;
    }
}

int
token_database::initialize(const fc::path& dbpath) {
    using namespace rocksdb;
    using namespace __internal;

    static std::string AssetsColumnFamilyName = "Assets";

    assert(db_ == nullptr);
    Options options;
    options.OptimizeUniversalStyleCompaction();

    auto tokens_plain_table_opts = PlainTableOptions();
    auto assets_plain_table_opts = PlainTableOptions();
    tokens_plain_table_opts.user_key_len = sizeof(name128) + sizeof(name128);
    assets_plain_table_opts.user_key_len = PKEY_SIZE + sizeof(symbol);

    options.create_if_missing      = true;
    options.compression            = CompressionType::kLZ4Compression;
    options.bottommost_compression = CompressionType::kZSTD;
    options.table_factory.reset(NewPlainTableFactory(tokens_plain_table_opts));
    options.prefix_extractor.reset(NewFixedPrefixTransform(sizeof(name128)));
    // options.sst_file_manager.reset(NewSstFileManager(Env::Default()));

    auto assets_opts = ColumnFamilyOptions(options);
    assets_opts.table_factory.reset(NewPlainTableFactory(assets_plain_table_opts));
    assets_opts.prefix_extractor.reset(NewFixedPrefixTransform(PKEY_SIZE));

    read_opts_.prefix_same_as_start = true;

    auto native_path = dbpath.to_native_ansi_path();
    if(!fc::exists(dbpath)) {
        // create new databse and open
        fc::create_directories(dbpath);

        auto status = DB::Open(options, native_path, &db_);
        if(!status.ok()) {
            EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }

        status = db_->CreateColumnFamily(assets_opts, AssetsColumnFamilyName, &assets_handle_);
        if(!status.ok()) {
            EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        return 0;
    }

    auto columns = std::vector<ColumnFamilyDescriptor>();
    columns.emplace_back(kDefaultColumnFamilyName, options);
    columns.emplace_back(AssetsColumnFamilyName, assets_opts);

    auto handles = std::vector<ColumnFamilyHandle*>();

    auto status = DB::Open(options, native_path, columns, &handles, &db_);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }

    asset(handles.size() == 2);
    tokens_handle_ = handles[0];
    assets_handle_ = handles[1];

    return 0;
}

int
token_database::add_domain(const domain_def& domain) {
    using namespace __internal;
    auto key    = get_domain_key(domain.name);
    auto value  = get_value(domain);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_domain*)malloc(sizeof(sp_domain));
        act->name = domain.name;
        record(kNewDomain, act);
    }
    return 0;
}

int
token_database::exists_domain(const domain_name& name) const {
    using namespace __internal;
    auto key    = get_domain_key(name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::issue_tokens(const issuetoken& issue) {
    using namespace __internal;
    if(!exists_domain(issue.domain)) {
        EVT_THROW(tokendb_domain_not_found, "Cannot find domain: ${name}", ("name", (std::string)issue.domain));
    }
    rocksdb::WriteBatch batch;
    for(auto name : issue.names) {
        auto key   = get_token_key(issue.domain, name);
        auto value = get_value(token_def(issue.domain, name, issue.owner));
        batch.Put(key.as_slice(), value);
    }
    auto status = db_->Write(write_opts_, &batch);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act    = (sp_issuetoken*)malloc(sizeof(sp_issuetoken) + sizeof(token_name) * issue.names.size());
        act->domain = issue.domain;
        act->size   = issue.names.size();
        memcpy(act->names, &issue.names[0], sizeof(token_name) * act->size);
        record(kIssueToken, act);
    }
    return 0;
}

int
token_database::exists_token(const domain_name& domain, const token_name& name) const {
    using namespace __internal;
    auto key    = get_token_key(domain, name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::add_group(const group_def& group) {
    using namespace __internal;
    auto key    = get_group_key(group.name());
    auto value  = get_value(group);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_group*)malloc(sizeof(sp_group));
        act->name = group.name();
        record(kNewGroup, act);
    }
    return 0;
}

int
token_database::exists_group(const group_name& name) const {
    using namespace __internal;
    auto key    = get_group_key(name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::add_suspend(const suspend_def& suspend) {
    using namespace __internal;
    auto key    = get_suspend_key(suspend.name);
    auto value  = get_value(suspend);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_suspend*)malloc(sizeof(sp_suspend));
        act->name = suspend.name;
        record(kNewSuspend, act);
    }
    return 0;
}

int
token_database::exists_suspend(const proposal_name& name) const {
    using namespace __internal;
    auto key    = get_suspend_key(name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::add_fungible(const fungible_def& fungible) {
    using namespace __internal;
    auto key    = get_fungible_key(fungible.sym);
    auto value  = get_value(fungible);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_fungible*)malloc(sizeof(sp_fungible));
        act->sym = fungible.sym;
        record(kNewFungible, act);
    }
    return 0;
}

int
token_database::exists_fungible(const symbol sym) const {
    using namespace __internal;
    auto key    = get_fungible_key(sym);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::exists_fungible(const fungible_name& sym_name) const {
    using namespace __internal;
    auto key    = get_fungible_key(sym_name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::update_asset(const address& addr, const asset& asset) {
    using namespace __internal;
    auto key    = get_asset_key(addr, asset);
    auto value  = get_value(asset);
    auto status = db_->Put(write_opts_, assets_handle_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_asset*)malloc(sizeof(sp_asset));
        memcpy(act->key, key.buf, sizeof(key.buf));
        record(kUpdateAsset, act);
    }
    return 0;
}

int
token_database::exists_any_asset(const address& addr) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_, assets_handle_);
    auto key = get_asset_prefix_key(addr);
    it->Seek(key);

    auto existed = it->Valid();
    delete it;

    return existed;
}

int
token_database::exists_asset(const address& addr, const symbol symbol) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_, assets_handle_);
    auto key = get_asset_key(addr, symbol);
    it->Seek(key.as_slice());

    auto existed = it->Valid() && it->key().compare(key.as_slice()) == 0;
    delete it;

    return existed;
}

int
token_database::read_domain(const domain_name& name, domain_def& domain) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_domain_key(name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_domain_not_found, "Cannot find domain: ${name}", ("name",name));
    }
    domain = read_value<domain_def>(value);
    return 0;
}

int
token_database::read_token(const domain_name& domain, const token_name& name, token_def& token) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_token_key(domain, name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_token_not_found, "Cannot find token: ${domain}-${name}", ("domain",domain)("name",name));
    }
    token = read_value<token_def>(value);
    return 0;
}

int
token_database::read_group(const group_name& id, group_def& group) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_group_key(id);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_group_not_found, "Cannot find group: ${id}", ("id",id));
    }
    group = read_value<group_def>(value);
    return 0;
}

int
token_database::read_suspend(const proposal_name& name, suspend_def& suspend) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_suspend_key(name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_suspend_not_found, "Cannot find suspend: ${name}", ("name",name));
    }
    suspend = read_value<suspend_def>(value);
    return 0;
}

int
token_database::read_fungible(const symbol sym, fungible_def& fungible) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_fungible_key(sym);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_fungible_not_found, "Cannot find fungible def: ${sym}", ("sym",sym));
    }
    fungible = read_value<fungible_def>(value);
    return 0;
}

int
token_database::read_fungible(const fungible_name& sym_name, fungible_def& fungible) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_fungible_key(sym_name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_fungible_not_found, "Cannot find fungible def: ${name}", ("name",sym_name));
    }
    fungible = read_value<fungible_def>(value);
    return 0;
}

int
token_database::read_asset(const address& addr, const symbol symbol, asset& v) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_, assets_handle_);
    auto key = get_asset_key(addr, symbol);
    it->Seek(key.as_slice());

    if(!it->Valid() || it->key().compare(key.as_slice()) != 0) {
        delete it;
        EVT_THROW(tokendb_asset_not_found, "Cannot find fungible: ${sym} in address: {addr}", ("sym",symbol)("addr",addr));
    }
    v = read_value<asset>(it->value());
    delete it;
    return 0;
}

int
token_database::read_asset_no_throw(const address& addr, const symbol symbol, asset& v) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_, assets_handle_);
    auto key = get_asset_key(addr, symbol);
    it->Seek(key.as_slice());

    if(!it->Valid() || it->key().compare(key.as_slice()) != 0) {
        delete it;
        v = asset(0, symbol);
        return 0;
    }
    v = read_value<asset>(it->value());
    delete it;
    return 0;
}

int
token_database::read_all_assets(const address& addr, const read_fungible_func& func) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_, assets_handle_);
    auto key = get_asset_prefix_key(addr);
    it->Seek(key);

    while(it->Valid()) {
        auto v = read_value<asset>(it->value());
        if(!func(v)) {
            break;
        }
        it->Next();
    }
    delete it;
    return 0;
}

int
token_database::update_domain(const domain_def& domain) {
    using namespace __internal;
    auto key    = get_domain_key(domain.name);
    auto value  = get_value(domain);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_domain*)malloc(sizeof(sp_domain));
        act->name = domain.name;
        record(kUpdateDomain, act);
    }
    return 0;
}

int
token_database::update_group(const group& group) {
    using namespace __internal;
    auto key    = get_group_key(group.name());
    auto value  = get_value(group);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_group*)malloc(sizeof(sp_group));
        act->name = group.name();
        record(kUpdateGroup, act);
    }
    return 0;
}

int
token_database::update_token(const token_def& token) {
    using namespace __internal;
    auto key    = get_token_key(token.domain, token.name);
    auto value  = get_value(token);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act    = (sp_token*)malloc(sizeof(sp_token));
        act->domain = token.domain;
        act->name   = token.name;
        record(kUpdateToken, act);
    }
    return 0;
}

int
token_database::update_suspend(const suspend_def& suspend) {
    using namespace __internal;
    auto key    = get_suspend_key(suspend.name);
    auto value  = get_value(suspend);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_suspend*)malloc(sizeof(sp_suspend));
        act->name = suspend.name;
        record(kUpdateSuspend, act);
    }
    return 0;
}

int
token_database::update_fungible(const fungible_def& fungible) {
    using namespace __internal;
    auto key    = get_fungible_key(fungible.sym);
    auto value  = get_value(fungible);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_fail, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_fungible*)malloc(sizeof(sp_fungible));
        act->sym = fungible.sym;
        record(kUpdateFungible, act);
    }
    return 0;
}

int
token_database::record(int type, void* data) {
    if(!should_record()) {
        return 0;
    }
    savepoints_.back().actions.emplace_back(dbaction{.type = type, .data = data});
    return 0;
}

token_database::session
token_database::new_savepoint_session(int seq) {
    add_savepoint(seq);
    return session(*this, seq);
}

token_database::session
token_database::new_savepoint_session() {
    auto seq = 1;
    if(!savepoints_.empty()) {
        seq = savepoints_.back().seq + 1;
    }
    add_savepoint(seq);
    return session(*this, seq);
}

int
token_database::add_savepoint(int32_t seq) {
    if(!savepoints_.empty()) {
        auto& b = savepoints_.back();
        if(b.seq >= seq) {
            EVT_THROW(tokendb_seq_not_valid, "Seq is not valid, prev: ${prev}, curr: ${curr}",
                      ("prev", b.seq)("curr", seq));
        }
    }
    savepoints_.emplace_back(savepoint{.seq = seq, .rb_snapshot = (const void*)db_->GetSnapshot(), .actions = {}});
    return 0;
}

int
token_database::free_savepoint(savepoint& cp) {
    for(auto& act : cp.actions) {
        free(act.data);
    }
    db_->ReleaseSnapshot((const rocksdb::Snapshot*)cp.rb_snapshot);
    return 0;
}

int
token_database::pop_savepoints(int32_t until) {
    if(savepoints_.empty()) {
        EVT_THROW(tokendb_no_savepoint, "There's no savepoints anymore");
    }
    while(!savepoints_.empty() && savepoints_.front().seq < until) {
        auto it = std::move(savepoints_.front());
        savepoints_.pop_front();
        free_savepoint(it);
    }
    return 0;
}

int
token_database::pop_back_savepoint() {
    if(savepoints_.empty()) {
        EVT_THROW(tokendb_no_savepoint, "There's no savepoints anymore");
    }
    auto it = std::move(savepoints_.back());
    savepoints_.pop_back();
    free_savepoint(it);
    return 0;
}

int
token_database::rollback_to_latest_savepoint() {
    using namespace __internal;

    if(savepoints_.empty()) {
        EVT_THROW(tokendb_no_savepoint, "There's no savepoints anymore");
    }
    auto& cp = savepoints_.back();
    if(cp.actions.size() > 0) {
        auto snapshot_read_opts_     = read_opts_;
        snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)cp.rb_snapshot;
        rocksdb::WriteBatch batch;
        for(auto it = --cp.actions.end(); it >= cp.actions.begin(); it--) {
            switch(it->type) {
            case kNewDomain: {
                auto act = (sp_domain*)it->data;
                auto key = get_domain_key(act->name);
                batch.Delete(key.as_slice());
                break;
            }
            case kIssueToken: {
                auto act = (sp_issuetoken*)it->data;
                for(size_t i = 0; i < act->size; i++) {
                    auto key = get_token_key(act->domain, act->names[i]);
                    batch.Delete(key.as_slice());
                }
                break;
            }
            case kNewGroup: {
                auto act = (sp_group*)it->data;
                auto key = get_group_key(act->name);
                batch.Delete(key.as_slice());
                break;
            }
            case kNewSuspend: {
                auto act = (sp_suspend*)it->data;
                auto key = get_suspend_key(act->name);
                batch.Delete(key.as_slice());
                break;
            }
            case kNewFungible: {
                auto act = (sp_fungible*)it->data;
                auto key = get_fungible_key(act->sym);
                batch.Delete(key.as_slice());
                break;
            }
            case kUpdateDomain: {
                auto act = (sp_domain*)it->data;
                auto key = get_domain_key(act->name);

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);

                FC_ASSERT(status.ok(), "Not expected rocksdb status: ${status}", ("status",status.getState()));
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateGroup: {
                auto act = (sp_group*)it->data;
                auto key = get_group_key(act->name);

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);

                FC_ASSERT(status.ok(), "Not expected rocksdb status: ${status}", ("status",status.getState()));
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateToken: {
                auto act = (sp_token*)it->data;
                auto key = get_token_key(act->domain, act->name);

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);

                FC_ASSERT(status.ok(), "Not expected rocksdb status: ${status}", ("status",status.getState()));
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateSuspend: {
                auto act = (sp_suspend*)it->data;
                auto key = get_suspend_key(act->name);

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);

                FC_ASSERT(status.ok(), "Not expected rocksdb status: ${status}", ("status",status.getState()));
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateFungible: {
                auto act = (sp_fungible*)it->data;
                auto key = get_fungible_key(act->sym);

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);

                FC_ASSERT(status.ok(), "Not expected rocksdb status: ${status}", ("status",status.getState()));
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateAsset: {
                auto act = (sp_asset*)it->data;
                auto key = rocksdb::Slice(act->key, sizeof(act->key));

                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, assets_handle_, key, &old_value);
                if(!status.ok()) {
                    // key may not existed in latest snapshot, remove it
                    FC_ASSERT(status.code() == rocksdb::Status::kNotFound, "Not expected rocksdb status: ${status}", ("status",status.getState()));
                    batch.Delete(assets_handle_, key);
                    break;
                }
                batch.Put(assets_handle_, key, old_value);
                break;
            }
            default: {
                FC_ASSERT(false, "Unexpected action type: ${type}", ("type", it->type));
                break;
            }
            }  // switch
            free(it->data);
        }  // for
        auto sync_write_opts = write_opts_;
        sync_write_opts.sync = true;
        db_->Write(sync_write_opts, &batch);
    }  // if

    db_->ReleaseSnapshot((const rocksdb::Snapshot*)cp.rb_snapshot);
    savepoints_.pop_back();
    return 0;
}

}}  // namespace evt::chain
