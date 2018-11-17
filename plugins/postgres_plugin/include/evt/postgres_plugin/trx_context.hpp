/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
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

private:
    fmt::memory_buffer trx_buf_;

private:
    pg& db_;

private:
    friend class pg;
};

}  // namespace evt
