/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

class controller;
struct action;

namespace contracts {

class lua_engine {
public:
    lua_engine();

public:
    bool invoke_filter(const controller& control, const action& act, const script_name& script);

};

}}}  // namespac evt::chain::contracts
