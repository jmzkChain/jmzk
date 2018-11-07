#include <evt/chain/token_database_snapshot.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <rocksdb/utilities/backupable_db.h>

namespace evt { namespace chain {

const char* kBackupPath = "/tmp/backup";

snapshot_env::snapshot_env() : rocksdb::EnvWrapper(rocksdb::Env::Default()) {}

Status
snapshot_env::NewSequentialFile(const std::string&               fname,
                  std::unique_ptr<SequentialFile>* result,
                  const EnvOptions&                options) {
    printf("NewSequentialFile\n");
    return Status::OK();
}

Status
snapshot_env::NewRandomAccessFile(const std::string&                 fname,
                    std::unique_ptr<RandomAccessFile>* result,
                    const EnvOptions&                  options) {
    printf("NewRandomAccessFile\n");
    return Status::OK();
}

Status
snapshot_env::NewWritableFile(const std::string&             fname,
                std::unique_ptr<WritableFile>* result,
                const EnvOptions&              options) {
    printf("NewWritableFile %s\n", fname.c_str());
    return Status::OK();
}

Status
snapshot_env::ReuseWritableFile(const std::string&             fname,
                  const std::string&             old_fname,
                  std::unique_ptr<WritableFile>* result,
                  const EnvOptions&              options) {
    printf("ReuseWritableFile\n");
    return Status::OK();
}

Status
snapshot_env::NewDirectory(const std::string&          name,
             std::unique_ptr<Directory>* result) {
    printf("NewDirectory %s\n", name.c_str());
    return Status::OK();
}

Status
snapshot_env::FileExists(const std::string& fname) {
    printf("FileExists %s\n", fname.c_str());
    if(boost::starts_with(fname, kBackupPath) && boost::ends_with(fname, "/")) {
        // no need to check folder existance in target directory
        return Status::OK();
    }
    return EnvWrapper::FileExists(fname);
}

Status
snapshot_env::GetChildren(const std::string& dir, std::vector<std::string>* result) {
    printf("GetChildren: %s\n", dir.c_str());
    if(boost::starts_with(dir, kBackupPath)) {
        // there's no files in target forlder
        return Status::OK();
    }
    return EnvWrapper::GetChildren(dir, result);
}

Status
snapshot_env::DeleteFile(const std::string& fname) {
    printf("DeleteFile\n");
    return Status::OK();
}

Status
snapshot_env::CreateDir(const std::string& dirname) {
    printf("CreateDir %s\n", dirname.c_str());
    // no need to create directory
    return Status::OK();
}

Status
snapshot_env::CreateDirIfMissing(const std::string& dirname) {
    printf("CreateDirIfMissing %s\n", dirname.c_str());
    // no need to create directory
    return Status::OK();
}

Status
snapshot_env::DeleteDir(const std::string& dirname) {
    printf("DeleteDir\n");
    return Status::OK();
}

Status
snapshot_env::GetFileSize(const std::string& fname, uint64_t* file_size) {
    printf("GetFileSize\n");
    return Status::OK();
}

Status
snapshot_env::GetFileModificationTime(const std::string& fname,
                        uint64_t*          file_mtime) {
    printf("GetFileModificationTime\n");
    return Status::OK();
}

Status
snapshot_env::RenameFile(const std::string& src, const std::string& target) {
    printf("RenameFile\n");
    return Status::OK();
}

Status
snapshot_env::LinkFile(const std::string& src, const std::string& target) {
    printf("LinkFile\n");
    return Status::OK();
}

Status
snapshot_env::LockFile(const std::string& fname, FileLock** lock) {
    printf("LockFile\n");
    return Status::OK();
}

Status
snapshot_env::UnlockFile(FileLock* lock) {
    printf("UnlockFile\n");
    return Status::OK();
}

void
token_database_snapshot::add_to_snapshot(const snapshot_writer_ptr& snapshot) {
    using namespace rocksdb;

    auto env = snapshot_env();

    BackupEngine* backup_engine;
    auto s = BackupEngine::Open(Env::Default(), BackupableDBOptions(kBackupPath, &env), &backup_engine);
    assert(s.ok());

    s = backup_engine->CreateNewBackup(db_.db_);
    assert(s.ok());
}

}}  // namespace evt::chain