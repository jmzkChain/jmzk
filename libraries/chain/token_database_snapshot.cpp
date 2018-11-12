#include <evt/chain/token_database_snapshot.hpp>

#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <rocksdb/utilities/backupable_db.h>
#include <fmt/format.h>

namespace evt { namespace chain {

const char* kBackupPath = "/tmp/backup";

namespace __internal {

enum row_type {
    kRaw  = 0,
    kLink = 1
};

class SnapshotDirectory : public Directory {
public:
    SnapshotDirectory() = default;
    ~SnapshotDirectory() {}

    Status Fsync() override { return Status::OK(); }
};


class SnapshotReadableFile : public rocksdb::SequentialFile,
                             public rocksdb::RandomAccessFile {
public:
    SnapshotReadableFile(snapshot_reader_ptr reader)
        : reader_(reader) {}

public:
    Status
    Read(size_t n, rocksdb::Slice* result, char* scratch) override {
        if(pos_ >= buf_.size()) {
            *result = rocksdb::Slice(scratch, 0);
            return Status::OK();
        }
        auto r = (n < (buf_.size() - pos_)) ? n : (buf_.size() - pos_);
        *result = rocksdb::Slice(buf_.data() + pos_, r);
        pos_ += r;

        return Status::OK();
    }

    Status
    Read(uint64_t offset, size_t n, rocksdb::Slice* result, char* scratch) const override {
        if(offset >= buf_.size()) {
            *result = rocksdb::Slice(scratch, 0);
            return Status::OK();
        }
        auto r = (n < (buf_.size() - offset)) ? n : (buf_.size() - offset);
        *result = rocksdb::Slice(buf_.data() + offset, r);

        return Status::OK();
    }

    Status
    Skip(uint64_t n) override {
        pos_ += n;

        return Status::OK();
    }

public:
    bool
    init(const std::string& name) {
        auto type   = int();
        auto data   = std::string();
        auto result = false;
        auto sname  = name;

        while(true) {
            reader_->read_section(sname, [&](auto& reader) {
                result = try_read(reader, type, data);
            });

            if(!result) {
                return false;
            }

            if(type == kRaw) {
                buf_.insert(buf_.begin(), data.cbegin(), data.cend());
                pos_ = 0;

                return true;
            }
            // link
            sname = data;
        }
    }

    bool
    try_read(snapshot_reader::section_reader& reader, int& type, std::string& data) {
        auto tmp  = std::string();
        auto sz   = size_t();

        reader.read_row(tmp, sizeof(type));
        type = *(int*)tmp.data();

        reader.read_row(tmp, sizeof(sz));
        sz = *(size_t*)tmp.data();

        reader.read_row(tmp, sz);
        data = std::move(tmp);
        return true;
    }

private:
    snapshot_reader_ptr reader_;
    vector<char>        buf_;
    size_t              pos_;
};


class SnapshotWritableFile : public rocksdb::WritableFile {
public:
    SnapshotWritableFile(const std::string& name, snapshot_writer_ptr writer, std::mutex& mutex)
        : name_(name), writer_(writer), mutex_(mutex) {}

public:
    Status
    Append(const rocksdb::Slice& data) override {
        buf_.insert(buf_.cend(), data.data(), data.data() + data.size());
        return Status::OK();
    }

    Status
    Truncate(uint64_t sz) override {
        buf_.resize(sz);
        return Status::OK();
    }

    Status
    Close() override {
        auto l = std::unique_lock<std::mutex>(mutex_);

        // only write to snapshot when close
        writer_->write_section(name_, [this](auto& rw) {
            size_t sz   = buf_.size();
            int    type = kRaw;

            rw.add_row("type", (char*)&type, sizeof(type));
            rw.add_row("size", (char*)&sz, sizeof(sz));
            rw.add_row("raw",  buf_.data(), buf_.size());
        });
        dlog("Written ${n}: ${sz}", ("n",name_)("sz",buf_.size()));
        return Status::OK();
    }

    Status
    Flush() override {
        return Status::OK();
    }

    Status
    Sync() override {
        return Status::OK();
    }

    /*
    * Get the size of valid data in the file.
    */
    uint64_t
    GetFileSize() override {
        return buf_.size();
    }

private:
    const std::string&  name_;
    snapshot_writer_ptr writer_;
    std::vector<char>   buf_;
    std::mutex&         mutex_;
};

}  // namespace __internal

snapshot_env::snapshot_env(snapshot_writer_ptr writer, snapshot_reader_ptr reader)
    : rocksdb::EnvWrapper(rocksdb::Env::Default())
    , writer_(writer)
    , reader_(reader) {}

Status
snapshot_env::NewSequentialFile(const std::string&               fname,
                                std::unique_ptr<SequentialFile>* result,
                                const EnvOptions&                options) {
    using namespace __internal;

    if(boost::starts_with(fname, kBackupPath)) {
        if(writer_) {
            if(std::find(created_files_.cbegin(), created_files_.cend(), fname) != created_files_.cend()) {
                return Status::NotSupported();
            }
            return Status::NotFound();
        }
        else {
            if(!reader_->has_section(fname)) {
                return Status::NotFound();
            }
            auto sr = std::make_unique<SnapshotReadableFile>(reader_);
            if(!sr->init(fname)) {
                return Status::IOError();
            }
            *result = std::move(sr);
            return Status::OK();
        }
    }
    return EnvWrapper::NewSequentialFile(fname, result, options);
}

Status
snapshot_env::NewRandomAccessFile(const std::string&                 fname,
                                  std::unique_ptr<RandomAccessFile>* result,
                                  const EnvOptions&                  options) {
    using namespace __internal;

    if(boost::starts_with(fname, kBackupPath)) {
        if(writer_) {
            if(std::find(created_files_.cbegin(), created_files_.cend(), fname) != created_files_.cend()) {
                return Status::NotSupported();
            }
            return Status::NotFound();
        }
        else {
            if(!reader_->has_section(fname)) {
                return Status::NotFound();
            }
            auto sr = std::make_unique<SnapshotReadableFile>(reader_);
            if(!sr->init(fname)) {
                return Status::IOError();
            }
            *result = std::move(sr);
            return Status::OK();
        }
    }
    return EnvWrapper::NewRandomAccessFile(fname, result, options);
}

Status
snapshot_env::NewWritableFile(const std::string&             fname,
                              std::unique_ptr<WritableFile>* result,
                              const EnvOptions&              options) {
    using namespace __internal;

    if(boost::starts_with(fname, kBackupPath)) {
        FC_ASSERT(writer_);
        *result = std::make_unique<SnapshotWritableFile>(fname, writer_, mutex_);
        created_files_.emplace_back(fname);
        return Status::OK();
    }
    return EnvWrapper::NewWritableFile(fname, result, options);
}

Status
snapshot_env::ReuseWritableFile(const std::string&             fname,
                                const std::string&             old_fname,
                                std::unique_ptr<WritableFile>* result,
                                const EnvOptions&              options) {
    return Status::NotSupported();
}

Status
snapshot_env::NewDirectory(const std::string&          name,
                           std::unique_ptr<Directory>* result) {
    using namespace __internal;

    if(boost::starts_with(name, kBackupPath)) {
        if(writer_) {
            if(std::find(created_folders_.cbegin(), created_folders_.cend(), name) != created_folders_.cend()) {
                *result = std::make_unique<SnapshotDirectory>();
                return Status::OK();
            }
            else {
                Status::NotFound();
            }
        }

    }
    return EnvWrapper::NewDirectory(name, result);
}

Status
snapshot_env::FileExists(const std::string& fname) {
    if(boost::starts_with(fname, kBackupPath)) {
        if(boost::ends_with(fname, "/")) {
            // folder
            if(writer_) {
                if(std::find(created_folders_.cbegin(), created_folders_.cend(), fname) != created_folders_.cend()) {
                    return Status::OK();
                }
                else {
                    return Status::NotFound();
                }
            }
            else {
                // for reader, always return true
                return Status::OK();
            }
        }
        else {
            // file
            if(writer_) {
                if(std::find(created_files_.cbegin(), created_files_.cend(), fname) != created_files_.cend()) {
                    return Status::OK();
                }
                else {
                    return Status::NotFound();
                }
            }
            else {
                return reader_->has_section(fname) ? Status::OK() : Status::NotFound();
            }
        }
        
        return Status::OK();
    }
    return EnvWrapper::FileExists(fname);
}

Status
snapshot_env::GetChildren(const std::string& dir, std::vector<std::string>* result) {
    if(boost::starts_with(dir, kBackupPath)) {
        if(writer_) {
            for(auto& file : created_files_) {
                if(boost::starts_with(file, dir)) {
                    result->emplace_back(file.substr(dir.size() + (boost::ends_with(dir, "/") ? 0 : 1)));
                }
            }
            return Status::OK();
        }
        else {
            auto names = reader_->get_section_names(dir);
            for(auto& name : names) {
                if(!boost::ends_with(name, ".tmp")) {
                    result->emplace_back(name.substr(dir.size() + (boost::ends_with(dir, "/") ? 0 : 1)));
                }
            }
            return Status::OK();
        }
    }
    return EnvWrapper::GetChildren(dir, result);
}

Status
snapshot_env::GetChildrenFileAttributes(const std::string& dir, std::vector<Env::FileAttributes>* result) {
    if(boost::starts_with(dir, kBackupPath)) {
        if(writer_) {
            return Status::NotSupported();
        }
        else {
            auto names = reader_->get_section_names(dir);
            for(auto& name : names) {
                if(!boost::ends_with(name, ".tmp")) {
                    auto attr = Env::FileAttributes {
                        .name       = name.substr(dir.size() + (boost::ends_with(dir, "/") ? 0 : 1)),
                        .size_bytes = reader_->get_section_size(name)
                    };
                    result->emplace_back(attr);
                }
            }
            return Status::OK();
        }
    }
    return EnvWrapper::GetChildrenFileAttributes(dir, result);
}

Status
snapshot_env::DeleteFile(const std::string& fname) {
    if(boost::starts_with(fname, kBackupPath)) {
        // delete is not supported in snapshot
        return Status::OK();
    }
    return EnvWrapper::DeleteFile(fname);
}

Status
snapshot_env::CreateDir(const std::string& dirname) {
    if(boost::starts_with(dirname, kBackupPath)) {
        if(std::find(created_folders_.cbegin(), created_folders_.cend(), dirname) == created_folders_.cend()) {
            created_folders_.emplace_back(dirname);
            return Status::OK();
        }
        return Status::InvalidArgument();
    }
    return EnvWrapper::CreateDir(dirname);
}

Status
snapshot_env::CreateDirIfMissing(const std::string& dirname) {
    if(boost::starts_with(dirname, kBackupPath)) {
        if(std::find(created_folders_.cbegin(), created_folders_.cend(), dirname) == created_folders_.cend()) {
            created_folders_.emplace_back(dirname);
        }
        return Status::OK();
    }
    return EnvWrapper::CreateDirIfMissing(dirname);
}

Status
snapshot_env::DeleteDir(const std::string& dirname) {
    if(boost::starts_with(dirname, kBackupPath)) {
        created_folders_.erase(std::remove(created_folders_.begin(), created_folders_.end(), dirname), created_folders_.end());
        return Status::OK();
    }
    return EnvWrapper::DeleteDir(dirname);
}

Status
snapshot_env::GetFileSize(const std::string& fname, uint64_t* file_size) {
    if(boost::starts_with(fname, kBackupPath)) {
        // delete is not supported in snapshot
        return Status::NotSupported();
    }
    return EnvWrapper::GetFileSize(fname, file_size);
}

Status
snapshot_env::GetFileModificationTime(const std::string& fname,
                                      uint64_t*          file_mtime) {
    if(boost::starts_with(fname, kBackupPath)) {
        // GetFileModificationTime is not supported in snapshot
        return Status::NotSupported();
    }
    return EnvWrapper::GetFileSize(fname, file_mtime);
}

Status
snapshot_env::RenameFile(const std::string& src, const std::string& target) {
    using namespace __internal;

    if(boost::starts_with(src, kBackupPath)) {
        FC_ASSERT(boost::starts_with(target, kBackupPath));

        // only write to snapshot when close
        writer_->write_section(target, [&src](auto& rw) {
            size_t sz   = src.size();
            int    type = kLink;

            rw.add_row("type", (char*)&type,  sizeof(type));
            rw.add_row("size", (char*)&sz,    sizeof(sz));
            rw.add_row("raw",  src.data(),    sz);
        });
        created_files_.emplace_back(target);
        return Status::OK();
    }
    return EnvWrapper::RenameFile(src, target);
}

Status
snapshot_env::LinkFile(const std::string& src, const std::string& target) {
    return Status::NotSupported();
}

Status
snapshot_env::LockFile(const std::string& fname, FileLock** lock) {
    return Status::NotSupported();
}

Status
snapshot_env::UnlockFile(FileLock* lock) {
    return Status::NotSupported();
}

void
token_database_snapshot::add_to_snapshot(snapshot_writer_ptr writer, const token_database& db) {
    using namespace rocksdb;
    using namespace __internal;

    auto env = snapshot_env(writer, nullptr);

    BackupEngine* backup_engine;
    
    auto s = BackupEngine::Open(Env::Default(), BackupableDBOptions(kBackupPath, &env), &backup_engine);
    EVT_ASSERT(s.ok(), snapshot_exception, "Cannot open snapshot for token database, reason: ${d}", ("d",s.getState()));

    s = db.db_->Flush(rocksdb::FlushOptions());
    EVT_ASSERT(s.ok(), tokendb_exception, "Flush rocksdb failed, reason: ${d}", ("d",s.getState()));

    s = backup_engine->CreateNewBackup(db.db_, true);
    EVT_ASSERT(s.ok(), snapshot_exception, "Backup to snapshot for token database failed, reason: ${d}", ("d",s.getState()));

    delete backup_engine;

    if(db.savepoints_size() > 0) {
        auto ss = std::stringstream();
        ss.exceptions(std::fstream::failbit | std::fstream::badbit);

        db.persist_savepoints(ss);

        auto   buf  = ss.str();
        size_t sz   = buf.size();
        int    type = kRaw;

        writer->write_section(config::tokendb_persisit_filename, [&](auto& rw) {
            rw.add_row("type", (char*)&type, sizeof(type));
            rw.add_row("size", (char*)&sz, sizeof(sz));
            rw.add_row("raw",  buf.data(), buf.size());
        });
    }
}

void
token_database_snapshot::read_from_snapshot(snapshot_reader_ptr reader, token_database& db) {
    using namespace rocksdb;

    auto env = snapshot_env(nullptr, reader);

    BackupEngineReadOnly* backup_engine;
    
    auto s = BackupEngineReadOnly::Open(Env::Default(), BackupableDBOptions(kBackupPath, &env), &backup_engine);
    EVT_ASSERT(s.ok(), snapshot_exception, "Cannot open snapshot for token database, reason: ${d}", ("d",s.getState()));

    db.close(false);
    db.open(false);

    auto a = db.exists_domain("test-domain-2");

    db.close(false);

    s = backup_engine->RestoreDBFromLatestBackup(db.db_path_, kBackupPath);
    EVT_ASSERT(s.ok(), snapshot_exception, "Restore from snapshot for token database failed, reason: ${d}", ("d",s.getState()));

    delete backup_engine;

    if(reader->has_section(config::tokendb_persisit_filename)) {
        auto data = std::string();

        reader->read_section(config::tokendb_persisit_filename, [&data](auto& rr) {
            auto tmp = std::string();
            auto type = int();
            auto sz = size_t();

            rr.read_row(tmp, sizeof(type));
            type = *(int*) tmp.data();

            rr.read_row(tmp, sizeof(sz));
            sz = *(size_t*) tmp.data();

            rr.read_row(tmp, sz);
            data = std::move(tmp);
        });

        auto ss = std::stringstream(data);
        ss.exceptions(std::fstream::failbit | std::fstream::badbit);

        db.load_savepoints(ss);
    }

    db.open(false);
}

}}  // namespace evt::chain
