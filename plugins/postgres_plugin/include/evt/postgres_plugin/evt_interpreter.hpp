/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>
#include <functional>
#include <evt/chain/trace.hpp>

namespace evt {

using evt::chain::transaction;

class interpreter_impl;
using interpreter_impl_ptr = std::shared_ptr<interpreter_impl>;

struct write_context;

class add_trx {
public:
    evt_interpreter();

public:
    void process_trx(const transaction& trx, write_context&);

private:
    interpreter_impl_ptr my_;
};

}  // namespace evt