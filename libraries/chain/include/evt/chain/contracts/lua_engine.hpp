/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>

namespace evt { namespace chain {

class token_database;
struct action;

namespace contracts {

class lua_engine {
public:
    lua_engine();

public:
    void invoke_filter(const token_database& tokendb, const action& act, const std::string& filter_fuc);

public:
    static void set_token_db(const token_database& tokendb);
    static const token_database& get_token_db();

};

}}}  // namespac evt::chain::contracts
