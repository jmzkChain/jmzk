/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/token_database.hpp>

#include <unordered_set>
#include <fstream>

#define XXH_INLINE_ALL
#include <xxhash.h>

#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <rocksdb/sst_file_manager.h>

#include <boost/container/flat_map.hpp>

#include <fc/filesystem.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>

#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

using namespace evt::chain;

namespace __internal {

const size_t PKEY_SIZE = sizeof(public_key_type::storage_type::type_at<0>);

struct db_key : boost::noncopyable {
    template<typename T>
    db_key(const char* prefix, const T& t)
        : prefix(prefix)
        , slice((const char*)this, 16 + sizeof(T)) {
        static_assert(sizeof(name128) == 16, "Not valid prefix size");
        static_assert(sizeof(T) == 16, "Not valid value type");

        memcpy(data, &t, sizeof(T));
    }

    template<typename T>
    db_key(name128 prefix, const T& t)
        : prefix(prefix)
        , slice((const char*)this, 16 + sizeof(T)) {
        memcpy(data, &t, sizeof(T));
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    std::string
    as_string() const {
        return std::string((const char*)this, 16 + 16);
    }

    name128 prefix;
    char    data[16];

    rocksdb::Slice slice;
};

struct db_asset_key : boost::noncopyable {
    db_asset_key(const address& addr, symbol symbol)
        : slice((const char*)this, sizeof(buf)) {
        addr.to_bytes(buf, PKEY_SIZE);
        memcpy(buf + PKEY_SIZE, &symbol, sizeof(symbol));
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    std::string
    as_string() const {
        return std::string((const char*)this, sizeof(buf));
    }

    char buf[PKEY_SIZE + sizeof(symbol)];

    rocksdb::Slice slice;
};

struct db_asset_prefix_key {
    db_asset_prefix_key(const address& addr)
        : slice((const char*)this, sizeof(buf)) {
        addr.to_bytes(buf, PKEY_SIZE);
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    char buf[PKEY_SIZE];

    rocksdb::Slice slice;
};

struct key_hasher {
    size_t
    operator()(const std::string& key) const {
        return XXH64(key.data(), key.size(), 0);
    }
};

using key_unordered_set = std::unordered_set<std::string, key_hasher>;

inline db_key
get_domain_key(const domain_name& name) {
    return db_key(N128(.domain), name);
}

inline db_key
get_token_key(const domain_name& domain, const token_name& name) {
    return db_key(domain, name);
}

inline db_key
get_group_key(const group_name& name) {
    return db_key(N128(.group), name);
}

inline db_key
get_suspend_key(const proposal_name& suspend) {
    return db_key(N128(.suspend), suspend);
}

inline db_key
get_fungible_key(const symbol sym) {
    return db_key(N128(.fungible), (uint128_t)sym.id());
}

inline db_key
get_fungible_key(const symbol_id_type& sym_id) {
    return db_key(N128(.fungible), (uint128_t)sym_id);
}

inline db_key
get_prodvote_key(const conf_key& key) {
    return db_key(N128(.prodvote), key);
}

inline db_asset_key
get_asset_key(const address& addr, const asset& asset) {
    return db_asset_key(addr, asset.sym());
}

inline db_asset_key
get_asset_key(const address& addr, const symbol symbol) {
    return db_asset_key(addr, symbol);
}

inline db_asset_prefix_key
get_asset_prefix_key(const address& addr) {
    return db_asset_prefix_key(addr);
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

enum action_type {
    kRT = 0,
    kPD = 1
};

enum rt_action_type {
    kNone = 0,

    kNewDomain = 1,
    kUpdateDomain = 2,

    kIssueToken = 3,
    kUpdateToken = 4,

    kNewGroup = 5,
    kUpdateGroup = 6,

    kNewSuspend = 7,
    kUpdateSuspend = 8,

    kNewFungible = 9,
    kUpdateFungible = 10,

    kUpdateAsset = 11,

    kUpdateProdVote = 13
};

enum pd_action_type {
    kRevert = 0,
    kRemove,
    kRemoveOrRevert
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
    symbol_id_type sym_id;
};

struct sp_asset {
    char key[PKEY_SIZE + sizeof(symbol)];
};

struct sp_issuetoken {
    domain_name domain;
    size_t      size;
    token_name  names[0];
};

struct sp_prodvote {
    conf_key key;
};

}  // namespace __internal

token_database::token_database(const fc::path& dbpath)
    : token_database() {
    initialize(dbpath);
}

token_database::~token_database() {
    persist_savepoints();
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

    db_path_ = dbpath.to_native_ansi_path();
    if(!fc::exists(db_path_)) {
        // create new databse and open
        fc::create_directories(db_path_);

        auto status = DB::Open(options, db_path_, &db_);
        if(!status.ok()) {
            EVT_THROW(tokendb_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }

        status = db_->CreateColumnFamily(assets_opts, AssetsColumnFamilyName, &assets_handle_);
        if(!status.ok()) {
            EVT_THROW(tokendb_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }

        load_savepoints();
        return 0;
    }

    auto columns = std::vector<ColumnFamilyDescriptor>();
    columns.emplace_back(kDefaultColumnFamilyName, options);
    columns.emplace_back(AssetsColumnFamilyName, assets_opts);

    auto handles = std::vector<ColumnFamilyHandle*>();

    auto status = DB::Open(options, db_path_, columns, &handles, &db_);
    if(!status.ok()) {
        EVT_THROW(tokendb_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }

    assert(handles.size() == 2);
    tokens_handle_ = handles[0];
    assets_handle_ = handles[1];

    load_savepoints();
    return 0;
}

int
token_database::add_domain(const domain_def& domain) {
    using namespace __internal;
    auto key    = get_domain_key(domain.name);
    auto value  = get_value(domain);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_fungible*)malloc(sizeof(sp_fungible));
        act->sym_id = fungible.sym.id();
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
token_database::exists_fungible(const symbol_id_type sym_id) const {
    using namespace __internal;
    auto key    = get_fungible_key(sym_id);
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
    it->Seek(key.as_slice());

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
token_database::update_prodvote(const conf_key& key, const public_key_type& pkey, int64_t value) {
    using namespace __internal;
    using boost::container::flat_map;

    auto dbkey  = get_prodvote_key(key);
    auto v      = std::string();
    auto map    = flat_map<public_key_type, int64_t>();
    auto status = db_->Get(read_opts_, dbkey.as_slice(), &v);
    if(!status.ok()) {
        if(status.code() != rocksdb::Status::kNotFound) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        map.emplace(pkey, value);
    }
    else {
        map = read_value<decltype(map)>(v);
        map.emplace(pkey, value);
    }
    v = get_value(map);
    
    status = db_->Put(write_opts_, dbkey.as_slice(), v);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_prodvote*)malloc(sizeof(sp_prodvote));
        act->key = key;
        record(kUpdateProdVote, act);
    }
    return 0;
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
token_database::read_fungible(const symbol_id_type sym_id, fungible_def& fungible) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_fungible_key(sym_id);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(tokendb_fungible_not_found, "Cannot find fungible def: ${id}", ("id",sym_id));
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
    it->Seek(key.as_slice());

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
token_database::read_prodvotes_no_throw(const conf_key& key, const read_prodvote_func& func) const {
    using namespace __internal;
    using boost::container::flat_map;

    auto value  = std::string();
    auto dbkey  = get_prodvote_key(key);
    auto status = db_->Get(read_opts_, dbkey.as_slice(), &value);
    if(!status.ok() && status.code() != rocksdb::Status::kNotFound) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(value.empty()) {
        return 0;
    }

    auto map = read_value<flat_map<public_key_type, int64_t>>(value);
    for(auto& it : map) {
        if(!func(it.first, it.second)) {
            break;
        }
    }
    return 0;
}

int
token_database::update_domain(const domain_def& domain) {
    using namespace __internal;
    auto key    = get_domain_key(domain.name);
    auto value  = get_value(domain);
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
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
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (sp_fungible*)malloc(sizeof(sp_fungible));
        act->sym_id = fungible.sym.id();
        record(kUpdateFungible, act);
    }
    return 0;
}

int
token_database::record(int type, void* data) {
    using namespace __internal;

    if(!should_record()) {
        return 0;
    }
    auto n = savepoints_.back().node;
    FC_ASSERT(n.f.type == kRT);

    GETPOINTER(rt_group, n.group)->actions.emplace_back(rt_action(type, data));
    return 0;
}

token_database::session
token_database::new_savepoint_session(int64_t seq) {
    add_savepoint(seq);
    return session(*this, seq);
}

token_database::session
token_database::new_savepoint_session() {
    int64_t seq = 1;
    if(!savepoints_.empty()) {
        seq = savepoints_.back().seq + 1;
    }

    add_savepoint(seq);
    return session(*this, seq);
}

int
token_database::add_savepoint(int64_t seq) {
    using namespace __internal;

    if(!savepoints_.empty()) {
        auto& b = savepoints_.back();
        if(b.seq >= seq) {
            EVT_THROW(tokendb_seq_not_valid, "Seq is not valid, prev: ${prev}, curr: ${curr}",
                      ("prev", b.seq)("curr", seq));
        }
    }

    savepoints_.emplace_back(savepoint(seq, kRT));
    auto rt = new rt_group { .rb_snapshot = (const void*)db_->GetSnapshot(), .actions = {} }; 
    SETPOINTER(void, savepoints_.back().node.group, rt);

    return 0;
}

int
token_database::free_savepoint(savepoint& sp) {
    using namespace __internal;

    auto n = sp.node;

    switch(n.f.type) {
    case kRT: {
        auto rt = GETPOINTER(rt_group, n.group);
        for(auto& act : rt->actions) {
            free(GETPOINTER(void, act.data));
        }
        db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
        delete rt;
        break;
    }
    case kPD: {
        auto pd = GETPOINTER(pd_group, n.group);
        delete pd;
        break;
    }
    default: {
        FC_ASSERT(false);
    }
    }  // switch

    return 0;
}

int
token_database::pop_savepoints(int64_t until) {
    while(!savepoints_.empty() && savepoints_.front().seq < until) {
        auto it = std::move(savepoints_.front());
        savepoints_.pop_front();
        free_savepoint(it);
    }
    return 0;
}

int
token_database::pop_back_savepoint() {
    EVT_ASSERT(!savepoints_.empty(), tokendb_no_savepoint, "There's no savepoints anymore");

    auto it = std::move(savepoints_.back());
    savepoints_.pop_back();
    free_savepoint(it);
    return 0;
}

int
token_database::squash() {
    using namespace __internal;
    EVT_ASSERT(savepoints_.size() >= 2, tokendb_squash_exception, "Squash needs at least two savepoints.");
    
    auto n = savepoints_.back().node;
    EVT_ASSERT(n.f.type == kRT, tokendb_squash_exception, "Squash needs two realtime savepoints.");

    savepoints_.pop_back();
    auto n2 = savepoints_.back().node;
    EVT_ASSERT(n2.f.type == kRT, tokendb_squash_exception, "Squash needs two realtime savepoints.");

    auto rt1 = GETPOINTER(rt_group, n.group);
    auto rt2 = GETPOINTER(rt_group, n2.group);

    // add all actions from rt1 into end of rt2
    rt2->actions.insert(rt2->actions.cend(), rt1->actions.cbegin(), rt1->actions.cend());

    // just release rt1's snapshot
    db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt1->rb_snapshot);
    delete rt1;

    return 0;
}

namespace __internal {

std::string
get_sp_keyop(const token_database::rt_action& it, int& op) {
    // for the db action type, New-Ops are odd and Update-Ops are even
    // for Update-Ops return kRevert(0) and returns kRemove(1) for New-Ops
    op = it.f.type % 2;

    switch(it.f.type) {
    case kNewDomain:
    case kUpdateDomain: {
        auto act = GETPOINTER(sp_domain, it.data);
        return get_domain_key(act->name).as_string();
    }
    case kUpdateToken: {
        auto act = GETPOINTER(sp_token, it.data);
        return get_token_key(act->domain, act->name).as_string();
    }
    case kNewGroup:
    case kUpdateGroup: {
        auto act = GETPOINTER(sp_group, it.data);
        return get_group_key(act->name).as_string();
    }
    case kNewSuspend:
    case kUpdateSuspend: {
        auto act = GETPOINTER(sp_suspend, it.data);
        return get_suspend_key(act->name).as_string();
    }
    case kNewFungible:
    case kUpdateFungible: {
        auto act = GETPOINTER(sp_fungible, it.data);
        return get_fungible_key(act->sym_id).as_string();
    }
    case kUpdateAsset: {
        op = kRemoveOrRevert;
        auto act = GETPOINTER(sp_asset, it.data);
        return std::string(act->key, sizeof(act->key));
    }
    case kUpdateProdVote: {
        op = kRemoveOrRevert;
        auto act = GETPOINTER(sp_prodvote, it.data);
        return get_prodvote_key(act->key).as_string();
    }
    default: {
        FC_ASSERT(false);
    }
    }  // switch
}

}  // namespace __internal

int
token_database::rollback_rt_group(rt_group* rt) {
    using namespace __internal;

    if(rt->actions.empty()) {
        db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
        return 0;
    }

    auto snapshot_read_opts_     = read_opts_;
    snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)rt->rb_snapshot;

    auto key_set = key_unordered_set();
    key_set.reserve(rt->actions.size());
    
    auto batch = rocksdb::WriteBatch();
    for(auto it = rt->actions.begin(); it < rt->actions.end(); it++) {
        auto data = GETPOINTER(void, it->data);

        if(it->f.type == kIssueToken) {
            // special process issue token action, cuz it's multiple keys
            auto act = (sp_issuetoken*)data;
            for(size_t i = 0; i < act->size; i++) {
                auto  key = get_token_key(act->domain, act->names[i]).as_string();
                batch.Delete(key);

                // insert key into key set
                key_set.emplace(std::move(key));
            }

            free(data);
            continue;
        }

        auto op  = 0;
        auto key = get_sp_keyop(*it, op);

        switch(op) {
        case kRevert: {
            // only update operation need to check if key is already processed
            if(key_set.find(key) != key_set.cend()) {
                break;
            }
            auto old_value = std::string();
            auto status    = db_->Get(snapshot_read_opts_, key, &old_value);
            if(!status.ok()) {
                FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
            }
            batch.Put(key, old_value);

            // insert key into key set
            key_set.emplace(std::move(key));
            break;
        }
        case kRemove: {
            FC_ASSERT(key_set.find(key) == key_set.cend());

            batch.Delete(key);
        
            // insert key into key set
            key_set.emplace(std::move(key));
            break;
        }
        case kRemoveOrRevert: {
            // only update operation need to check if key is already processed
            if(key_set.find(key) != key_set.cend()) {
                break;
            }

            auto old_value = std::string();
            if(it->f.type == kUpdateAsset) {
                auto status = db_->Get(snapshot_read_opts_, assets_handle_, key, &old_value);

                // key may not existed in latest snapshot, remove it
                if(!status.ok()) {
                    if(status.code() != rocksdb::Status::kNotFound) {
                        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                    }
                    batch.Delete(assets_handle_, key);
                }
                else {
                    batch.Put(assets_handle_, key, old_value);
                }
            }
            else {
                auto status = db_->Get(snapshot_read_opts_, key, &old_value);

                // key may not existed in latest snapshot, remove it
                if(!status.ok()) {
                    if(status.code() != rocksdb::Status::kNotFound) {
                        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                    }
                    batch.Delete(key);
                }
                else {
                    batch.Put(key, old_value);
                }
            }

            // insert key into key set
            key_set.emplace(std::move(key));
            break;
        }
        default: {
            FC_ASSERT(false);
        }
        }  // switch

        free(data);
    }  // for

    auto sync_write_opts = write_opts_;
    sync_write_opts.sync = true;
    db_->Write(sync_write_opts, &batch);

    db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
    return 0;
}

int
token_database::rollback_pd_group(pd_group* pd) {
    using namespace __internal;

    if(pd->actions.empty()) {
        return 0;
    }

    auto key_set = key_unordered_set();
    key_set.reserve(pd->actions.size());

    auto batch = rocksdb::WriteBatch();

    for(auto it = pd->actions.begin(); it < pd->actions.end(); it++) {
        switch(it->op) {
        case kRevert: {
            if(key_set.find(it->key) != key_set.cend()) {
                break;
            }

            FC_ASSERT(!it->value.empty());
            batch.Put(it->key, it->value);

            key_set.emplace(it->key);
            break;
        }
        case kRemove: {
            FC_ASSERT(key_set.find(it->key) == key_set.cend());

            FC_ASSERT(it->value.empty());
            batch.Delete(it->key);

            key_set.emplace(it->key);
            break;
        }
        case kRemoveOrRevert: {
            if(key_set.find(it->key) != key_set.cend()) {
                break;
            }

            if(it->type == kUpdateAsset) {
                // update asset action
                if(it->value.empty()) {
                    batch.Delete(assets_handle_, it->key);
                }
                else {
                    batch.Put(assets_handle_, it->key, it->value);
                }
            }
            else {
                if(it->value.empty()) {
                    batch.Delete(it->key);
                }
                else {
                    batch.Put(it->key, it->value);
                }
            }

            key_set.emplace(it->key);
            break;
        }
        default: {
            FC_ASSERT(false);
        }
        }  // switch
    }

    auto sync_write_opts = write_opts_;
    sync_write_opts.sync = true;
    db_->Write(sync_write_opts, &batch);

    return 0;
}

int
token_database::rollback_to_latest_savepoint() {
    using namespace __internal;
    EVT_ASSERT(!savepoints_.empty(), tokendb_no_savepoint, "There's no savepoints anymore");

    auto n = savepoints_.back().node;

    switch(n.f.type) {
    case kRT: {
        auto rt = GETPOINTER(rt_group, n.group);
        rollback_rt_group(rt);
        delete rt;

        break;
    }
    case kPD: {
        auto pd = GETPOINTER(pd_group, n.group);
        rollback_pd_group(pd);
        delete pd;

        break;
    }
    default: {
        FC_ASSERT(false);
    }
    }  // switch

    savepoints_.pop_back();
    return 0;
}

namespace __internal {

struct pd_header {
    int dirty_flag;
};

}  // namespace __internal

int
token_database::persist_savepoints() {
    using namespace __internal;

    auto filename = fc::path(db_path_) / "savepoints.log";
    if(fc::exists(filename)) {
        fc::remove(filename);
    }
    auto fs = std::fstream();
    fs.exceptions(std::fstream::failbit | std::fstream::badbit);
    fs.open(filename.to_native_ansi_path(), (std::ios::out | std::ios::binary));

    auto h = pd_header {
        .dirty_flag = 1
    };
    // set dirty first
    fc::raw::pack(fs, h);

    auto pds = std::vector<pd_group>();

    for(auto& sp : savepoints_) {
        auto pd = pd_group();
        pd.seq  = sp.seq;

        auto n = sp.node;

        switch(n.f.type) {
        case kPD: {
            auto pd2 = GETPOINTER(pd_group, n.group);
            pd.actions.insert(pd.actions.cbegin(), pd2->actions.cbegin(), pd2->actions.cend());
            
            delete pd2;
            break;
        }
        case kRT: {
            auto rt = GETPOINTER(rt_group, n.group);

            auto key_set = key_unordered_set();
            key_set.reserve(rt->actions.size());

            auto snapshot_read_opts_     = read_opts_;
            snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)rt->rb_snapshot;

            for(auto& act : rt->actions) {
                auto data = GETPOINTER(void, act.data);

                if(act.f.type == kIssueToken) {
                    // special process issue token action, cuz it's multiple keys
                    auto itact = (sp_issuetoken*)data;
                    for(size_t i = 0; i < itact->size; i++) {
                        auto key   = get_token_key(itact->domain, itact->names[i]).as_string();
                        auto pdact = pd_action();
                        pdact.op   = kRemove;
                        pdact.type = act.f.type;
                        pdact.key  = key;

                        pd.actions.emplace_back(std::move(pdact));
                        key_set.emplace(std::move(key));
                    }

                    free(data);
                    continue;
                }

                auto pdact = pd_action();
                int  op    = 0;
                auto key   = get_sp_keyop(act, op);

                pdact.op   = op;
                pdact.type = act.f.type;
                pdact.key  = key;

                switch(op) {
                case kRevert: {
                    if(key_set.find(key) != key_set.cend()) {
                        break;
                    }

                    auto status = db_->Get(snapshot_read_opts_, key, &pdact.value);
                    if(!status.ok()) {
                        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                    }

                    key_set.emplace(std::move(key));
                    break;
                }
                case kRemove: {
                    FC_ASSERT(key_set.find(key) == key_set.cend());

                    // no need to read value
                    key_set.emplace(std::move(key));
                    break;
                }
                case kRemoveOrRevert: {
                    if(key_set.find(key) != key_set.cend()) {
                        break;
                    }

                    rocksdb::Status status;
                    if(act.f.type == kUpdateAsset) {
                        status = db_->Get(snapshot_read_opts_, assets_handle_, key, &pdact.value);
                    }
                    else {
                        status = db_->Get(snapshot_read_opts_, key, &pdact.value);
                    }
                    
                    // key may not existed in latest snapshot
                    if(!status.ok() && status.code() != rocksdb::Status::kNotFound) {
                        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                    }

                    key_set.emplace(std::move(key));
                    break;
                }
                }  // switch
                
                pd.actions.emplace_back(std::move(pdact));
                free(data);
            }

            db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
            delete rt;

            break;
        }
        default: {
            FC_ASSERT(false);
        }
        }  // switch

        pds.emplace_back(std::move(pd));
    }  // for

    fc::raw::pack(fs, pds);
    fs.seekp(0);

    h.dirty_flag = 0;
    fc::raw::pack(fs, h);

    fs.flush();
    fs.close();

    return 0;
}

int
token_database::load_savepoints() {
    using namespace __internal;

    auto filename = fc::path(db_path_) / "savepoints.log";
    if(!fc::exists(filename)) {
        wlog("No savepoints log in token database");
        return 0;
    }

    auto fs = std::fstream();
    fs.exceptions(std::fstream::failbit | std::fstream::badbit);
    fs.open(filename.to_native_ansi_path(), (std::ios::in | std::ios::binary));

    auto h = pd_header();
    fc::raw::unpack(fs, h);
    EVT_ASSERT(h.dirty_flag == 0, tokendb_dirty_flag_exception, "checkpoints log file dirty flag set");

    auto pds = std::vector<pd_group>();
    fc::raw::unpack(fs, pds);

    for(auto& pd : pds) {
        savepoints_.emplace_back(savepoint(pd.seq, kPD));

        auto ppd = new pd_group(pd);
        SETPOINTER(void, savepoints_.back().node.group, ppd);
    }
    fs.close();

    return 0;
}

}}  // namespace evt::chain

FC_REFLECT(evt::chain::__internal::pd_header, (dirty_flag));
FC_REFLECT(evt::chain::token_database::pd_action, (op)(type)(key)(value));
FC_REFLECT(evt::chain::token_database::pd_group, (seq)(actions));
