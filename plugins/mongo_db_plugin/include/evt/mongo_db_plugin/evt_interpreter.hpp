/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>
#include <functional>
#include <mongocxx/database.hpp>
#include <evt/chain/trace.hpp>

namespace evt {

using evt::chain::transaction_trace;

class interpreter_impl;
using interpreter_impl_ptr = std::shared_ptr<interpreter_impl>;

class evt_interpreter {
public:
    evt_interpreter();

public:
    void initialize_db(const mongocxx::database& db);
    void process_trx(const transaction_trace& trx_trace);

private:
    interpreter_impl_ptr my_;
};

}  // namespace evt