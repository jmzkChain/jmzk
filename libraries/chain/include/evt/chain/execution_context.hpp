/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <boost/noncopyable.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/types.hpp>

namespace evt { namespace chain {

struct action_ver {
    name   name;
    int    version;
    string type;
};

class execution_context : boost::noncopyable {
public:
    virtual int index_of(name act) const = 0;
    virtual std::string get_acttype_name(int index) const = 0;
    virtual int set_version(name act, int ver) = 0;
    virtual std::vector<action_ver> get_current_actions() const = 0;
};

// helper method to add const lvalue reference to type object
template<typename T>
using add_clr_t = typename std::add_const<typename std::add_lvalue_reference<T>::type>::type;

}}  // namespace evt::chain

FC_REFLECT(evt::chain::action_ver, (name)(version)(type));