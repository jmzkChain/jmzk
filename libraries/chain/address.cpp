/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/address.hpp>
#include <evt/chain/exceptions.hpp>
#include <fc/crypto/base58.hpp>

namespace evt { namespace chain {

const std::string reserved_key = "EVT00000000000000000000000000000000000000000000000000";

std::string
address::to_string() const {
    switch(type()) {
    case reserved_t: {
        return reserved_key;
    }
    case public_key_t: {
        return (std::string)this->get_public_key();
    }
    case generated_t: {
        auto str = std::string();
        str.reserve(53);

        str.append("EVT0");
        auto gen = storage_.get<generated_type>();
        auto hash = fc::to_base58((char*)&gen, sizeof(gen));
        str.append(53 - 4 - hash.size(), '0');
        str.append(std::move(hash));

        return str;
    }
    default: {
        EVT_ASSERT(false, address_type_exception, "Not valid address type: ${type}", ("type",type()));
    }
    }  // switch
}

address
address::from_string(const std::string& str) {
    EVT_ASSERT(str.size() == 53, address_type_exception, "Address is empty");

    address addr;
    // fast path
    if(str[3] != '0') {
        addr.set_public_key((public_key_type)str);
        return addr;
    }

    if(str == reserved_key) {
        addr.set_reserved();
        return addr;
    }

    auto gen = address::generated_type();
    auto hash = str.substr(str.find_first_not_of('0', 4));
    fc::from_base58(hash, (char*)&gen, sizeof(gen));
    addr.set_generated(std::get<address::prefix_idx>(gen), std::get<address::key_idx>(gen));

    return addr;
}

}}  // namespace evt::chain

namespace fc {

void
to_variant(const evt::chain::address& addr, fc::variant& v) {
    v = addr.to_string();
}

void
from_variant(const fc::variant& v, evt::chain::address& addr) {
    addr = evt::chain::address::from_string(v.get_string());
}

}  // namespace fc