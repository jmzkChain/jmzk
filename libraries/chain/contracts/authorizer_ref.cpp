/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <evt/chain/exceptions.hpp>

namespace fc {

using namespace evt::chain;
using namespace evt::chain::contracts;
using evt::chain::contracts::authorizer_ref;

void
to_variant(const authorizer_ref& ref, fc::variant& v) {
    switch(ref.type()) {
    case authorizer_ref::account_t: {
        auto& key = ref.get_account();
        v = "[A] " + (std::string)key;
        break;
    }
    case authorizer_ref::owner_t: {
        v = "[G] .OWNER";
        break;
    }
    case authorizer_ref::group_t: {
        auto& name = ref.get_group();
        v = "[G] " + (std::string)name;
        break;
    }
    default: {
        EVT_ASSERT(false, authorizer_ref_type_exception, "Not valid ref type: ${type}", ("type",ref.type()));
    }
    }  // switch
}

void
from_variant(const fc::variant& v, authorizer_ref& ref) {
    auto& str = v.get_string();
    EVT_ASSERT(str.size() > 4, authorizer_ref_type_exception, "Not valid authorizer ref string");    
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
    EVT_ASSERT(false, authorizer_ref_type_exception, "Unknown authorizer ref prefix: ${prefix}", ("prefix",str.substr(0,4)));
}

}  // namespace fc