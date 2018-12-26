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
#include <boost/hana.hpp>

#include <fc/filesystem.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>

#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

namespace hana = boost::hana;

namespace __internal {

const size_t PKEY_SIZE = sizeof(fc::ecc::public_key_shim);

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

enum act_type {
    kRT = 0,
    kPD
};

enum act_op {
    kNew = 0,
    kUpdate,
    kNewOrUpdate
};

enum act_data_type {
    kDomain = 0,
    kToken,
    kGroup,
    kSuspend,
    kLock,
    kFungible,
    kProdVote,
    kAsset,
    kEvtLink
};

template<uint128_t i>
using uint128 = hana::integral_constant<uint128_t, i>;

template<uint128_t i>
constexpr uint128<i> uint128_c{};

auto act_data_map = hana::make_map(
    hana::make_pair(hana::int_c<kDomain>, hana::make_tuple(uint128_c<N128(.domain)>, hana::type_c<domain_def>)),
    hana::make_pair(hana::int_c<kGroup>, hana::make_tuple(uint128_c<N128(.group)>, hana::type_c<group_def>)),
    hana::make_pair(hana::int_c<kSuspend>, hana::make_tuple(uint128_c<N128(.suspend)>, hana::type_c<suspend_def>)),
    hana::make_pair(hana::int_c<kLock>, hana::make_tuple(uint128_c<N128(.lock)>, hana::type_c<lock_def>)),
    hana::make_pair(hana::int_c<kFungible>, hana::make_tuple(uint128_c<N128(.fungible)>, hana::type_c<fungible_def>)),
    hana::make_pair(hana::int_c<kProdVote>, hana::make_tuple(uint128_c<N128(.prodvote)>, hana::type_c<prodvote>)),
    hana::make_pair(hana::int_c<kEvtLink>, hana::make_tuple(uint128_c<N128(.evtlink)>, hana::type_c<evt_link_object>))
);

inline db_key
get_db_key(const name128& prefix, const name128& n) {
    return db_key(prefix, n);
}

template<int DT>
inline db_key
get_db_key(const name128& n) {
    return db_key(name128(hana::at(hana::at_key(act_data_map, hana::int_c<DT>), hana::int_c<0>)), n);
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
    auto size  = fc::raw::pack_size(v);
    value.resize(size);

    auto ds = fc::datastream<char*>((char*)value.data(), size);
    fc::raw::pack(ds, v);

    return value;
}

template <typename T, typename V>
T
read_value(const V& value) {
    auto v  = T{};
    auto ds = fc::datastream<const char*>(value.data(), value.size());
    fc::raw::unpack(ds, v);
    
    return v;
}

struct rt_data_key {
    name128 key;
};

struct rt_data_key2 {
    name128 key1;
    name128 key2;
};

struct rt_data_keys {
    size_t   sz;
    name128  keys[0];
};

struct rt_data_asset {
    char key[PKEY_SIZE + sizeof(symbol)];
};

template<typename T>
name128
get_key(const T& v) {
    return v.name;
}

template<>
name128
get_key<group_def>(const group_def& v) {
    return v.name();
}

template<>
name128
get_key<fungible_def>(const fungible_def& v) {
    return v.sym.id();
}

template<>
name128
get_key<evt_link_object>(const evt_link_object& v) {
    return v.link_id;
}

}  // namespace __internal

class token_database_impl {
public:
    token_database_impl(token_database& self) : self_(self) {}

public:
    template<int DT, typename TV = typename decltype(hana::at(hana::at_key(__internal::act_data_map, hana::int_c<DT>), hana::int_c<1>))::type>
    int
    add_impl(const TV& v) {
        using namespace __internal;
        static_assert(DT != kToken && DT != kAsset);

        auto key    = get_key<TV>(v);
        auto dbkey  = get_db_key<DT>(key);
        auto value  = get_value(v);
        auto status = self_.db_->Put(self_.write_opts_, dbkey.as_slice(), value);
        if(!status.ok()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        if(self_.should_record()) {
            auto data = (rt_data_key*)malloc(sizeof(rt_data_key));
            data->key = key;
            self_.record(DT, kNew, data);
        }
        return 0;
    }

    template<int DT, typename TV = typename decltype(hana::at(hana::at_key(__internal::act_data_map, hana::int_c<DT>), hana::int_c<1>))::type>
    int
    update_impl(const TV& v) {
        using namespace __internal;
        static_assert(DT != kAsset);

        auto dbkey = std::string();
        if constexpr(DT == kToken) {
            dbkey = get_db_key(v.domain, v.name).as_string();
        }
        else {
            auto key = get_key<TV>(v);
            dbkey    = get_db_key<DT>(key).as_string();
        }

        auto value  = get_value(v);
        auto status = self_.db_->Put(self_.write_opts_, dbkey, value);
        if(!status.ok()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        if(self_.should_record()) {
            if constexpr(DT == kToken) {
                auto data  = (rt_data_key2*)malloc(sizeof(rt_data_key2));
                data->key1 = v.domain;
                data->key2 = v.name;
                self_.record(DT, kUpdate, data);
            }
            else {
                auto data = (rt_data_key*)malloc(sizeof(rt_data_key));
                data->key = get_key<TV>(v);
                self_.record(DT, kUpdate, data);
            }
        }
        return 0;
    }

    template<int DT>
    int
    exists_impl(const name128& key) {
        using namespace __internal;

        auto dbkey  = get_db_key<DT>(key);
        auto value  = std::string();
        auto status = self_.db_->Get(self_.read_opts_, dbkey.as_slice(), &value);
        return status.ok();
    }

    template<int DT, typename T = typename decltype(hana::at(hana::at_key(__internal::act_data_map, hana::int_c<DT>), hana::int_c<1>))::type>
    int
    read_impl(const name128& key, T& out) {
        using namespace __internal;

        auto value  = std::string();
        auto dbkey  = get_db_key<DT>(key);
        auto status = self_.db_->Get(self_.read_opts_, dbkey.as_slice(), &value);
        if(!status.ok()) {
            if(!status.IsNotFound()) {
                FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
            }
            EVT_THROW(tokendb_key_not_found, "Cannot find key: ${k} with prefix: ${p}",
                ("k",key)("p",name128(hana::at(hana::at_key(act_data_map, hana::int_c<DT>), hana::int_c<0>))));
        }
        out = read_value<T>(value);
        return 0;
    }

private:
    token_database& self_;
};

token_database::token_database() 
    : db_(nullptr)
    , read_opts_()
    , write_opts_()
    , tokens_handle_(nullptr)
    , assets_handle_(nullptr)
    , savepoints_()
    , my_(std::make_unique<token_database_impl>(*this)) {}

token_database::token_database(const fc::path& dbpath)
    : token_database() {
    db_path_ = dbpath.to_native_ansi_path();
}

token_database::~token_database() {
    close();
}

int
token_database::open(int load_persistence) {
    using namespace rocksdb;
    using namespace __internal;

    static std::string AssetsColumnFamilyName = "Assets";

    EVT_ASSERT(!db_path_.empty(), tokendb_exception, "No dbpath set, cannot open");
    EVT_ASSERT(db_ == nullptr, tokendb_exception, "Token database is already opened");

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

        if(load_persistence) {
            load_savepoints();
        }
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

    if(load_persistence) {
        load_savepoints();
    }
    return 0;
}

int
token_database::close(int persist) {
    if(db_ != nullptr) {
        if(persist) {
            persist_savepoints();
        }
        if(!savepoints_.empty()) {
            free_all_savepoints();
        }
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
    return 0;
}

int
token_database::add_domain(const domain_def& domain) {
    using namespace __internal;
    return my_->add_impl<kDomain>(domain);
}

int
token_database::exists_domain(const domain_name& name) const {
    using namespace __internal;
    return my_->exists_impl<kDomain>(name);
}

int
token_database::issue_tokens(const issuetoken& issue) {
    using namespace __internal;
    if(!exists_domain(issue.domain)) {
        EVT_THROW(tokendb_key_not_found, "Cannot find key: ${name} with type: `domain`", ("name",issue.domain));
    }
    rocksdb::WriteBatch batch;
    for(auto name : issue.names) {
        auto key   = get_db_key(issue.domain, name);
        auto value = get_value(token_def(issue.domain, name, issue.owner));
        batch.Put(key.as_slice(), value);
    }
    auto status = db_->Write(write_opts_, &batch);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto data     = (rt_data_keys*)malloc(sizeof(rt_data_keys) + sizeof(name128) * (issue.names.size() + 1));
        data->sz      = issue.names.size() + 1;
        data->keys[0] = issue.domain;
        memcpy(data->keys + 1, &issue.names[0], sizeof(name128) * issue.names.size());
        record(kToken, kNew, data);
    }
    return 0;
}

int
token_database::exists_token(const domain_name& domain, const token_name& name) const {
    using namespace __internal;
    auto key    = get_db_key(domain, name);
    auto value  = std::string();
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
token_database::add_group(const group_def& group) {
    using namespace __internal;
    return my_->add_impl<kGroup>(group);
}

int
token_database::exists_group(const group_name& name) const {
    using namespace __internal;
    return my_->exists_impl<kGroup>(name);
}

int
token_database::add_suspend(const suspend_def& suspend) {
    using namespace __internal;
    return my_->add_impl<kSuspend>(suspend);
}

int
token_database::exists_suspend(const proposal_name& name) const {
    using namespace __internal;
    return my_->exists_impl<kSuspend>(name);
}

int
token_database::add_lock(const lock_def& lock) {
    using namespace __internal;
    return my_->add_impl<kLock>(lock);
}

int
token_database::exists_lock(const proposal_name& name) const {
    using namespace __internal;
    return my_->exists_impl<kLock>(name);
}

int
token_database::add_fungible(const fungible_def& fungible) {
    using namespace __internal;
    return my_->add_impl<kFungible>(fungible);
}

int
token_database::exists_fungible(const symbol sym) const {
    using namespace __internal;
    return my_->exists_impl<kFungible>((uint128_t)sym.id());
}

int
token_database::exists_fungible(const symbol_id_type sym_id) const {
    using namespace __internal;
    return my_->exists_impl<kFungible>((uint128_t)sym_id);
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
        auto act  = (rt_data_asset*)malloc(sizeof(rt_data_asset));
        memcpy(act->key, key.buf, sizeof(key.buf));
        record(kAsset, kNewOrUpdate, act);
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

    auto dbkey  = get_db_key<kProdVote>(key);
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
        auto it = map.emplace(pkey, value);
        if(it.second == false) {
            // existed
            it.first->second = value;
        }
    }
    v = get_value(map);
    
    status = db_->Put(write_opts_, dbkey.as_slice(), v);
    if(!status.ok()) {
        FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
    }
    if(should_record()) {
        auto act  = (rt_data_key*)malloc(sizeof(rt_data_key));
        act->key = key;
        record(kProdVote, kNewOrUpdate, act);
    }
    return 0;
}

int
token_database::add_evt_link(const evt_link_object& link_obj) {
    using namespace __internal;
    return my_->add_impl<kEvtLink>(link_obj);
}

int
token_database::exists_evt_link(const link_id_type& id) const {
    using namespace __internal;
    return my_->exists_impl<kEvtLink>(id);
}

int
token_database::read_domain(const domain_name& name, domain_def& domain) const {
    using namespace __internal;
    try {
        return my_->read_impl<kDomain>(name, domain);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_domain_exception, "Unknown domain: ${n}", ("n",name));
    }
}

int
token_database::read_token(const domain_name& domain, const token_name& name, token_def& token) const {
    using namespace __internal;
    auto value  = std::string();
    auto key    = get_db_key(domain, name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        EVT_THROW(unknown_token_exception, "Unknown token: ${name} in ${domain}", ("domain",domain)("name",name));
    }
    token = read_value<token_def>(value);
    return 0;
}

int
token_database::read_tokens(const domain_name& domain, int skip, const read_token_func& func) const {
    using namespace __internal;
    auto it  = db_->NewIterator(read_opts_);
    auto key = rocksdb::Slice((char*)&domain, sizeof(domain));
    
    it->Seek(key);
    int i = 0;
    while(it->Valid()) {
        if(i++ < skip) {
            it->Next();
            continue;
        }
        auto v = read_value<token_def>(it->value());
        if(!func(v)) {
            delete it;
            return 0;
        }
        it->Next();
    }
    delete it;
    return 0;
}

int
token_database::read_group(const group_name& name, group_def& group) const {
    using namespace __internal;
    try {
        return my_->read_impl<kGroup>(name, group);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_group_exception, "Unknown group: ${n}", ("n",name));
    }
}

int
token_database::read_suspend(const proposal_name& name, suspend_def& suspend) const {
    using namespace __internal;
    try {
        return my_->read_impl<kSuspend>(name, suspend);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_suspend_exception, "Unknown suspend proposal: ${n}", ("n",name));
    }
}

int
token_database::read_lock(const proposal_name& name, lock_def& lock) const {
    using namespace __internal;
    try {
        return my_->read_impl<kLock>(name, lock);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_lock_exception, "Unknown lock assets proposal: ${n}", ("n",name));
    }
}

int
token_database::read_fungible(const symbol sym, fungible_def& fungible) const {
    using namespace __internal;
    try {
        return my_->read_impl<kFungible>((uint128_t)sym.id(), fungible);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_fungible_exception, "Unknown fungible symbol id: ${id}", ("id",sym.id()));
    }
}

int
token_database::read_fungible(const symbol_id_type sym_id, fungible_def& fungible) const {
    using namespace __internal;
    try {
        return my_->read_impl<kFungible>((uint128_t)sym_id, fungible);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(unknown_fungible_exception, "Unknown fungible symbol id: ${id}", ("id",sym_id));
    }
}

int
token_database::read_asset(const address& addr, const symbol symbol, asset& v) const {
    using namespace __internal;
    auto key   = get_asset_key(addr, symbol);
    auto value = std::string();

    auto status = db_->Get(read_opts_, assets_handle_, key.as_slice(), &value);
    if(!status.ok()) {
        if(!status.IsNotFound()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        EVT_THROW(balance_exception, "Cannot find any fungible(S#${id}) balance in address: {addr}", ("id",symbol.id())("addr",addr));
    }
    v = read_value<asset>(value);
    return 0;
}

int
token_database::read_asset_no_throw(const address& addr, const symbol symbol, asset& v) const {
    using namespace __internal;
    auto key   = get_asset_key(addr, symbol);
    auto value = std::string();

    auto status = db_->Get(read_opts_, assets_handle_, key.as_slice(), &value);
    if(!status.ok()) {
        if(!status.IsNotFound()) {
            FC_THROW_EXCEPTION(fc::unrecoverable_exception, "Rocksdb internal error: ${err}", ("err", status.getState()));
        }
        v = asset(0, symbol);
        return 0;
    }
    v = read_value<asset>(value);
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
    auto dbkey  = get_db_key<kProdVote>(key);
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
token_database::read_evt_link(const link_id_type& id, evt_link_object& link_obj) const {
    using namespace __internal;
    try {
        return my_->read_impl<kEvtLink>(id, link_obj);
    }
    catch(const tokendb_key_not_found&) {
        EVT_THROW(evt_link_existed_exception, "Unknown Evt Link: ${id}", ("id",id));
    }
}

int
token_database::update_domain(const domain_def& domain) {
    using namespace __internal;
    return my_->update_impl<kDomain>(domain);
}

int
token_database::update_group(const group& group) {
    using namespace __internal;
    return my_->update_impl<kGroup>(group);
}

int
token_database::update_token(const token_def& token) {
    using namespace __internal;
    return my_->update_impl<kToken>(token);
}

int
token_database::update_suspend(const suspend_def& suspend) {
    using namespace __internal;
    return my_->update_impl<kSuspend>(suspend);
}

int
token_database::update_lock(const lock_def& lock) {
    using namespace __internal;
    return my_->update_impl<kLock>(lock);
}

int
token_database::update_fungible(const fungible_def& fungible) {
    using namespace __internal;
    return my_->update_impl<kFungible>(fungible);
}

int
token_database::record(uint8_t type, uint8_t op, void* data) {
    using namespace __internal;

    if(!should_record()) {
        return 0;
    }
    auto n = savepoints_.back().node;
    FC_ASSERT(n.f.type == kRT);

    GETPOINTER(rt_group, n.group)->actions.emplace_back(rt_action(type, op, data));
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
token_database::free_all_savepoints() {
    for(auto& sp : savepoints_) {
        free_savepoint(sp);
    }
    savepoints_.clear();
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

int64_t
token_database::latest_savepoint_seq() const {
    EVT_ASSERT(!savepoints_.empty(), tokendb_no_savepoint, "There's no savepoints anymore");
    return savepoints_.back().seq;
}

namespace __internal {

std::string
get_sp_key(const token_database::rt_action& act) {
    switch(act.f.type) {
    case kDomain: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kDomain>(data->key).as_string();
    }
    case kToken: {
        FC_ASSERT(act.f.op != kNew);
        auto data = GETPOINTER(rt_data_key2, act.data);
        return get_db_key(data->key1, data->key2).as_string();
    }
    case kGroup: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kGroup>(data->key).as_string();
    }
    case kSuspend: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kSuspend>(data->key).as_string();
    }
    case kLock: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kLock>(data->key).as_string();
    }
    case kFungible: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kFungible>(data->key).as_string();
    }
    case kProdVote: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kProdVote>(data->key).as_string();
    }
    case kAsset: {
        auto data = GETPOINTER(rt_data_asset, act.data);
        return std::string(data->key, sizeof(data->key));
    }
    case kEvtLink: {
        auto data = GETPOINTER(rt_data_key, act.data);
        return get_db_key<kEvtLink>(data->key).as_string();
    }
    default: {
        EVT_THROW(fc::unrecoverable_exception, "Not excepted action type: ${t}", ("t",act.f.type));
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

        if(it->f.type == kToken && it->f.op == kNew) {
            // special process issue token action, cuz it's multiple keys
            auto  act    = (rt_data_keys*)data;
            auto& domain = act->keys[0]; 
            for(size_t i = 1; i < act->sz; i++) {
                auto  key = get_db_key(domain, act->keys[i]).as_string();
                batch.Delete(key);

                // insert key into key set
                key_set.emplace(std::move(key));
            }

            free(data);
            continue;
        }

        auto op  = it->f.op;
        auto key = get_sp_key(*it);

        switch(op) {
        case kNew: {
            FC_ASSERT(key_set.find(key) == key_set.cend());

            batch.Delete(key);
        
            // insert key into key set
            key_set.emplace(std::move(key));
            break;
        }
        case kUpdate: {
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
        case kNewOrUpdate: {
            // only update operation need to check if key is already processed
            if(key_set.find(key) != key_set.cend()) {
                break;
            }

            auto old_value = std::string();
            if(it->f.type == kAsset) {
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
        case kNew: {
            FC_ASSERT(key_set.find(it->key) == key_set.cend());

            FC_ASSERT(it->value.empty());
            batch.Delete(it->key);

            key_set.emplace(it->key);
            break;
        }
        case kUpdate: {
            if(key_set.find(it->key) != key_set.cend()) {
                break;
            }

            FC_ASSERT(!it->value.empty());
            batch.Put(it->key, it->value);

            key_set.emplace(it->key);
            break;
        }
        case kNewOrUpdate: {
            if(key_set.find(it->key) != key_set.cend()) {
                break;
            }

            if(it->type == kAsset) {
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
token_database::persist_savepoints() const {
    try {
        auto filename = fc::path(db_path_) / config::tokendb_persisit_filename;
        if(fc::exists(filename)) {
            fc::remove(filename);
        }
        auto fs = std::fstream();
        fs.exceptions(std::fstream::failbit | std::fstream::badbit);
        fs.open(filename.to_native_ansi_path(), (std::ios::out | std::ios::binary));

        persist_savepoints(fs);

        fs.flush();
        fs.close();
    }
    EVT_CAPTURE_AND_RETHROW(tokendb_persist_exception);
    return 0;
}

int
token_database::load_savepoints() {
    auto filename = fc::path(db_path_) / config::tokendb_persisit_filename;
    if(!fc::exists(filename)) {
        wlog("No savepoints log in token database");
        return 0;
    }

    auto fs = std::fstream();
    fs.exceptions(std::fstream::failbit | std::fstream::badbit);
    fs.open(filename.to_native_ansi_path(), (std::ios::in | std::ios::binary));

    // delete old savepoints if existed (from snapshot)
    savepoints_.clear();

    load_savepoints(fs);

    fs.close();
    return 0;
}

int
token_database::persist_savepoints(std::ostream& os) const {
    using namespace __internal;

    auto h = pd_header {
        .dirty_flag = 1
    };
    // set dirty first
    fc::raw::pack(os, h);

    auto pds = std::vector<pd_group>();

    for(auto& sp : savepoints_) {
        auto pd = pd_group();
        pd.seq  = sp.seq;

        auto n = sp.node;

        switch(n.f.type) {
        case kPD: {
            auto pd2 = GETPOINTER(pd_group, n.group);
            pd.actions.insert(pd.actions.cbegin(), pd2->actions.cbegin(), pd2->actions.cend());
            
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

                if(act.f.type == kToken && act.f.op == kNew) {
                    // special process issue token action, cuz it's multiple keys
                    auto  dkeys  = (rt_data_keys*)data;
                    auto& domain = dkeys->keys[0];
                    for(size_t i = 1; i < dkeys->sz; i++) {
                        auto key   = get_db_key(domain, dkeys->keys[i]).as_string();
                        auto pdact = pd_action();
                        pdact.op   = kNew;
                        pdact.type = act.f.type;
                        pdact.key  = key;

                        pd.actions.emplace_back(std::move(pdact));
                        key_set.emplace(std::move(key));
                    }

                    continue;
                }

                auto pdact = pd_action();
                auto op    = act.f.op;
                auto key   = get_sp_key(act);

                pdact.op   = op;
                pdact.type = act.f.type;
                pdact.key  = key;

                switch(op) {
                case kNew: {
                    FC_ASSERT(key_set.find(key) == key_set.cend());

                    // no need to read value
                    key_set.emplace(std::move(key));
                    break;
                }
                case kUpdate: {
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
                case kNewOrUpdate: {
                    if(key_set.find(key) != key_set.cend()) {
                        break;
                    }

                    rocksdb::Status status;
                    if(act.f.type == kAsset) {
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
            }

            break;
        }
        default: {
            FC_ASSERT(false);
        }
        }  // switch

        pds.emplace_back(std::move(pd));
    }  // for

    fc::raw::pack(os, pds);
    os.seekp(0);

    h.dirty_flag = 0;
    fc::raw::pack(os, h);

    return 0;
}

int
token_database::load_savepoints(std::istream& is) {
    using namespace __internal;

    auto h = pd_header();
    fc::raw::unpack(is, h);
    EVT_ASSERT(h.dirty_flag == 0, tokendb_dirty_flag_exception, "checkpoints log file dirty flag set");

    auto pds = std::vector<pd_group>();
    fc::raw::unpack(is, pds);

    for(auto& pd : pds) {
        savepoints_.emplace_back(savepoint(pd.seq, kPD));

        auto ppd = new pd_group(pd);
        SETPOINTER(void, savepoints_.back().node.group, ppd);
    }

    return 0;
}

}}  // namespace evt::chain

FC_REFLECT(evt::chain::__internal::pd_header, (dirty_flag));
FC_REFLECT(evt::chain::token_database::pd_action, (op)(type)(key)(value));
FC_REFLECT(evt::chain::token_database::pd_group, (seq)(actions));
