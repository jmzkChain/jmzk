/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/

#include "eosio/chain/tokendb.hpp"
#include <boost/foreach.hpp>
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/slice_transform.h>
#include <fc/io/raw.hpp>
#include <fc/io/datastream.hpp>

namespace evt { namespace chain {

int
token_db::initialize(const std::string& dbpath) {
    using namespace rocksdb;

    assert(db_ == nullptr);
    Options options;
    options.create_if_missing = true;
    options.compression = CompressionType::kLZ4Compression;
    options.bottommost_compression = CompressionType::kZSTD;
    options.table_factory.reset(NewPlainTableFactory());
    options.prefix_extractor.reset(NewFixedPrefixTransform(sizeof(int64_t)));

    auto status = DB::Open(options, dbpath, &db_);
    if(!status.ok()) {
        return status.code();
    }

    // add system reservered types perfix
    if(!exists_token_type("type")) {
        add_new_token_type(token_type_def("type"));
    }
    if(!exists_token_type("group")) {
        add_new_token_type(token_type_def("group"));
    }

    static_assert(sizeof(group_key) <= 64);
    static_assert(sizeof(token_type_name) == 8);

    return 0;
}

namespace __internal {

template<typename T, int N = sizeof(T)>
struct db_key {
    db_key(const char* prefix, const T& t) : prefix(prefix) {
        memcpy(data, &t, N);
        static_assert(sizeof(*this) ==  8 + N);
    }

    db_key(token_type_name prefix, const T& t) : prefix(prefix) {
        memcpy(data, &t, N);
        static_assert(sizeof(*this) ==  8 + N);
    }

    rocksdb::Slice as_slice() {
        return rocksdb::Slice((const char*)this, sizeof(*this));
    }

    token_type_name prefix;
    char            data[N];
};

db_key<token_type_name>
get_token_type_key(const token_type_name name) {
    return db_key<token_type_name>("type", name);
}

db_key<token_id>
get_token_key(const token_type_name name, const token_id id) {
    return db_key<token_id>(name, id);
}

db_key<group_key>
get_group_key(const group_key& key) {
    return db_key<group_key>("group", key);
}

template<typename T>
std::string
get_value(const T& v) {
    std::string value;
    auto size = fc::raw::pack_size(v);
    value.resize(size);
    auto ds = fc::datastream<char*>((char*)value.data(), size);
    fc::raw::pack(ds, v);
    return value;
}

template<typename T>
T
read_value(const std::string& value) {
    T v;
    auto ds = fc::datastream<const char*>(value.data(), value.size());
    fc::raw::unpack(ds, v);
    return v;
}

} // namespace __internal

int
token_db::add_new_token_type(const token_type_def& ttype) {
    using namespace __internal;
    if(exists_token_type(ttype.name)) {
        return tokendb_error::token_type_existed;
    }
    auto key = get_token_type_key(ttype.name);
    auto value = get_value(ttype);
    auto status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::exists_token_type(const token_type_name name) {
    using namespace __internal;
    auto key = get_token_type_key(name);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
token_db::issue_tokens(const issuetoken& issue) {
    using namespace __internal;
    if(!exists_token_type(issue.type)) {
        return tokendb_error::not_found_token_type;
    }
    rocksdb::WriteBatch batch;
    for(auto id : issue.ids) {
        auto key = get_token_key(issue.type, id);
        auto value = get_value(token_def(issue.type, id, issue.owner));
        batch.Put(key.as_slice(), value);
    }
    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::exists_token(const token_type_name type, const token_id id) {
    using namespace __internal;
    auto key = get_token_key(type, id);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
token_db::add_group(const group_def& group) {
    using namespace __internal;
    if(exists_group(group.key)) {
        return tokendb_error::group_existed;
    }
    auto key = get_group_key(group.key);
    auto value = get_value(group);
    auto status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::exists_group(const group_key& gkey) {
    using namespace __internal;
    auto key = get_group_key(gkey);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
token_db::update_token_type(const token_type_name type, const update_token_type_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_token_type_key(type);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_type;
    }
    auto v = read_value<token_type_def>(value);
    func(v);
    auto new_value = get_value(v);
    auto new_status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), new_value);
    if(!new_status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::read_token_type(const token_type_name type, const read_token_type_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_token_type_key(type);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_type;
    }
    auto v = read_value<token_type_def>(value);
    func(v);
    return 0;
}

int
token_db::update_token(const token_type_name type, const token_id id, const update_token_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_token_key(type, id);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_id;
    }
    auto v = read_value<token_def>(value);
    func(v);
    auto new_value = get_value(v);
    auto new_status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), new_value);
    if(!new_status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::read_token(const token_type_name type, const token_id id, const read_token_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_token_key(type, id);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_id;
    }
    auto v = read_value<token_def>(value);
    func(v);
    return 0;
}

int
token_db::update_group(const group_key& gkey, const update_group_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_group_key(gkey);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_group;
    }
    auto v = read_value<group_def>(value);
    func(v);
    auto new_value = get_value(v);
    auto new_status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), new_value);
    if(!new_status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
token_db::read_group(const group_key& gkey, const read_group_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_group_key(gkey);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_group;
    }
    auto v = read_value<group_def>(value);
    func(v);
    return 0;
}

}}  // namespace evt::chain
