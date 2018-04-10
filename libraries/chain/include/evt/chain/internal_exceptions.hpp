/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <evt/chain/exceptions.hpp>

#define EVT_DECLARE_INTERNAL_EXCEPTION( exc_name, seqnum, msg )  \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      internal_ ## exc_name,                                          \
      evt::chain::internal_exception,                            \
      3990000 + seqnum,                                               \
      msg                                                             \
      )

namespace evt { namespace chain {

FC_DECLARE_DERIVED_EXCEPTION( internal_exception, evt::chain::chain_exception, 3990000, "internal exception" )

EVT_DECLARE_INTERNAL_EXCEPTION( verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out" )
EVT_DECLARE_INTERNAL_EXCEPTION( verify_auth_account_not_found, 2, "Auth account not found" )

} } // evt::chain
