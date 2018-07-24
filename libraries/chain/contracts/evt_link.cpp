/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_link.hpp>

#include <algorithm>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/endian/conversion.hpp>

#include <fc/crypto/elliptic.hpp>
#include <evt/chain/exceptions.hpp>

using namespace boost::multiprecision;

using bigint_segs = number<cpp_int_backend<576, 576, unsigned_magnitude, checked, void>>;
using bigint_sigs = number<cpp_int_backend<1560, 1560, unsigned_magnitude, checked, void>>;

namespace evt { namespace chain { namespace contracts {

namespace __internal {

template<typename T>
bytes
decode(const std::string& nums, int pos, int end) {
    auto num = T{0};
    for(auto i = pos; i < end; i++) {
        auto c = nums[i];
        FC_ASSERT(c >= '0' && c <= '9', "invalid character in evt-link");
        num *= 10;
        num += (c - '0');
    }

    auto b = bytes();
    for(auto i = 0u; i < nums.find_first_not_of('0'); i++) {
        b.emplace_back(0);
    }
    boost::multiprecision::export_bits(num, std::back_inserter(b), 8);

    return b;
}

fc::flat_map<uint32_t, evt_link::segment>
parse_segments(const bytes& b) {
    FC_ASSERT(b.size() > 0);
    auto segs = fc::flat_map<uint32_t, evt_link::segment>();

    auto i = 0u;
    while(i < b.size()) {
        auto k = (uint8_t)b[i];
        if(k <= 40) {
            FC_ASSERT(b.size() > i + 1); // value is 1 byte
            auto v = b[i + 1];
            segs.emplace(k, evt_link::segment(k, v));

            i += 2;
        }
        else if(k <= 90) {
            FC_ASSERT(b.size() > i + 4); // value is 4 byte
            auto v = *(uint32_t*)(&b[0] + i + 1);
            segs.emplace(k, evt_link::segment(k, boost::endian::big_to_native(v)));

            i += 5;
        }
        else if(k <= 180) {
            FC_ASSERT(b.size() > i + 1); // first read length byte
            auto sz = (uint8_t)b[i + 1];
            if(sz > 0) {
                FC_ASSERT(b.size() > i + 1 + sz);
                segs.emplace(k, evt_link::segment(k, std::string(&b[0] + i + 2, sz)));
            }
            else {
                segs.emplace(k, evt_link::segment(k, std::string()));
            }

            i += 2 + sz;
        }
        else {
            EVT_THROW(evt_link_exception, "Invalid key type: ${k}", ("k",k));
        }
    }
    return segs;
}

fc::flat_set<signature_type>
parse_signatures(const bytes& b) {
    FC_ASSERT(b.size() > 0 && b.size() % 65 == 0);
    auto sigs = fc::flat_set<signature_type>();

    for(auto i = 0u; i < b.size() / 65u; i++) {
        auto shim = fc::ecc::compact_signature();
        static_assert(sizeof(shim) == 65);
        memcpy(shim.data, &b[0] + i * 65, 65);

        sigs.emplace(fc::ecc::signature_shim(shim));
    }
    return sigs;
}

}  // namespace __internal

evt_link
evt_link::parse_from_evtli(const std::string& str) {
    using namespace __internal;

    EVT_ASSERT(str.size() < 650, evt_link_exception, "Link is too long, max length allowed: 650");

    const char* uri_schema = "https://evt.li/";
    if(memcmp(str.data(), uri_schema, strlen(uri_schema)) != 0) {
        EVT_THROW(evt_link_exception, "Link uri schema is not valid");
    }

    auto d = str.find_first_of('-', strlen(uri_schema));
    
    auto bsegs = bytes();
    auto bsigs = bytes();

    if(d == std::string::npos) {
        bsegs = decode<bigint_segs>(str, strlen(uri_schema), str.size());
    }
    else {
        bsegs = decode<bigint_segs>(str, strlen(uri_schema), d);
        bsigs = decode<bigint_sigs>(str, d + 1, str.size());
    }

    auto link = evt_link();

    link.segments_       = parse_segments(bsegs);
    link.signatures_     = parse_signatures(bsigs);
    link.segments_bytes_ = std::move(bsegs);

    return link;
}

const evt_link::segment&
evt_link::get_segment(uint32_t key) const {
    auto it = segments_.find(key);
    EVT_ASSERT(it != segments_.end(), evt_link_no_key_exception, "Cannot find segment for key: ${k}", ("k",key));

    return it->second;
}

bool
evt_link::has_segment(uint32_t key) const {
    return segments_.find(key) != segments_.end();
}

fc::flat_set<public_key_type>
evt_link::restore_keys() const {
    auto keys = fc::flat_set<public_key_type>();
    auto hash = fc::sha256::hash(&segments_bytes_[0], segments_bytes_.size());
    for(auto& sig : signatures_) {
        keys.emplace(public_key_type(sig, hash));
    }
    return keys;
}

}}}  // namespac evt::chain::contracts
