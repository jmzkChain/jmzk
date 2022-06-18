/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/jmzk_link.hpp>

#include <string.h>
#include <algorithm>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/endian/conversion.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <jmzk/chain/exceptions.hpp>

using namespace boost::multiprecision;

// pay: 2(header) + 5(time) + 5(max_pay) + 7(symbol) + 16(link-id)  = 35
// pass: 2(header) + 5(time) + 22(domain) + 22(token) + 16(link-id) = 67
// sigs: 65 * 3 = 195
using bigint_segs = number<cpp_int_backend<536, 536, unsigned_magnitude, checked, void>>;
using bigint_sigs = number<cpp_int_backend<1560, 1560, unsigned_magnitude, checked, void>>;

namespace jmzk { namespace chain { namespace contracts {

namespace internal {

const char* ALPHABETS   = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$+-/:*";
const int   MAX_BYTES   = 240;  // 195 / ((42 ^ 2) / 2048)
const char* URI_SCHEMA  = "https://jmzk.li/";
const char* URI_SCHEMA2 = "jmzklink://";

template<typename T>
bytes
decode(const std::string& nums, uint pos, uint end) {
    auto num = T{0};
    auto pz  = nums.find_first_not_of('0', pos);
    jmzk_ASSERT(pz != string::npos, jmzk_link_exception, "Invalid jmzk-Link");

    for(auto i = pz; i < end; i++) {
        auto c = strchr(ALPHABETS, nums[i]);
        FC_ASSERT(c != nullptr, "invalid character in jmzk-link");
        num *= 42;
        num += (c - ALPHABETS);
    }

    auto b = bytes();
    b.reserve(MAX_BYTES);
    for(auto i = 0u; i < (pz - pos); i++) {
        b.emplace_back(0);
    }
    boost::multiprecision::export_bits(num, std::back_inserter(b), 8);

    return b;
}

jmzk_link::segments_type
parse_segments(const bytes& b, uint16_t& header) {
    FC_ASSERT(b.size() > 2);

    auto h  = *(uint16_t*)&b[0];
    header  = boost::endian::big_to_native(h);

    auto segs = jmzk_link::segments_type();

    auto i  = 2u;
    auto pk = 0u;
    while(i < b.size()) {
        auto k = (uint8_t)b[i];
        jmzk_ASSERT(k > pk, jmzk_link_exception, "Segments are not ordered by keys");
        pk = k;

        if(k <= 20) {
            FC_ASSERT(b.size() > i + 1); // value is 1 byte
            auto v = (uint8_t)b[i + 1];
            segs.emplace(k, jmzk_link::segment(k, v));

            i += 2;
        }
        else if(k <= 40) {
            FC_ASSERT(b.size() > i + 2); // value is 2 byte
            auto v = *(uint16_t*)(&b[i] + 1);
            segs.emplace(k, jmzk_link::segment(k, boost::endian::big_to_native(v)));

            i += 3;
        }
        else if(k <= 90) {
            FC_ASSERT(b.size() > i + 4); // value is 4 byte
            auto v = *(uint32_t*)(&b[i] + 1);
            segs.emplace(k, jmzk_link::segment(k, boost::endian::big_to_native(v)));

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
                segs.emplace(k, jmzk_link::segment(k, std::string(&b[i] + 1 + s, sz)));
            }
            else {
                segs.emplace(k, jmzk_link::segment(k, std::string()));
            }

            i += 1 + s + sz;
        }
        else {
            jmzk_THROW(jmzk_link_exception, "Invalid key type: ${k}", ("k",k));
        }
    }
    return segs;
}

jmzk_link::signatures_type
parse_signatures(const bytes& b) {
    FC_ASSERT(b.size() > 0 && b.size() % 65 == 0);
    auto sigs = jmzk_link::signatures_type();

    for(auto i = 0u; i < b.size() / 65u; i++) {
        auto shim = fc::ecc::compact_signature();
        static_assert(sizeof(shim) == 65);
        memcpy(shim.data(), &b[0] + i * 65, 65);

        sigs.emplace(fc::ecc::signature_shim(shim));
    }
    return sigs;
}

}  // namespace internal

jmzk_link
jmzk_link::parse_from_jmzkli(const std::string& str) {
    using namespace internal;

    jmzk_ASSERT(str.size() < 400, jmzk_link_exception, "Link is too long, max length allowed: 400");
    jmzk_ASSERT(str.size() > 20, jmzk_link_exception, "Link is too short");

    size_t start = 0;
    if(memcmp(str.data(), URI_SCHEMA, strlen(URI_SCHEMA)) == 0) {
        start = strlen(URI_SCHEMA);
    }
    else if(memcmp(str.data(), URI_SCHEMA2, strlen(URI_SCHEMA2)) == 0) {
        start = strlen(URI_SCHEMA2);
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

    auto link = jmzk_link();

    link.segments_   = parse_segments(bsegs, link.header_);
    link.signatures_ = parse_signatures(bsigs);

    return link;
}

const jmzk_link::segment&
jmzk_link::get_segment(uint8_t key) const {
    auto it = segments_.find(key);
    jmzk_ASSERT(it != segments_.end(), jmzk_link_no_key_exception, "Cannot find segment for key: ${k}", ("k",key));

    return it->second;
}

bool
jmzk_link::has_segment(uint8_t key) const {
    return segments_.find(key) != segments_.end();
}

link_id_type
jmzk_link::get_link_id() const {
    auto& seg = get_segment(link_id);
    jmzk_ASSERT(seg.strv && seg.strv->size() == sizeof(link_id_type), jmzk_link_id_exception, "Not valid link id in this jmzk-Link");

    auto id = link_id_type();
    memcpy(&id, seg.strv->data(), sizeof(id));
    return id;
}

namespace internal {

template<typename Stream>
void
write_segments_bytes(const jmzk_link& link, Stream& stream) {
    auto h = boost::endian::native_to_big(link.get_header());
    static_assert(sizeof(h) == 2);  // uint16_t
    stream.write((char*)&h, sizeof(h));

    for(auto& seg_ : link.get_segments()) {
        auto  key = seg_.first;
        auto& seg = seg_.second;

        static_assert(sizeof(key) == 1);  // uint8_t
        stream.write((char*)&key, sizeof(key));
        if(key <= 20) {
            auto v = *seg.intv;
            stream.write((char*)&v, 1);
        }
        else if(key <= 40) {
            auto v = boost::endian::native_to_big((uint16_t)*seg.intv);
            stream.write((char*)&v, 2);
        }
        else if(key <= 90) {
            auto v = boost::endian::native_to_big((uint32_t)*seg.intv);
            stream.write((char*)&v, 4);
        }
        else if(key <= 180) {
            if(key > 155 && key <= 165) {
                FC_ASSERT(seg.strv->size() == 16);
                stream.write((char*)seg.strv->data(), 16);
            }
            else {
                auto s = seg.strv->size();
                FC_ASSERT(s < 255); // 1 byte length

                stream.write((char*)&s, 1);
                stream.write(seg.strv->data(), s);
            }
        }
    }
}

template<typename Stream>
struct stream_visitor : public fc::visitor<void> {
public:
    stream_visitor(Stream& stream) : stream_(stream) {}

public:
    template<typename Sig>
    void
    operator()(const Sig& sig) const {
        static_assert(sizeof(sig._data) == 65, "sig size is expected to be 65");
        stream_.write((char*)sig._data.data(), 65);
    }

private:
    Stream& stream_;
};

template<typename Stream>
void
write_signatures_bytes(const jmzk_link& link, Stream& stream) {
    auto visitor = stream_visitor<Stream>(stream);
    for(auto& sig : link.get_signatures()) {
        sig.view(visitor);
    }
}

template<typename Num>
void
encode(const bytes& b, size_t sz, std::string& str) {
    auto i = 0u;
    for(i = 0u; i < sz; i++) {
        if(b[i] == 0) {
            str.push_back('0');
        }
        else {
            break;
        }
    }

    auto num = Num{0};
    boost::multiprecision::import_bits(num, b.begin() + i, b.begin() + sz, 8);

    while(num >= 42) {
        auto r = num % 42;
        str.push_back(*(ALPHABETS + (int)r));
        num /= 42;
    }
    str.push_back(*(ALPHABETS + (int)num));
    std::reverse(str.begin() + i, str.end());
}

}  // namespace internal

fc::sha256
jmzk_link::digest() const {
    using namespace internal;

    auto enc = fc::sha256::encoder();
    write_segments_bytes(*this, enc);
    return enc.result();
}

std::string
jmzk_link::to_string(int prefix) const {
    using namespace internal;

    auto temp = bytes(MAX_BYTES);
    auto ds   = fc::datastream<char*>(temp.data(), temp.size());
    auto str  = string();

    if(prefix) {
        str.append(URI_SCHEMA);
    }

    auto str1 = string();
    write_segments_bytes(*this, ds);
    encode<bigint_segs>(temp, ds.tellp(), str1);
    str.append(str1);

    if(!signatures_.empty()) {
        str.push_back('_');

        ds.seekp(0);
        write_signatures_bytes(*this, ds);

        auto str2 = string();
        encode<bigint_sigs>(temp, ds.tellp(), str2);
        str.append(str2);
    }

    return str;
}

public_keys_set
jmzk_link::restore_keys() const {
    auto hash = digest();
    auto keys = public_keys_set();

    keys.reserve(signatures_.size());
    for(auto& sig : signatures_) {
        keys.emplace(public_key_type(sig, hash));
    }
    return keys;
}

void
jmzk_link::add_segment(const segment& seg) {
    auto it = segments_.emplace(seg.key, seg);
    if(!it.second) {
        // existed, replace old one
        it.first->second = seg;
    }
}

void
jmzk_link::remove_segment(uint8_t key) {
    segments_.erase(key);
}

void
jmzk_link::add_signature(const signature_type& sig) {
    signatures_.emplace(sig);
}

void
jmzk_link::sign(const private_key_type& pkey) {
    signatures_.emplace(pkey.sign(digest()));
}

}}}  // namespac jmzk::chain::contracts

namespace fc {

using jmzk::chain::contracts::jmzk_link;

void
from_variant(const fc::variant& v, jmzk_link& link) {
    link = jmzk_link::parse_from_jmzkli(v.get_string());
}

void
to_variant(const jmzk::chain::contracts::jmzk_link& link, fc::variant& v) {
    auto vo   = fc::mutable_variant_object();
    auto segs = fc::variants();
    auto sigs = fc::variants();
    auto keys = fc::variants();
    
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

    for(auto& key : link.restore_keys()) {
        keys.emplace_back((std::string)key);
    }


    vo["header"]     = link.get_header();
    vo["segments"]   = std::move(segs);
    vo["signatures"] = std::move(sigs);
    vo["keys"]       = std::move(keys);

    v = std::move(vo);
}

}  // namespace fc
