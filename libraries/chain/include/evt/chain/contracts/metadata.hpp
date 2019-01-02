/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>

namespace evt { namespace chain { namespace contracts {

using meta_key   = name128;
using meta_value = string;

struct meta {
    meta() = default;
    meta(const meta_key& key, const string& value, const authorizer_ref& creator)
        : key(key)
        , value(value)
        , creator(creator) {}

    meta_key       key;
    meta_value     value;
    authorizer_ref creator;
};
using meta_list = small_vector<meta, 4>;

}}}  // namespac evt::chain::contracts

FC_REFLECT(evt::chain::contracts::meta, (key)(value)(creator));
