/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_link.hpp>

#include <string.h>
#include <algorithm>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/endian/conversion.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <evt/chain/exceptions.hpp>

using namespace boost::multiprecision;

// pay: 2(header) + 5(time) + 5(max_pay) + 7(symbol) + 16(link-id)  = 35
// pass: 2(header) + 5(time) + 22(domain) + 22(token) + 16(link-id) = 67
// sigs: 65 * 3 = 195
using bigint_segs = number<cpp_int_backend<536, 536, unsigned_magnitude, checked, void>>;
using bigint_sigs = number<cpp_int_backend<1560, 1560, unsigned_magnitude, checked, void>>;

namespace evt { namespace chain { namespace contracts {

namespace __internal {

const char* ALPHABETS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$+-/:*";

template<typename T>
bytes
decode(const std::string& nums, int pos, int end) {
    auto num = T{0};
    auto pz  = (int)nums.find_first_not_of('0', pos);

    for(auto i = pz; i < end; i++) {
        auto c = strchr(ALPHABETS, nums[i]);
        FC_ASSERT(c != nullptr, "invalid character in evt-link");
        num *= 42;
        num += (c - ALPHABETS);
    }

    auto b = bytes();
    for(auto i = 0; i < (pz - pos); i++) {
        b.emplace_back(0);
    }
    boost::multiprecision::export_bits(num, std::back_inserter(b), 8);

    return b;
}

fc::flat_map<uint8_t, evt_link::segment>
parse_segments(const bytes& b, uint16_t& header) {
    FC_ASSERT(b.size() > 2);

    auto h  = *(uint16_t*)&b[0];
    header  = boost::endian::big_to_native(h);

    auto segs = fc::flat_map<uint8_t, evt_link::segment>();

    auto i  = 2u;
    auto pk = 0u;
    while(i < b.size()) {
        auto k = (uint8_t)b[i];
        EVT_ASSERT(k > pk, evt_link_exception, "Segments are not ordered by keys");
        pk = k;

        if(k <= 20) {
            FC_ASSERT(b.size() > i + 1); // value is 1 byte
            auto v = (uint8_t)b[i + 1];
            segs.emplace(k, evt_link::segment(k, v));

            i += 2;
        }
        else if(k <= 40) {
            FC_ASSERT(b.size() > i + 2); // value is 2 byte
            auto v = *(uint16_t*)(&b[i] + 1);
            segs.emplace(k, evt_link::segment(k, boost::endian::big_to_native(v)));

            i += 3;
        }
        else if(k <= 90) {
            FC_ASSERT(b.size() > i + 4); // value is 4 byte
            auto v = *(uint32_t*)(&b[i] + 1);
            segs.emplace(k, evt_link::segment(k, boost::endian::big_to_native(v)));

            i += 5;
        }
        else if(k <= 180) {
            auto sz = 0u;
            auto s  = 0u;

            if(k > 155 && k <= 165) {
                sz = 16;  // uuid, sizeof(uint128_t)
            }
            else {
                FC_ASSERT(b.size() > i + 1); // first read length byte
                sz = (uint8_t)b[i + 1];
                s  = 1;
            }


            if(sz > 0) {
                FC_ASSERT(b.size() > i + s + sz);
                segs.emplace(k, evt_link::segment(k, std::string(&b[i] + 1 + s, sz)));
            }
            else {
                segs.emplace(k, evt_link::segment(k, std::string()));
            }

            i += 1 + s + sz;
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

    EVT_ASSERT(str.size() < 400, evt_link_exception, "Link is too long, max length allowed: 400");
    EVT_ASSERT(str.size() > 20, evt_link_exception, "Link is too short");

    size_t start = 0;
    const char* uri_schema = "https://evt.li/";
    if(memcmp(str.data(), uri_schema, strlen(uri_schema)) == 0) {
        start = strlen(uri_schema);
    }

    auto d = str.find_first_of('_', start);
    
    auto bsegs = bytes();
    auto bsigs = bytes();

    if(d == std::string::npos) {
        bsegs = decode<bigint_segs>(str, start, str.size());
    }
    else {
        bsegs = decode<bigint_segs>(str, start, d);
        bsigs = decode<bigint_sigs>(str, d + 1, str.size());
    }

    auto link = evt_link();

    link.segments_       = parse_segments(bsegs, link.header_);
    link.signatures_     = parse_signatures(bsigs);

    return link;
}

const evt_link::segment&
evt_link::get_segment(uint8_t key) const {
    auto it = segments_.find(key);
    EVT_ASSERT(it != segments_.end(), evt_link_no_key_exception, "Cannot find segment for key: ${k}", ("k",key));

    return it->second;
}

bool
evt_link::has_segment(uint8_t key) const {
    return segments_.find(key) != segments_.end();
}

fc::sha256
evt_link::digest() const {
    auto enc = fc::sha256::encoder();

    auto h = boost::endian::native_to_big(header_);
    static_assert(sizeof(h) == 2);  // uint16_t
    enc.write((char*)&h, sizeof(h));

    for(auto& seg_ : segments_) {
        auto  key = seg_.first;
        auto& seg = seg_.second;

        static_assert(sizeof(key) == 1);  // uint8_t
        enc.write((char*)&key, sizeof(key));
        if(key <= 20) {
            auto v = *seg.intv;
            enc.write((char*)&v, 1);
        }
        else if(key <= 40) {
            auto v = boost::endian::native_to_big((uint16_t)*seg.intv);
            enc.write((char*)&v, 2);
        }
        else if(key <= 90) {
            auto v = boost::endian::native_to_big((uint32_t)*seg.intv);
            enc.write((char*)&v, 4);
        }
        else if(key <= 180) {
            if(key > 155 && key <= 165) {
                FC_ASSERT(seg.strv->size() == 16);
                enc.write((char*)seg.strv->data(), 16);
            }
            else {
                auto s = seg.strv->size();
                FC_ASSERT(s < 255); // 1 byte length

                enc.write((char*)&s, 1);
                enc.write(seg.strv->data(), s);
            }
        }
    }

    return enc.result();
}

fc::flat_set<public_key_type>
evt_link::restore_keys() const {
    auto hash = digest();
    auto keys = fc::flat_set<public_key_type>();

    keys.reserve(signatures_.size());
    for(auto& sig : signatures_) {
        keys.emplace(public_key_type(sig, hash));
    }
    return keys;
}

void
evt_link::add_segment(const segment& seg) {
    auto it = segments_.emplace(seg.key, seg);
    if(!it.second) {
        // existed, replace old one
        it.first->second = seg;
    }
}

void
evt_link::add_signature(const signature_type& sig) {
    signatures_.emplace(sig);
}

void
evt_link::sign(const private_key_type& pkey) {
    signatures_.emplace(pkey.sign(digest()));
}

}}}  // namespac evt::chain::contracts

namespace fc {

using evt::chain::contracts::evt_link;

void
from_variant(const fc::variant& v, evt_link& link) {
    link = evt_link::parse_from_evtli(v.get_string());
}

void
to_variant(const evt::chain::contracts::evt_link& link, fc::variant& v) {
    auto vo   = fc::mutable_variant_object();
    auto segs = fc::variants();
    auto sigs = fc::variants();
    
    for(auto& it : link.get_segments()) {
        auto  sego = fc::mutable_variant_object();
        auto& seg  = it.second;

        sego["key"] = seg.key;
        if(seg.key <= 90) {
            sego["value"] = *seg.intv;
        }
        else if(seg.key <= 155) {
            sego["value"] = *seg.strv;
        }
        else if(seg.key <= 180) {
            sego["value"] = fc::to_hex(seg.strv->c_str(), seg.strv->size());
        }
        segs.emplace_back(std::move(sego));
    }

    for(auto& sig : link.get_signatures()) {
        sigs.emplace_back((std::string)sig);
    }

    vo["header"]     = link.get_header();
    vo["segments"]   = segs;
    vo["signatures"] = sigs;
    vo["keys"]       = link.restore_keys();

    v = std::move(vo);
}

}  // namespace fc
