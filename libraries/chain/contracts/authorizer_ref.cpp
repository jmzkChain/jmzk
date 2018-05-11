/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain { namespace contracts {

}}}  // namespac evt::chain::contracts

namespace fc {

using namespace evt::chain;
using namespace evt::chain::contracts;

void
to_variant(const evt::chain::contracts::authorizer_ref& ref, fc::variant& v) {
    if(ref.is_account_ref()) {
        auto& key = ref.get_account();
        v = "[A] " + (std::string)key;
    }
    else {
        EVT_ASSERT(ref.is_group_ref(), serialization_exception, "Not excepted ref type");
        auto& gkey = ref.get_group();
        v = "[G] " + gkey.to_string();
    }
}

void
from_variant(const fc::variant& v, evt::chain::contracts::authorizer_ref& ref) {
    auto& str = v.get_string();
    EVT_ASSERT(str.size() > 4, deserialization_exception, "Not valid authorizer ref string");    
    if(boost::starts_with(str, "[A] ")) {
        auto key = public_key_type(str.substr(4));
        ref = authorizer_ref(key);
        return;
    }
    else if(boost::starts_with(str, "[G] ")) {
        auto gid = group_id::from_string(str.substr(4));
        ref = authorizer_ref(gid);
        return;
    }
    EVT_ASSERT(false, deserialization_exception, "Unknown authorizer ref prefix");
}

}  // namespace fc