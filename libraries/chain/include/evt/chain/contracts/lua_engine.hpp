/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <string>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

class controller;
struct action;

namespace contracts {

class lua_engine {
public:
    lua_engine();

public:
    bool invoke_filter(const controller& control, const action& act, const script_name& script);

};

}}}  // namespac jmzk::chain::contracts
