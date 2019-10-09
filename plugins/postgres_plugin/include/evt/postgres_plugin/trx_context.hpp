/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string_view>
#include <memory>
#include <fmt/format.h>
#include <evt/postgres_plugin/evt_pg.hpp>

namespace evt {

struct trx_context : boost::noncopyable {
public:
    trx_context(pg& pg) : db_(pg) {}

public:
    void
    commit() {
        db_.commit_trx_context(*this);
    }

    void set_trx_id(const std::string& trx_id) { trx_id_ = trx_id; }
    void set_timestamp(const std::string& timestamp) { timestamp_ = timestamp; }
    void set_trx_num(const int64_t trx_num) { trx_num_ = trx_num; }
  
    const std::string_view& trx_id() const { return trx_id_; }
    const int64_t trx_num() const { return trx_num_; }
    const std::string_view& timestamp() const { return timestamp_; }

private:
    fmt::memory_buffer trx_buf_;

private:
    pg&              db_;
    std::string_view trx_id_;
    int64_t          trx_num_;
    std::string_view timestamp_;

private:
    friend class pg;
};

}  // namespace evt
