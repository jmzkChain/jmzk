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
        static domain_name GroupPrefixName("group");
        static rocksdb::Slice GroupPrefixSlice((const char*)&GroupPrefixName, sizeof(GroupPrefixName));
        static domain_name DomainPrefixName("domain");
        static rocksdb::Slice DomainPrefixSlice((const char*)&DomainPrefixName, sizeof(DomainPrefixName));

        try {
            // merge only need to consider latest one
            if(merge_in.key.starts_with(GroupPrefixSlice)) {
                // group
                auto v = read_value<group_def>(*merge_in.existing_value);
                auto ug = read_value<updategroup>(merge_in.operand_list[merge_in.operand_list.size() - 1]);
                v.threshold = ug.threshold;
                v.keys = std::move(ug.keys);
                merge_out->new_value = get_value(v);
            }
            else if(merge_in.key.starts_with(DomainPrefixSlice)) {
                // domain
                auto v = read_value<domain_def>(*merge_in.existing_value);
                auto ug = read_value<updatedomain>(merge_in.operand_list[merge_in.operand_list.size() - 1]);
                v.issue = std::move(ug.issue);
                v.transfer = std::move(ug.transfer);
                v.manage = std::move(ug.manage);
                merge_out->new_value = get_value(v);
            }
            else {
                // token
                auto v = read_value<token_def>(*merge_in.existing_value);
                auto tt = read_value<transfertoken>(merge_in.operand_list[merge_in.operand_list.size() - 1]);
                v.owner = std::move(tt.to);
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

enum dbaction_type {
    none = 0,
    kNewDomain,
    kIssueToken,
    kAddGroup,
    kUpdateDomain,
    kUpdateGroup,
    kUpdateToken
};

struct sp_newdomain {
    domain_name name;
};

struct sp_issuetoken {
    domain_name domain;
    size_t      size;
    token_name  names[0];
};

struct sp_addgroup {
    group_id    id;
};

struct sp_updatedomain {
    domain_name name;
};

struct sp_updategroup {
    group_id    id;
};

struct sp_updatetoken {
    domain_name domain;
    token_name  name;
};

} // namespace __internal

tokendb::~tokendb() {
    if(db_ != nullptr) {
        delete db_;
        db_ = nullptr;
    }
}

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
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    if(should_record()) {
        auto act = (sp_newdomain*)malloc(sizeof(sp_newdomain));
        act->name = domain.name;
        record(kNewDomain, act);
    }
    return 0;
}

int
tokendb::exists_domain(const domain_name name) {
    using namespace __internal;
    auto key = get_domain_key(name);
    std::string value;
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
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
    auto status = db_->Write(write_opts_, &batch);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    if(should_record()) {
        auto act = (sp_issuetoken*)malloc(sizeof(sp_issuetoken) + sizeof(token_name) * issue.names.size());
        act->domain = issue.domain;
        act->size = issue.names.size();
        memcpy(act->names, &issue.names[0], sizeof(token_name) * act->size); 
        record(kIssueToken, act);
    }
    return 0;
}

int
tokendb::exists_token(const domain_name type, const token_name name) {
    using namespace __internal;
    auto key = get_token_key(type, name);
    std::string value;
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
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
    auto status = db_->Put(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    if(should_record()) {
        auto act = (sp_addgroup*)malloc(sizeof(sp_addgroup));
        act->id = group.id;
        record(kAddGroup, act);
    }
    return 0;
}

int
tokendb::exists_group(const group_id& id) {
    using namespace __internal;
    auto key = get_group_key(id);
    std::string value;
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    return status.ok();
}

int
tokendb::read_domain(const domain_name type, const read_domain_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_domain_key(type);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_domain;
    }
    auto v = read_value<domain_def>(value);
    func(v);
    return 0;
}

int
tokendb::read_token(const domain_name type, const token_name name, const read_token_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_token_key(type, name);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_token_id;
    }
    auto v = read_value<token_def>(value);
    func(v);
    return 0;
}

int
tokendb::read_group(const group_id& id, const read_group_func& func) const {
    using namespace __internal;
    std::string value;
    auto key = get_group_key(id);
    auto status = db_->Get(read_opts_, key.as_slice(), &value);
    if(!status.ok()) {
        return tokendb_error::not_found_group;
    }
    auto v = read_value<group_def>(value);
    func(v);
    return 0;
}

int
tokendb::update_domain(const updatedomain& ud) {
    using namespace __internal;
    auto key = get_domain_key(ud.name);
    auto value = get_value(ud);
    auto status = db_->Merge(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    if(should_record()) {
        auto act = (sp_updatedomain*)malloc(sizeof(sp_updatedomain));
        act->name = ud.name;
        record(kUpdateDomain, act);
    }
    return 0;
}

int
tokendb::update_group(const updategroup& ug) {
    using namespace __internal;
    auto key = get_group_key(ug.id);
    auto value = get_value(ug);
    auto status = db_->Merge(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
    if(should_record()) { 
        auto act = (sp_updategroup*)malloc(sizeof(sp_updategroup));
        act->id = ug.id;
        record(kUpdateGroup, act);
    }
    return 0;
}

int
tokendb::transfer_token(const transfertoken& tt) {
    using namespace __internal;
    auto key = get_token_key(tt.domain, tt.name);
    auto value = get_value(tt);
    auto status = db_->Merge(write_opts_, key.as_slice(), value);
    if(!status.ok()) {
        return tokendb_error::rocksdb_err;
    }
     if(should_record()) {
        auto act = (sp_updatetoken*)malloc(sizeof(sp_updatetoken));
        act->domain = tt.domain;
        act->name = tt.name;
        record(kUpdateToken, act);
    }  
    return 0;
}

int 
tokendb::record(int type, void* data) {
    if(!should_record()) {
        return tokendb_error::no_savepoint;
    }
    savepoints_.back().actions.emplace_back(dbaction { .type = type, .data = data });
    return 0;
}

int
tokendb::add_savepoint(uint32 seq) {
    if(!savepoints_.empty()) {
        if(savepoints_.back().seq >= seq) {
            return tokendb_error::seq_not_valid;
        }
    }
    savepoints_.emplace_back(savepoint {
        .seq = seq,
        .rb_snapshot = (const void*)db_->GetSnapshot(),
        .actions = {}
    });
    return 0;
}

int
tokendb::free_savepoint(savepoint& cp) {
    for(auto& act : cp.actions) {
        free(act.data);
    }
    db_->ReleaseSnapshot((const rocksdb::Snapshot*)cp.rb_snapshot);
    return 0;
}

int
tokendb::pop_savepoints(uint32 until) {
    if(savepoints_.empty()) {
        return tokendb_error::no_savepoint;
    }
    while(!savepoints_.empty() && savepoints_.front().seq < until) {
        free_savepoint(savepoints_.front());
        savepoints_.pop_front();
    }
    return 0;
}

int
tokendb::rollback_to_latest_savepoint() {
    using namespace __internal;

    if(savepoints_.empty()) {
        return tokendb_error::no_savepoint;
    }
    auto& cp = savepoints_.back();
    if(cp.actions.size() > 0) {
        auto snapshot_read_opts_ = read_opts_;
        snapshot_read_opts_.snapshot = (const rocksdb::Snapshot*)cp.rb_snapshot;
        rocksdb::WriteBatch batch;
        for(auto it = --cp.actions.end(); it >= cp.actions.begin(); it--) {
            switch(it->type) {
            case kNewDomain: {
                auto act = (sp_newdomain*)it->data;
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
            case kAddGroup: {
                auto act = (sp_addgroup*)it->data;
                auto key = get_group_key(act->id);
                batch.Delete(key.as_slice());
                break;
            }
            case kUpdateDomain: {
                auto act = (sp_updatedomain*)it->data;
                auto key = get_domain_key(act->name);
                std::string old_value;
                auto status = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);
                if(!status.ok()) {
                    // key may not existed in latest snapshot, remove it
                    FC_ASSERT(status.code() == rocksdb::Status::kNotFound, "Not expected rocksdb code: ${status}", ("status", status.getState()));
                    batch.Delete(key.as_slice());
                    break;
                }
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateGroup: {
                auto act = (sp_updategroup*)it->data;
                auto key = get_group_key(act->id);
                std::string old_value;
                auto status = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);
                if(!status.ok()) {
                    // key may not existed in latest snapshot, remove it
                    FC_ASSERT(status.code() == rocksdb::Status::kNotFound, "Not expected rocksdb code: ${status}", ("status", status.getState()));
                    batch.Delete(key.as_slice());
                    break;
                }
                batch.Put(key.as_slice(), old_value);
                break;
            }
            case kUpdateToken: {
                auto act = (sp_updatetoken*)it->data;
                auto key = get_token_key(act->domain, act->name);
                std::string old_value;
                auto status = db_->Get(snapshot_read_opts_, key.as_slice(), &old_value);
                if(!status.ok()) {
                    // key may not existed in latest snapshot, remove it
                    FC_ASSERT(status.code() == rocksdb::Status::kNotFound, "Not expected rocksdb code: ${status}", ("status", status.getState()));
                    db_->Delete(write_opts_, key.as_slice());
                    break;
                }
                batch.Put(key.as_slice(), old_value);
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
