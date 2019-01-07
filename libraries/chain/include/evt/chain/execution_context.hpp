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

class execution_context : boost::noncopyable {
public:
    virtual int index_of(name act) const = 0;
    virtual std::string get_acttype_name(int index) const = 0;
};

// helper method to add const lvalue reference to type object
template<typename T>
using add_clr_t = typename std::add_const<typename std::add_lvalue_reference<T>::type>::type;

}}  // namespace evt::chain
