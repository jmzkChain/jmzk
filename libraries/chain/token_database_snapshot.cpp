#include <evt/chain/token_database_snapshot.hpp>

namespace evt { namespace chain {

Status
NewSequentialFile(const std::string&               fname,
                  std::unique_ptr<SequentialFile>* result,
                  const EnvOptions&                options) {
}

Status
NewRandomAccessFile(const std::string&                 fname,
                    std::unique_ptr<RandomAccessFile>* result,
                    const EnvOptions&                  options) {

}

Status
NewWritableFile(const std::string&             fname,
                std::unique_ptr<WritableFile>* result,
                const EnvOptions&              options) {

}

Status
ReuseWritableFile(const std::string&             fname,
                  const std::string&             old_fname,
                  std::unique_ptr<WritableFile>* result,
                  const EnvOptions&              options) {

}

Status
NewDirectory(const std::string&          name,
             std::unique_ptr<Directory>* result) {

}

Status
FileExists(const std::string& fname) {

}

Status GetChildren(const std::string& dir, std::vector<std::string>* result) {

}

Status
DeleteFile(const std::string& fname) {

}

Status
CreateDir(const std::string& dirname) {

}

Status
CreateDirIfMissing(const std::string& dirname) {

}

Status
DeleteDir(const std::string& dirname) {

}

Status
GetFileSize(const std::string& fname, uint64_t* file_size) {

}

Status
GetFileModificationTime(const std::string& fname,
                        uint64_t*          file_mtime) {

}

Status
RenameFile(const std::string& src, const std::string& target) {

}

Status
LinkFile(const std::string& src, const std::string& target) {

}

LockFile(const std::string& fname, FileLock** lock) {

}

Status
UnlockFile(FileLock* lock) {

}

}}  // namespace evt::chain