/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/authorizer_ref.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fmt/format.h>
#include <jmzk/chain/exceptions.hpp>

namespace jmzk { namespace chain { namespace contracts {

std::string
authorizer_ref::to_string() const {
    switch(this->type()) {
    case authorizer_ref::account_t: {
        auto& key = this->get_account();
        return fmt::format("[A] {}", (std::string)key);
    }
    case authorizer_ref::owner_t: {
        return "[G] .OWNER";
    }
    case authorizer_ref::group_t: {
        auto& name = this->get_group();
        return fmt::format("[G] {}", (std::string)name);
    }
    case authorizer_ref::script_t: {
        auto& name = this->get_script();
        return fmt::format("[S] {}", (std::string)name);
    }
    default: {
        jmzk_ASSERT(false, authorizer_ref_type_exception, "Not valid ref type: ${type}", ("type",this->type()));
    }
    }  // switch
}

}}}  // namespac jmzk::chain::contracts

namespace fc {

using namespace jmzk::chain;
using namespace jmzk::chain::contracts;
using jmzk::chain::contracts::authorizer_ref;

void
to_variant(const authorizer_ref& ref, fc::variant& v) {
    v = ref.to_string();
}

void
from_variant(const fc::variant& v, authorizer_ref& ref) {
    auto& str = v.get_string();
    jmzk_ASSERT(str.size() > 4, authorizer_ref_type_exception, "Not valid authorizer ref string");    
    if(boost::starts_with(str, "[A] ")) {
        auto key = public_key_type(str.substr(4));
        ref.set_account(key);
        return;
    }
    else if(boost::starts_with(str, "[G] ")) {
        if(str == "[G] .OWNER") {
            ref.set_owner();
        }
        else {
            auto name = (group_name)(str.substr(4));
            ref.set_group(name);
        }
        return;
    }
    else if(boost::starts_with(str, "[S] ")) {
        auto name = (script_name)(str.substr(4));
        ref.set_script(name);
        return;
    }
    jmzk_ASSERT(false, authorizer_ref_type_exception, "Unknown authorizer ref prefix: ${prefix}", ("prefix",str.substr(0,4)));
}

}  // namespace fc