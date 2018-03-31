/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/

#include "eosio/chain/tokendb.hpp"
#include <boost/foreach.hpp>
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/merge_operator.h>
#include <fc/io/raw.hpp>
#include <fc/io/datastream.hpp>

namespace evt { namespace chain {

namespace __internal {

template<typename T, int N = sizeof(T)>
struct db_key {
    db_key(const char* prefix, const T& t) : prefix(prefix), slice((const char*)this, 16 + N) {
        static_assert(sizeof(domain_name) == 16);
        memcpy(data, &t, N);
    }

    db_key(domain_name prefix, const T& t) : prefix(prefix), slice((const char*)this, 16 + N) {
        memcpy(data, &t, N);
    }

    const rocksdb::Slice& as_slice() const {
        return slice;
    }

    domain_name     prefix;
    char            data[N];

    rocksdb::Slice  slice;
};

db_key<domain_name>
get_domain_key(const domain_name name) {
    return db_key<domain_name>("domain", name);
}

db_key<token_name>
get_token_key(const domain_name domain, const token_name name) {
    return db_key<token_name>(domain, name);
}

db_key<group_id>
get_group_key(const group_id& id) {
    return db_key<group_id>("group", id);
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

template<typename T, typename V>
T
read_value(const V& value) {
    T v;
    auto ds = fc::datastream<const char*>(value.data(), value.size());
    fc::raw::unpack(ds, v);
    return v;
}

class TokendbMerge : public rocksdb::MergeOperator {
public:
    virtual bool
    FullMergeV2(const MergeOperationInput& merge_in, MergeOperationOutput* merge_out) const override 
    {
        if(merge_in.existing_value == nullptr) {
            return false;
        }
        static domain_name GroupDomainName("group");
        static rocksdb::Slice GroupDomainPrefix((const char*)&GroupDomainName, sizeof(GroupDomainName));

        try {
            // merge only need to consider latest one
            if(merge_in.key.starts_with(GroupDomainPrefix)) {
                // group
                auto v = read_value<group_def>(*merge_in.existing_value);
                auto ug = read_value<updategroup>(merge_in.operand_list[merge_in.operand_list.size() - 1]);
                v.threshold = ug.threshold;
                v.keys = std::move(ug.keys);
                merge_out->new_value = get_value(v);
            }
            else {
                // token
                auto v = read_value<token_def>(*merge_in.existing_value);
                auto tt = read_value<transfertoken>(merge_in.operand_list[merge_in.operand_list.size() - 1]);
                v.owner = tt.to;
                merge_out->new_value = get_value(v);
            }
        }
        catch(fc::exception& e) {
            return false;
        }
        return true;
    }

    virtual bool
    PartialMerge(const rocksdb::Slice& key, const rocksdb::Slice& left_operand,
                 const rocksdb::Slice& right_operand, std::string* new_value,
                 rocksdb::Logger* logger) const override 
    {
        *new_value = right_operand.ToString();
        return true;
    }

    virtual const char* Name() const override { return "Tokendb"; };
    virtual bool AllowSingleOperand() const override { return true; }
};

} // namespace __internal

int
tokendb::initialize(const std::string& dbpath) {
    using namespace rocksdb;
    using namespace __internal;

    assert(db_ == nullptr);
    Options options;
    options.create_if_missing = true;
    options.compression = CompressionType::kLZ4Compression;
    options.bottommost_compression = CompressionType::kZSTD;
    options.table_factory.reset(NewPlainTableFactory());
    options.prefix_extractor.reset(NewFixedPrefixTransform(sizeof(uint128_t)));
    options.merge_operator.reset(new TokendbMerge());

    auto status = DB::Open(options, dbpath, &db_);
    if(!status.ok()) {
        return status.code();
    }

    // add system reservered domain
    if(!exists_domain("domain")) {
        add_domain(domain_def("domain"));
    }
    if(!exists_domain("group")) {
        add_domain(domain_def("group"));
    }

    return 0;
}

int
tokendb::add_domain(const domain_def& domain) {
    using namespace __internal;
    if(exists_domain(domain.name)) {
        return tokendb_error::domain_existed;
    }
    auto key = get_domain_key(domain.name);
    auto value = get_value(domain);
    auto status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
tokendb::exists_domain(const domain_name name) {
    using namespace __internal;
    auto key = get_domain_key(name);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
tokendb::issue_tokens(const issuetoken& issue) {
    using namespace __internal;
    if(!exists_domain(issue.domain)) {
        return tokendb_error::not_found_domain;
    }
    rocksdb::WriteBatch batch;
    for(auto name : issue.names) {
        auto key = get_token_key(issue.domain, name);
        auto value = get_value(token_def(issue.domain, name, issue.owner));
        batch.Put(key.as_slice(), value);
    }
    auto status = db_->Write(rocksdb::WriteOptions(), &batch);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
tokendb::exists_token(const domain_name type, const token_name name) {
    using namespace __internal;
    auto key = get_token_key(type, name);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
tokendb::add_group(const group_def& group) {
    using namespace __internal;
    if(exists_group(group.id)) {
        return tokendb_error::group_existed;
    }
    auto key = get_group_key(group.id);
    auto value = get_value(group);
    auto status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
tokendb::exists_group(const group_id& id) {
    using namespace __internal;
    auto key = get_group_key(id);
    std::string value;
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    return status.ok();
}

int
tokendb::update_domain(const domain_name type, const update_domain_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_domain_key(type);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_domain;
    }
    auto v = read_value<domain_def>(value);
    func(v);
    auto new_value = get_value(v);
    auto new_status = db_->Put(rocksdb::WriteOptions(), key.as_slice(), new_value);
    if(!new_status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
tokendb::read_domain(const domain_name type, const read_domain_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_domain_key(type);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_domain;
    }
    auto v = read_value<domain_def>(value);
    func(v);
    return 0;
}

int
tokendb::update_token(const domain_name type, const token_name name, const update_token_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_token_key(type, name);
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
tokendb::read_token(const domain_name type, const token_name name, const read_token_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_token_key(type, name);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_id;
    }
    auto v = read_value<token_def>(value);
    func(v);
    return 0;
}

int
tokendb::update_group(const group_id& id, const update_group_func& func) {
    using namespace __internal;
    std::string value;
    auto key = get_group_key(id);
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
tokendb::read_group(const group_id& id, const read_group_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_group_key(id);
    auto status = db_->Get(rocksdb::ReadOptions(), key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_group;
    }
    auto v = read_value<group_def>(value);
    func(v);
    return 0;
}

int
tokendb::update_group(const updategroup& ug) {
    using namespace __internal;
    auto key = get_group_key(ug.id);
    auto value = get_value(ug);
    auto status = db_->Merge(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

int
tokendb::transfer_token(const transfertoken& tt) {
    using namespace __internal;
    auto key = get_token_key(tt.domain, tt.name);
    auto value = get_value(tt);
    auto status = db_->Merge(rocksdb::WriteOptions(), key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    return 0;
}

}}  // namespace evt::chain
