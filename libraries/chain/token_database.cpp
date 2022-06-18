/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/token_database.hpp>

#ifndef __cpp_lib_string_view
#define __cpp_lib_string_view
#endif

#include <deque>
#include <fstream>
#include <string_view>
#include <unordered_set>

#include <rocksdb/db.h>
#include <rocksdb/cache.h>
#include <rocksdb/options.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/statistics.h>
#include <rocksdb/table.h>

#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/StringMap.h>

#include <fc/filesystem.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>
#include <fc/container/ring_vector.hpp>

#include <jmzk/chain/config.hpp>
#include <jmzk/chain/exceptions.hpp>

namespace jmzk { namespace chain {

namespace internal {

// Use only lower 48-bit address for some pointers.
/*
    This optimization uses the fact that current X86-64 architecture only implement lower 48-bit virtual address.
    The higher 16-bit can be used for storing other data.
*/
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define SETPOINTER(type, p, x) (p = reinterpret_cast<type *>((reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(0xFFFF000000000000UL)) | reinterpret_cast<uintptr_t>(reinterpret_cast<const void*>(x))))
#define GETPOINTER(type, p) (reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(0x0000FFFFFFFFFFFFUL)))
#else
#error jmzk can only be compiled in X86-64 architecture
#endif

const char*  kAssetsColumnFamilyName = "Assets";
const size_t kSymbolIdSize           = sizeof(symbol_id_type);
const size_t kPublicKeySize          = sizeof(fc::ecc::public_key_shim);
const size_t kDefaultSavePointsSize  = (4 / 3 * 24 + 1) * 12;

struct db_token_key : boost::noncopyable {
public:
    db_token_key(const name128& prefix, const name128& key)
        : prefix(prefix)
        , key(key)
        , slice((const char*)this, sizeof(name128) * 2) {}

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    std::string_view
    as_string_view() const {
        return std::string_view((const char*)this, 16 + 16);
    }

    std::string
    as_string() const {
        return std::string((const char*)this, 16 + 16);
    }

private:
    name128 prefix;
    name128 key;

    rocksdb::Slice slice;
};

struct db_asset_key : boost::noncopyable {
public:
    db_asset_key(const address& addr, symbol_id_type sym_id)
        : slice((const char*)this, sizeof(buf)) {
        memcpy(buf, &sym_id, kSymbolIdSize);
        addr.to_bytes(buf + kSymbolIdSize, kPublicKeySize);
    }

    const rocksdb::Slice&
    as_slice() const {
        return slice;
    }

    std::string_view
    as_string_view() const {
        return std::string_view((const char*)this, sizeof(buf));
    }

    std::string
    as_string() const {
        return std::string((const char*)this, sizeof(buf));
    }

private:
    char buf[kSymbolIdSize + kPublicKeySize];

    rocksdb::Slice slice;
};

name128 action_key_prefixes[] = {
    N128(.asset),
    N128(.domain),
    N128(.token),
    N128(.group),
    N128(.suspend),
    N128(.lock),
    N128(.fungible),
    N128(.prodvote),
    N128(.jmzklink),
    N128(.psvbonus),
    N128(.psvbonus-dist),
    N128(.validator),
    N128(.stakepool),
    N128(.script)
};

static_assert(sizeof(action_key_prefixes) / sizeof(name128) == (int)token_type::max_value + 1);

using keys_hash_set = llvm::StringSet<llvm::MallocAllocator>;

struct flag {
public:
    flag() = default;
    flag(uint8_t type, uint8_t exts) : type(type), exts(exts) {}

public:
    char    payload[6];
    uint8_t type;
    uint8_t exts;
};

enum action_type {
    kRuntime = 0,
    kPersist
};

enum data_type {
    kTokenKey = 0,
    kTokenFullKey,
    kAssetKey,
    kTokenKeys
};

// realtime action
// stored in memory
struct rt_action {
public:
    rt_action(uint8_t token_type, uint8_t op, uint8_t data_type, void* data) {
        assert(data_type < 8 && op < 8);
        f.type  = token_type;
        f.exts  = op;
        f.exts |= (data_type << 4);
        SETPOINTER(void, this->data, data);
    }

    token_type get_token_type() const { return (token_type)f.type; }
    action_op  get_action_op()  const { return (action_op)(f.exts & 0xF); }
    data_type  get_data_type()  const { return (data_type)(f.exts >> 4); }

public:
    union {
        flag  f;
        void* data;
    };
};

struct rt_group {
    const void*                rb_snapshot;
    small_vector<rt_action, 4> actions;
};

// persistent action
// stored in disk
struct pd_action {
    uint16_t    op;
    uint16_t    type;
    std::string key;
    std::string value;
};

struct pd_group {
    int64_t                seq; // used for persistent
    std::vector<pd_action> actions;
};

struct sp_node {
public:
    sp_node() = default;
    sp_node(uint8_t type) : f(type, 0) {}

public:
    union {
        flag  f;
        void* group;
    };
};

struct savepoint {
public:
    savepoint() = default;
    savepoint(int64_t seq, uint8_t type)
        : seq(seq), node(type) {}

public:
    int64_t seq;
    sp_node node;
};

struct rt_token_key {
    name128 key;
};

struct rt_token_fullkey {
    name128 prefix;
    name128 key;
};

struct rt_token_keys {
    name128      prefix;
    token_keys_t keys;
};

struct rt_asset_key {
    char key[kSymbolIdSize + kPublicKeySize];
};

struct pd_header {
    int dirty_flag;
};

}  // namespace internal

class write_cache_layer : boost::noncopyable {
private:
    struct cache_entry {
    public:
        cache_entry(int32_t used_count, std::string&& value)
            : used_count(used_count)
            , value(std::move(value)) {}

    public:
        int32_t     used_count;
        std::string value;
    };

    using data_map_t = llvm::StringMap<cache_entry>;

    struct data_op {
    public:
        data_op(data_map_t::iterator& it, std::string&& pv)
            : it(&(*it)), pv(std::move(pv)) {}

    public:
        data_map_t::value_type* it;
        std::string             pv;
    };

    struct data_ops {
        int64_t              seq;
        std::vector<data_op> vec;
    };

public:
    write_cache_layer() : ops_(internal::kDefaultSavePointsSize) {}

public:
    void put(const std::string_view& key, const std::string_view& value);
    int read(const std::string_view& key, std::string& value) const;
    int exists(const std::string_view& key) const;

public:
    void add_savepoint(int64_t seq);
    void rollback_to_latest_savepoint();
    void squash();
    void pop_front(std::function<void(const llvm::StringRef&, std::string&&)> persist_func);
    void pop_back();

    void clear();
    void persist_savepoints(std::ostream& os) const;
    void load_savepoints(std::istream& is);

private:
    data_map_t                data_;
    fc::ring_vector<data_ops> ops_;

private:
    friend class token_database_impl;
};

void
write_cache_layer::put(const std::string_view& key, const std::string_view& value) {
    assert(!ops_.empty());

    auto pair = data_.try_emplace(llvm::StringRef(key.data(), key.size()), 1, std::string(value.data(), value.size()));
    if(!pair.second) {
        auto pv = std::move(pair.first->second.value);
        pair.first->second.used_count += 1;
        pair.first->second.value       = std::string(value.data(), value.size());
        ops_.back().vec.emplace_back(data_op(pair.first, std::move(pv)));
        return;
    }
    ops_.back().vec.emplace_back(data_op(pair.first, std::string()));
}

int
write_cache_layer::read(const std::string_view& key, std::string& value) const {
    auto it = data_.find(llvm::StringRef(key.data(), key.size()));
    if(it == data_.end()) {
        return 0;
    }
    value = it->second.value;
    return 1;
}

int
write_cache_layer::exists(const std::string_view& key) const {
    return data_.find(llvm::StringRef(key.data(), key.size())) != data_.end();
}

void
write_cache_layer::add_savepoint(int64_t seq) {
    ops_.push_back(data_ops{ .seq = seq, .vec = {} });
}

void
write_cache_layer::rollback_to_latest_savepoint() {
    auto& ops = ops_.back();
    for(auto it = ops.vec.rbegin(); it != ops.vec.rend(); it++) {
        auto& op = *it;
        if(--op.it->second.used_count == 0) {
            data_.erase(op.it->first());
        }
        else {
            assert(!op.it->second.value.empty());
            op.it->second.value = std::move(op.pv);
        }
    }
    ops_.pop_back();
}

void
write_cache_layer::squash() {
    assert(ops_.size() >= 2);
    auto& b1 = ops_[ops_.size() - 1];
    auto& b2 = ops_[ops_.size() - 2];

    b2.vec.insert(b2.vec.end(), b1.vec.begin(), b1.vec.end());
    ops_.pop_back();
}

void
write_cache_layer::pop_front(std::function<void(const llvm::StringRef&, std::string&&)> persist_func) {
    for(auto& op : ops_.front().vec) {
        if(--op.it->second.used_count == 0) {
            persist_func(op.it->first(), std::move(op.it->second.value));
            data_.erase(op.it->first());
        }
    }

    ops_.pop_front();
}

void
write_cache_layer::pop_back() {
    ops_.pop_back();
}

namespace internal {

struct wc_entry {
    std::string k;
    std::string v;
};

struct wc_entry_pack {
    int64_t               seq;
    std::vector<wc_entry> vec;
};

}  // namespace internal

void
write_cache_layer::clear() {
    data_.clear();
    ops_.clear();
}

void
write_cache_layer::persist_savepoints(std::ostream& os) const {
    using namespace internal;

    auto pack = std::vector<wc_entry_pack>();
    pack.resize(ops_.size());
    for(auto i = 0; i < ops_.size(); i++) {
        auto& ops = ops_[i];
        
        auto& epack = pack[i];
        epack.seq   = ops.seq;
        for(auto& op : ops.vec) {
            epack.vec.emplace_back(wc_entry {
                .k  = op.it->first().str(),
                .v  = op.it->second.value
            });
        }
    }
    fc::raw::pack(os, pack);
}

void
write_cache_layer::load_savepoints(std::istream& is) {
    using namespace internal;

    auto pack = std::vector<wc_entry_pack>();
    fc::raw::unpack(is, pack);

    for(auto& ops : pack) {
        this->add_savepoint(ops.seq);
        for(auto& op : ops.vec) {
            this->put(op.k, op.v);
        }
    }
}

class token_database_impl : boost::noncopyable {
public:
    token_database_impl(token_database& self, const token_database::config& config);

public:
    void open(int load_persistence = true);
    void close(int persist = true);

public:
    void put_token(token_type type, action_op op, const name128& prefix, const name128& key, const std::string_view& data);
    void put_tokens(token_type type,
                    action_op op,
                    const name128& prefix,
                    token_keys_t&& keys,
                    const small_vector_base<std::string_view>& data);
    void put_asset(const address& addr, const symbol_id_type sym_id, const std::string_view& data);

    int exists_token(const name128& prefix, const name128& key) const;
    int exists_asset(const address& addr, const symbol_id_type sym_id) const;

    int read_token(const name128& prefix, const name128& key, std::string& out, bool no_throw = false) const;
    int read_asset(const address& addr, const symbol_id_type sym_id, std::string& out, bool no_throw = false) const;

    int read_tokens_range(const name128& prefix, int skip, const read_value_func& func) const;
    int read_assets_range(const symbol_id_type sym_id, int skip, const read_value_func& func) const;

public:
    void add_savepoint(int64_t seq);
    void rollback_to_latest_savepoint();
    void pop_savepoints(int64_t until);
    void pop_back_savepoint();
    void squash();

    int64_t latest_savepoint_seq() const;
    int64_t new_savepoint_session_seq() const;
    size_t  savepoints_size() const { return savepoints_.size(); }

    void rollback_rt_group(internal::rt_group*);
    void rollback_pd_group(internal::pd_group*);

    int should_record() { return !savepoints_.empty(); }

    void record(uint8_t action_type, uint8_t op, uint8_t data_type, void* data);
    void free_savepoint(internal::savepoint&);
    void free_all_savepoints();

    void persist_savepoints() const;
    void load_savepoints();
    void persist_savepoints(std::ostream&) const;
    void load_savepoints(std::istream&);
    void flush() const;

    std::string get_db_path() const { return config_.db_path.to_native_ansi_path(); }

public:
    token_database&        self_;
    token_database::config config_;

    rocksdb::DB*          db_;
    rocksdb::ReadOptions  read_opts_;
    rocksdb::WriteOptions write_opts_;

    rocksdb::ColumnFamilyHandle* tokens_handle_;
    rocksdb::ColumnFamilyHandle* assets_handle_;

    write_cache_layer assets_write_cache_;

    fc::ring_vector<internal::savepoint> savepoints_;
};

token_database_impl::token_database_impl(token_database& self, const token_database::config& config)
    : self_(self)
    , config_(config)
    , db_(nullptr)
    , read_opts_()
    , write_opts_()
    , tokens_handle_(nullptr)
    , assets_handle_(nullptr)
    , savepoints_(internal::kDefaultSavePointsSize) {}

void
token_database_impl::open(int load_persistence) {
    using namespace rocksdb;
    using namespace internal;

    jmzk_ASSERT(db_ == nullptr, token_database_exception, "Token database is already opened");

    auto options = Options();
    options.OptimizeUniversalStyleCompaction();

    options.create_if_missing               = true;
    options.compression                     = CompressionType::kLZ4Compression;
    options.bottommost_compression          = CompressionType::kZSTD;
    options.allow_concurrent_memtable_write = false;
    options.prefix_extractor.reset(NewFixedPrefixTransform(sizeof(name128)));
    options.memtable_factory.reset(NewHashSkipListRepFactory());
    if(config_.enable_stats) {
        options.statistics = rocksdb::CreateDBStatistics();
#if ROCKSDB_MAJOR >= 6
        options.statistics->set_stats_level(StatsLevel::kExceptTimeForMutex);
#else
        options.statistics->stats_level_ = StatsLevel::kExceptTimeForMutex;
#endif
    }

    auto assets_options = ColumnFamilyOptions(options);

    if(config_.profile == storage_profile::disk) {
        auto table_opts = BlockBasedTableOptions();

        table_opts.index_type     = BlockBasedTableOptions::kHashSearch;
        table_opts.checksum       = kxxHash64;
        table_opts.format_version = 4;
        table_opts.block_cache    = NewLRUCache(config_.block_cache_size);
        table_opts.filter_policy.reset(NewBloomFilterPolicy(10, false));

        options.table_factory.reset(NewBlockBasedTableFactory(table_opts));
        assets_options.prefix_extractor.reset(NewFixedPrefixTransform(kSymbolIdSize));
    }
    else if(config_.profile == storage_profile::memory) {
        auto tokens_table_options = PlainTableOptions();
        auto assets_table_options = PlainTableOptions();
        tokens_table_options.user_key_len = sizeof(name128) + sizeof(name128);
        assets_table_options.user_key_len = kPublicKeySize + kSymbolIdSize;

        options.table_factory.reset(NewPlainTableFactory(tokens_table_options));
        assets_options.table_factory.reset(NewPlainTableFactory(assets_table_options));
        assets_options.prefix_extractor.reset(NewFixedPrefixTransform(kSymbolIdSize));
    }
    else {
        jmzk_THROW(token_database_exception, "Unknown token database profile");
    }

    read_opts_.total_order_seek     = false;
    read_opts_.prefix_same_as_start = true;
    read_opts_.tailing              = true;

    auto columns = std::vector<ColumnFamilyDescriptor>();
    auto handles = std::vector<ColumnFamilyHandle*>();
    columns.emplace_back(kDefaultColumnFamilyName, options);

    if(!fc::exists(config_.db_path)) {
        auto t = config_.db_path.to_native_ansi_path();
        // create new database and open
        fc::create_directories(config_.db_path);
        
        auto status  = DB::Open(options, config_.db_path.to_native_ansi_path(), columns, &handles, &db_);
        if(!status.ok()) {
            jmzk_THROW(token_database_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }

        assert(handles.size() == 1);
        tokens_handle_ = handles[0];

        status = db_->CreateColumnFamily(assets_options, kAssetsColumnFamilyName, &assets_handle_);
        if(!status.ok()) {
            jmzk_THROW(token_database_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }

        if(load_persistence) {
            load_savepoints();
        }
        return;
    }

    columns.emplace_back(kAssetsColumnFamilyName, assets_options);

    auto status = DB::Open(options, config_.db_path.to_native_ansi_path(), columns, &handles, &db_);
    if(!status.ok()) {
        jmzk_THROW(token_database_rocksdb_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }

    assert(handles.size() == 2);
    tokens_handle_ = handles[0];
    assets_handle_ = handles[1];

    if(load_persistence) {
        load_savepoints();
    }
}

void
token_database_impl::close(int persist) {
    if(db_) {
        if(persist) {
            persist_savepoints();
        }
        if(!savepoints_.empty()) {
            free_all_savepoints();
        }
        
        delete tokens_handle_;
        delete assets_handle_;
        delete db_;

        db_ = nullptr;
    }
}

void
token_database_impl::put_token(token_type type, action_op op, const name128& prefix, const name128& key, const std::string_view& data) {
    using namespace internal;

    auto dbkey  = db_token_key(prefix, key);
    auto status = db_->Put(write_opts_, dbkey.as_slice(), data);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        void* data;

        // for `token` action, needs to record both prefix and key, prefix refers to the domain
        // for `non-token` action, prefix is not necessary which can be inferred by the `type`
        if(type != token_type::token) {
            assert(prefix == action_key_prefixes[(int)type]);
            auto data = (rt_token_key*)malloc(sizeof(rt_token_key));
            data->key = key;

            record((int)type, (int)op, (int)kTokenKey, data);
        }
        else {
            auto data    = (rt_token_fullkey*)malloc(sizeof(rt_token_fullkey));
            data->prefix = prefix;
            data->key    = key;

            record((int)type, (int)op, (int)kTokenFullKey, data);
        }
    }
}

void
token_database_impl::put_tokens(token_type type,
                                action_op op,
                                const name128& prefix,
                                token_keys_t&& keys,
                                const small_vector_base<std::string_view>& data){
    using namespace internal;
    assert(keys.size() == data.size());

    for(auto i = 0u; i < keys.size(); i++) {
        auto dbkey  = db_token_key(prefix, keys[i]);
        auto status = db_->Put(write_opts_, dbkey.as_slice(), data[i]);
        if(!status.ok()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
    }
    if(should_record()) {
        auto data = (rt_token_keys*)malloc(sizeof(rt_token_keys));
        data->prefix = prefix;
        new(&data->keys) token_keys_t(std::move(keys));

        record((int)type, (int)op, (int)kTokenKeys, data);
    }
}

void
token_database_impl::put_asset(const address& addr, const symbol_id_type sym_id, const std::string_view& data) {
    using namespace internal;

    auto dbkey = db_asset_key(addr, sym_id);
    if(should_record()) {
        assets_write_cache_.put(dbkey.as_string_view(), data);
        return;
    }
    else {
        auto status = db_->Put(write_opts_, assets_handle_, dbkey.as_slice(), data);
        if(!status.ok()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
    }
}

int
token_database_impl::exists_token(const name128& prefix, const name128& key) const {
    using namespace internal;

    auto dbkey  = db_token_key(prefix, key);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, dbkey.as_slice(), &value);
    return status.ok();
}

int
token_database_impl::exists_asset(const address& addr, const symbol_id_type sym_id) const {
    using namespace internal;

    auto dbkey  = db_asset_key(addr, sym_id);
    auto value  = std::string();

    if(assets_write_cache_.exists(dbkey.as_string_view())) {
        return true;
    }
    auto status = db_->Get(read_opts_, assets_handle_, dbkey.as_slice(), &value);
    return status.ok();
}

int
token_database_impl::read_token(const name128& prefix, const name128& key, std::string& out, bool no_throw) const {
    using namespace internal;

    auto dbkey  = db_token_key(prefix, key);
    auto status = db_->Get(read_opts_, dbkey.as_slice(), &out);
    if(!status.ok()) {
        if(!status.IsNotFound()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        if(!no_throw) {
            jmzk_THROW(unknown_token_database_key, "Cannot find key: ${k} with prefix: ${p}", ("k",key)("p",prefix));
        }
        return false;
    }
    return true;
}

int
token_database_impl::read_asset(const address& addr, const symbol_id_type sym_id, std::string& out, bool no_throw) const {
    using namespace internal;

    auto key = db_asset_key(addr, sym_id);
    if(assets_write_cache_.read(key.as_string_view(), out)) {
        return true;
    }

    auto status = db_->Get(read_opts_, assets_handle_, key.as_slice(), &out);
    if(!status.ok()) {
        if(!status.IsNotFound()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        if(!no_throw) {
            jmzk_THROW2(unknown_token_database_key, "There's no balance of fungible with sym id: {} in address: {}", sym_id, addr);
        }
        return false;
    }
    return true;
}

int
token_database_impl::read_tokens_range(const name128& prefix, int skip, const read_value_func& func) const {
    using namespace internal;

    auto it    = db_->NewIterator(read_opts_);
    auto key   = rocksdb::Slice((char*)&prefix, sizeof(prefix));
    auto i     = 0;
    auto count = 0;
    
    it->Seek(key);
    while(it->Valid()) {
        if(i++ < skip) {
            it->Next();
            continue;
        }

        count++;
        auto value = it->value().ToString();
        auto key   = it->key();

        key.remove_prefix(sizeof(prefix));
        if(!func(key.ToStringView(), std::move(value))) {
            delete it;
            return count;
        }
        it->Next();
    }
    delete it;
    return count;
}

int
token_database_impl::read_assets_range(const symbol_id_type sym_id, int skip, const read_value_func& func) const {
    using namespace internal;

    // create snapshot first
    auto ss = db_->GetSnapshot();
    
    // write new values from cache into db
    for(auto& it : assets_write_cache_.data_) {
        auto k = rocksdb::Slice(it.first().data(), it.first().size());
        auto v = rocksdb::Slice(it.second.value.data(), it.second.value.size());
        db_->Put(write_opts_, assets_handle_, k, v);
    }

    // scan values
    auto it    = db_->NewIterator(read_opts_, assets_handle_);
    auto key   = rocksdb::Slice((char*)&sym_id, sizeof(sym_id));
    auto count = 0;
    auto i     = 0;
    
    it->Seek(key);
    while(it->Valid()) {
        if(i++ < skip) {
            it->Next();
            continue;
        }

        count++;
        auto value = it->value().ToString();
        auto key   = it->key();

        key.remove_prefix(sizeof(sym_id));
        if(!func(key.ToStringView(), std::move(value))) {
            delete it;
            return count;
        }
        it->Next();
    }
    delete it;

    // restore values
    auto snapshot_read_opts_     = read_opts_;
    snapshot_read_opts_.snapshot = ss;

    auto batch = rocksdb::WriteBatch();
    for(auto& it : assets_write_cache_.data_) {
        auto v = std::string();
        auto k = rocksdb::Slice(it.first().data(), it.first().size());
        auto s = db_->Get(snapshot_read_opts_, assets_handle_, k, &v);
        if(s.ok()) {
            // put value back
            batch.Put(assets_handle_, k, v);
        }
        else if(s.code() == rocksdb::Status::kNotFound) {
            // remove value
            batch.Delete(assets_handle_, k);
        }
        else {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", s.getState()));
        }
    }
    auto sync_write_opts = write_opts_;
    sync_write_opts.sync = true;
    db_->Write(sync_write_opts, &batch);

    return count;
}

void
token_database_impl::add_savepoint(int64_t seq) {
    using namespace internal;

    if(!savepoints_.empty()) {
        auto& b = savepoints_.back();
        if(b.seq >= seq) {
            jmzk_THROW(token_database_seq_not_valid, "Seq is not valid, prev: ${prev}, curr: ${curr}",
                      ("prev", b.seq)("curr", seq));
        }
    }

    savepoints_.push_back(savepoint(seq, kRuntime));
    auto rt = new rt_group { .rb_snapshot = (const void*)db_->GetSnapshot(), .actions = {} }; 
    SETPOINTER(void, savepoints_.back().node.group, rt);

    assets_write_cache_.add_savepoint(seq);
}

void
token_database_impl::free_savepoint(internal::savepoint& sp) {
    using namespace internal;

    auto n = sp.node;

    switch(n.f.type) {
    case kRuntime: {
        auto rt = GETPOINTER(rt_group, n.group);
        for(auto& act : rt->actions) {
            switch(act.get_data_type()) {
            case kTokenKey:
            case kTokenFullKey:
            case kAssetKey: {
                free(GETPOINTER(void, act.data));
                break;
            }
            case kTokenKeys: {
                auto p = GETPOINTER(rt_token_keys, act.data);
                //need to call dtor of keys manually
                p->keys.~token_keys_t();
                free(p);
                break;
            }
            }  // switch
        }
        db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
        delete rt;
        break;
    }
    case kPersist: {
        auto pd = GETPOINTER(pd_group, n.group);
        delete pd;
        break;
    }
    }  // switch
}

void
token_database_impl::free_all_savepoints() {
    while(!savepoints_.empty()) {
        free_savepoint(savepoints_.front());
        savepoints_.pop_front();
    }
}

void
token_database_impl::pop_savepoints(int64_t until) {
    while(!savepoints_.empty() && savepoints_.front().seq < until) {
        auto it = std::move(savepoints_.front());
        savepoints_.pop_front();
        free_savepoint(it);

        // pop write cache and persist into underlying db
        assert(assets_write_cache_.ops_.front().seq == it.seq);
        auto batch = rocksdb::WriteBatch();
        assets_write_cache_.pop_front([&](auto& k, auto&& v) {
            batch.Put(assets_handle_, rocksdb::Slice(k.data(), k.size()), v);
        });
        auto sync_write_opts = write_opts_;
        sync_write_opts.sync = true;
        db_->Write(sync_write_opts, &batch);
    }
}

void
token_database_impl::pop_back_savepoint() {
    jmzk_ASSERT(!savepoints_.empty(), token_database_no_savepoint, "There's no savepoints anymore");

    auto it = std::move(savepoints_.back());
    savepoints_.pop_back();
    free_savepoint(it);

    assets_write_cache_.pop_back();
}

void
token_database_impl::squash() {
    using namespace internal;
    jmzk_ASSERT(savepoints_.size() >= 2, token_database_squash_exception, "Squash needs at least two savepoints.");
    
    auto n = savepoints_.back().node;
    jmzk_ASSERT(n.f.type == kRuntime, token_database_squash_exception, "Squash needs two realtime savepoints.");

    savepoints_.pop_back();
    auto n2 = savepoints_.back().node;
    jmzk_ASSERT(n2.f.type == kRuntime, token_database_squash_exception, "Squash needs two realtime savepoints.");

    auto rt1 = GETPOINTER(rt_group, n.group);
    auto rt2 = GETPOINTER(rt_group, n2.group);

    // add all actions from rt1 into end of rt2
    rt2->actions.insert(rt2->actions.cend(), rt1->actions.cbegin(), rt1->actions.cend());

    // just release rt1's snapshot
    db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt1->rb_snapshot);
    delete rt1;

    assets_write_cache_.squash();
}

int64_t
token_database_impl::latest_savepoint_seq() const {
    jmzk_ASSERT(!savepoints_.empty(), token_database_no_savepoint, "There's no savepoints anymore");
    return savepoints_.back().seq;
}

int64_t
token_database_impl::new_savepoint_session_seq() const {
    int64_t seq = 1;
    if(!savepoints_.empty()) {
        seq = savepoints_.back().seq + 1;
    }
    return seq;
}

void
token_database_impl::record(uint8_t action_type, uint8_t op, uint8_t data_type, void* data) {
    using namespace internal;

    if(!should_record()) {
        return;
    }
    auto n = savepoints_.back().node;
    assert(n.f.type == kRuntime);

    GETPOINTER(rt_group, n.group)->actions.emplace_back(rt_action(action_type, op, data_type, data));
}

namespace internal {

std::string
get_sp_key(const rt_action& act) {
    switch(act.get_data_type()) {
    case kTokenKey: {
        auto data = GETPOINTER(rt_token_key, act.data);
        return db_token_key(action_key_prefixes[act.f.type], data->key).as_string();
    }
    case kTokenFullKey: {
        auto data = GETPOINTER(rt_token_fullkey, act.data);
        return db_token_key(data->prefix, data->key).as_string();
    }
    case kAssetKey: {
        auto data = GETPOINTER(rt_asset_key, act.data);
        return std::string(data->key, sizeof(data->key));
    }
    case kTokenKeys: {
        assert(false);
    }
    }  // switch
    return std::string();
}

}  // namespace internal

void
token_database_impl::rollback_rt_group(internal::rt_group* rt) {
    using namespace internal;

    if(rt->actions.empty()) {
        db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
        return;
    }

    auto snapshot_read_opts_     = read_opts_;
    snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)rt->rb_snapshot;

    auto key_set = keys_hash_set();
    auto batch   = rocksdb::WriteBatch();
    
    for(auto it = rt->actions.begin(); it < rt->actions.end(); it++) {
        auto data = GETPOINTER(void, it->data);

        auto fn = [&](auto& key, auto type, auto op) {
            switch(op) {
            case action_op::add: {
                assert(key_set.find(key) == key_set.end());

                batch.Delete(key);
                self_.remove_token_value(key);
            
                // insert key into key set
                key_set.insert(key);
                break;
            }
            case action_op::update: {
                // only update operation need to check if key is already processed
                if(key_set.find(key) != key_set.end()) {
                    break;
                }
                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, key, &old_value);
                if(!status.ok()) {
                    FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                }
                batch.Put(key, old_value);
                self_.rollback_token_value(key);

                // insert key into key set
                key_set.insert(key);
                break;
            }
            case action_op::put: {
                // only update operation need to check if key is already processed
                if(key_set.find(key) != key_set.end()) {
                    break;
                }

                // Asset type only has put op
                auto handle    = (type == token_type::asset) ? assets_handle_ : tokens_handle_;
                auto old_value = std::string();
                auto status    = db_->Get(snapshot_read_opts_, handle, key, &old_value);

                // key may not existed in latest snapshot, remove it
                if(!status.ok()) {
                    if(status.code() != rocksdb::Status::kNotFound) {
                        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                    }
                    batch.Delete(handle, key);
                    if(handle == tokens_handle_) {
                        self_.remove_token_value(key);
                    }
                }
                else {
                    batch.Put(handle, key, old_value);
                    if(handle == tokens_handle_) {
                        self_.rollback_token_value(key);
                    }
                }

                // insert key into key set
                key_set.insert(key);
                break;
            }
            }  // switch
        };

        auto op         = it->get_action_op();
        auto data_type  = it->get_data_type();
        auto type       = it->get_token_type();

        switch(data_type) {
        case kTokenKey:
        case kTokenFullKey:
        case kAssetKey: {
            auto key  = get_sp_key(*it);
            fn(key, type, op);
            break;
        }
        case kTokenKeys: {
            auto  keys   = (rt_token_keys*)data;
            auto& prefix = keys->prefix;
            for(auto& k : keys->keys) {
                auto key = db_token_key(prefix, k).as_string();
                fn(key, type, op);
            }

            //need to call dtor of keys manually
            keys->keys.~token_keys_t();
            break;
        }
        }  // switch

        free(data);
    }  // for

    auto sync_write_opts = write_opts_;
    sync_write_opts.sync = true;
    db_->Write(sync_write_opts, &batch);

    db_->ReleaseSnapshot((const rocksdb::Snapshot*)rt->rb_snapshot);
}

void
token_database_impl::rollback_pd_group(internal::pd_group* pd) {
    using namespace internal;

    if(pd->actions.empty()) {
        return;
    }

    // keyset is not required here,
    // because it has done during creating persist savepoint
    // cache rollback is also no need here
    // because cache cannot have persist value objects
    auto batch = rocksdb::WriteBatch();
    for(auto it = pd->actions.begin(); it < pd->actions.end(); it++) {
        switch((action_op)it->op) {
        case action_op::add: {
            assert(it->value.empty());
            batch.Delete(it->key);
            break;
        }
        case action_op::update: {
            assert(!it->value.empty());
            batch.Put(it->key, it->value);
            break;
        }
        case action_op::put: {
            // Asset type only has put op
            auto handle = (it->type == (int)token_type::asset) ? assets_handle_ : tokens_handle_;
            if(it->value.empty()) {
                batch.Delete(handle, it->key);
            }
            else {
                batch.Put(handle, it->key, it->value);
            }
            break;
        }
        }  // switch
    }

    auto sync_write_opts = write_opts_;
    sync_write_opts.sync = true;
    db_->Write(sync_write_opts, &batch);
}

void
token_database_impl::rollback_to_latest_savepoint() {
    using namespace internal;
    jmzk_ASSERT(!savepoints_.empty(), token_database_no_savepoint, "There's no savepoints anymore");

    auto  seq = savepoints_.back().seq;
    auto& n   = savepoints_.back().node;

    switch(n.f.type) {
    case kRuntime: {
        auto rt = GETPOINTER(rt_group, n.group);
        rollback_rt_group(rt);
        delete rt;

        break;
    }
    case kPersist: {
        auto pd = GETPOINTER(pd_group, n.group);
        rollback_pd_group(pd);
        delete pd;

        break;
    }
    }  // switch

    savepoints_.pop_back();

    assert(seq == assets_write_cache_.ops_.back().seq);
    assets_write_cache_.rollback_to_latest_savepoint();
}

void
token_database_impl::persist_savepoints() const {
    using namespace internal;

    try {
        auto filename = config_.db_path / config::token_database_persisit_filename;
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

        persist_savepoints(fs);
        assets_write_cache_.persist_savepoints(fs);

        // clear dirty
        fs.seekp(0);
        h.dirty_flag = 0;
        fc::raw::pack(fs, h);

        // flush and close
        fs.flush();
        fs.close();
    }
    jmzk_CAPTURE_AND_RETHROW(token_database_persist_exception);
}

void
token_database_impl::load_savepoints() {
    auto filename = config_.db_path / config::token_database_persisit_filename;
    if(!fc::exists(filename)) {
        wlog("No savepoints log in token database");
        return;
    }

    auto fs = std::fstream();
    fs.exceptions(std::fstream::failbit | std::fstream::badbit);
    fs.open(filename.to_native_ansi_path(), (std::ios::in | std::ios::binary));

    // delete old savepoints if existed (from snapshot)
    savepoints_.clear();
    assets_write_cache_.clear();

    // load
    load_savepoints(fs);
    assets_write_cache_.load_savepoints(fs);

    // close
    fs.close();
}

void
token_database_impl::persist_savepoints(std::ostream& os) const {
    using namespace internal;

    auto pds = std::vector<pd_group>();

    for(auto i = 0u; i < savepoints_.size(); i++) {
        auto& sp = savepoints_[i];

        auto pd = pd_group();
        pd.seq  = sp.seq;

        auto n = sp.node;

        switch(n.f.type) {
        case kPersist: {
            auto pd2 = GETPOINTER(pd_group, n.group);
            pd.actions.insert(pd.actions.cbegin(), pd2->actions.cbegin(), pd2->actions.cend());
            
            break;
        }
        case kRuntime: {
            auto rt = GETPOINTER(rt_group, n.group);

            auto key_set = keys_hash_set();

            auto snapshot_read_opts_     = read_opts_;
            snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)rt->rb_snapshot;

            for(auto& act : rt->actions) {
                auto data = GETPOINTER(void, act.data);

                auto fn = [&](const auto& key, auto type, auto op) {
                    auto value = std::string();

                    switch(op) {
                    case action_op::add: {
                        assert(key_set.find(key) == key_set.end());

                        // no need to read value
                        key_set.insert(key);
                        break;
                    }
                    case action_op::update: {
                        if(key_set.find(key) != key_set.end()) {
                            break;
                        }

                        auto status = db_->Get(snapshot_read_opts_, key, &value);
                        if(!status.ok()) {
                            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                        }

                        key_set.insert(key);
                        break;
                    }
                    case action_op::put: {
                        if(key_set.find(key) != key_set.end()) {
                            break;
                        }

                        auto handle = (type == token_type::asset) ? assets_handle_ : tokens_handle_;
                        auto status = db_->Get(snapshot_read_opts_, handle, key, &value);
                        
                        // key may not existed in latest snapshot
                        if(!status.ok() && status.code() != rocksdb::Status::kNotFound) {
                            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
                        }

                        key_set.insert(key);
                        break;
                    }
                    }  // switch

                    return value;
                };

                auto op         = act.get_action_op();
                auto data_type  = act.get_data_type();
                auto type       = act.get_token_type();

                auto pdact = pd_action();
                pdact.op   = (int)op;
                pdact.type = (int)type;

                switch(data_type) {
                case kTokenKey:
                case kTokenFullKey:
                case kAssetKey: {
                    pdact.key   = get_sp_key(act);
                    pdact.value = fn(pdact.key, type, op);
                    pd.actions.emplace_back(std::move(pdact));
                    break;
                }
                case kTokenKeys: {
                    auto  keys   = (rt_token_keys*)data;
                    auto& prefix = keys->prefix;
                    for(auto& k : keys->keys) {
                        pdact.key   = db_token_key(prefix, k).as_string();
                        pdact.value = fn(pdact.key, type, op);
                        pd.actions.emplace_back(pdact);
                    }
                    break;
                }
                }  // switch
            }  // for
            break;
        }
        }  // switch

        pds.emplace_back(std::move(pd));
    }  // for

    fc::raw::pack(os, pds);
}

void
token_database_impl::load_savepoints(std::istream& is) {
    using namespace internal;

    auto h = pd_header();
    fc::raw::unpack(is, h);
    jmzk_ASSERT(h.dirty_flag == 0, token_database_dirty_flag_exception, "checkpoints log file dirty flag set");

    auto pds = std::vector<pd_group>();
    fc::raw::unpack(is, pds);

    for(auto& pd : pds) {
        savepoints_.push_back(savepoint(pd.seq, kPersist));

        auto ppd = new pd_group(pd);
        SETPOINTER(void, savepoints_.back().node.group, ppd);
    }
}

void
token_database_impl::flush() const {
    auto status = db_->Flush(rocksdb::FlushOptions());
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
}

token_database::token_database(const config& config)
    : my_(std::make_unique<token_database_impl>(*this, config)) {}

token_database::~token_database() {
    my_->close();
}

void
token_database::open(int load_persistence) {
    my_->open(load_persistence);
}

void
token_database::close(int persist) {
    my_->close(persist);
}

void
token_database::put_token(token_type type, action_op op, const std::optional<name128>& domain, const name128& key, const std::string_view& data) {
    using namespace internal;

    assert(type != token_type::asset);
    assert((type == token_type::token) != (!domain.has_value()));
    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    my_->put_token(type, op, prefix, key, data);
}

void
token_database::put_tokens(token_type type,
                           action_op op,
                           const std::optional<name128>& domain,
                           token_keys_t&& keys,
                           const small_vector_base<std::string_view>& data) {
    using namespace internal;

    assert(type != token_type::asset);
    assert((type == token_type::token) != (!domain.has_value()));
    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    my_->put_tokens(type, op, prefix, std::move(keys), data);
}

void
token_database::put_asset(const address& addr, const symbol_id_type sym_id, const std::string_view& data) {
    my_->put_asset(addr, sym_id, data);
}

int
token_database::exists_token(token_type type, const std::optional<name128>& domain, const name128& key) const {
    using namespace internal;

    assert(type != token_type::asset);
    assert((type == token_type::token) != (!domain.has_value()));
    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    return my_->exists_token(prefix, key);
}

int
token_database::exists_asset(const address& addr, const symbol_id_type sym_id) const {
    return my_->exists_asset(addr, sym_id);
}

int
token_database::read_token(token_type type, const std::optional<name128>& domain, const name128& key, std::string& out, bool no_throw) const {
    using namespace internal;

    assert(type != token_type::asset);
    assert((type == token_type::token) != (!domain.has_value()));
    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    return my_->read_token(prefix, key, out, no_throw);
}

int
token_database::read_asset(const address& addr, const symbol_id_type sym_id, std::string& out, bool no_throw) const {
    return my_->read_asset(addr, sym_id, out, no_throw);
}

int
token_database::read_tokens_range(token_type type, const std::optional<name128>& domain, int skip, const read_value_func& func) const {
    using namespace internal;

    assert(type != token_type::asset);
    assert((type == token_type::token) != (!domain.has_value()));
    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    return my_->read_tokens_range(prefix, skip, func);
}

int
token_database::read_assets_range(const symbol_id_type sym_id, int skip, const read_value_func& func) const {
    return my_->read_assets_range(sym_id, skip, func);
}

token_database::session
token_database::new_savepoint_session(int64_t seq) {
    my_->add_savepoint(seq);
    return session(*this, seq);
}

token_database::session
token_database::new_savepoint_session() {
    auto seq = my_->new_savepoint_session_seq();
    my_->add_savepoint(seq);
    return session(*this, seq);
}

size_t
token_database::savepoints_size() const {
    return my_->savepoints_size();
}

void
token_database::add_savepoint(int64_t seq) {
    my_->add_savepoint(seq);
}

void
token_database::rollback_to_latest_savepoint() {
    my_->rollback_to_latest_savepoint();
}

void
token_database::pop_savepoints(int64_t until) {
    my_->pop_savepoints(until);
}

void
token_database::pop_back_savepoint() {
    my_->pop_back_savepoint();
}

void
token_database::squash() {
    my_->squash();
}

int64_t
token_database::latest_savepoint_seq() const {
    return my_->latest_savepoint_seq();
}

std::string
token_database::stats() const {
    auto s = std::string();
    if(my_->db_->GetProperty(rocksdb::DB::Properties::kStats, &s)) {
        return s;
    }
    return "NA";
}

void
token_database::flush() const {
    my_->flush();
}

void
token_database::persist_savepoints(std::ostream& os) const {
    my_->persist_savepoints(os);
}

void
token_database::load_savepoints(std::istream& is) {
    my_->load_savepoints(is);
}

std::string
token_database::get_db_key(token_type type, const std::optional<name128>& domain, const name128& key) {
    using namespace internal;

    auto& prefix = domain.has_value() ? *domain : action_key_prefixes[(int)type];
    auto  dkey   = db_token_key(prefix, key);
    return dkey.as_string();
}

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::internal::pd_header, (dirty_flag));
FC_REFLECT(jmzk::chain::internal::pd_action, (op)(type)(key)(value));
FC_REFLECT(jmzk::chain::internal::pd_group,  (seq)(actions));
FC_REFLECT(jmzk::chain::internal::wc_entry, (k)(v));
FC_REFLECT(jmzk::chain::internal::wc_entry_pack, (seq)(vec));
