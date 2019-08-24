/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <optional>
#include <fc/container/flat_fwd.hpp>
#include <fc/static_variant.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/crypto/sha256.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

class evt_link {
public:
    struct segment {
        segment() = default;
        
        segment(uint8_t key, uint32_t intv)
            : key(key), intv(intv) {}

        segment(uint8_t key, const std::string& strv)
            : key(key), strv(strv) {}

        uint8_t                    key;
        std::optional<uint32_t>    intv;
        std::optional<std::string> strv;
    };

public:
    enum type_id {
        timestamp        = 42,
        max_pay          = 43,
        symbol_id        = 44,
        fixed_amount     = 46,
        domain           = 91,
        token            = 92,
        max_pay_str      = 94,
        address          = 95,
        fixed_amount_str = 97,
        memo             = 98,
        redirect_url     = 99,
        link_id          = 156
    };

    enum flag {
        version1  = 1,
        everiPass = 2,
        everiPay  = 4,
        destroy   = 8
    };

public:
    using segments_type   = fc::flat_map<uint8_t, segment, std::less<uint8_t>, fc::small_vector<std::pair<uint8_t, segment>, 6>>;
    using signatures_type = fc::flat_set<signature_type, std::less<signature_type>, fc::small_vector<signature_type, 2>>;

public:
    static evt_link parse_from_evtli(const std::string& str);
    std::string to_string(int prefix = 0) const;

public:
    uint16_t get_header() const { return header_; }

    const segment& get_segment(uint8_t key) const;
    bool has_segment(uint8_t key) const;

    link_id_type get_link_id() const;
    const segments_type& get_segments() const { return segments_; }
    const signatures_type& get_signatures() const { return signatures_; }

public:
    void set_header(uint16_t header) { header_ = header; }
    void add_segment(const segment&);
    void remove_segment(uint8_t key);
    void add_signature(const signature_type&);
    void clear_signatures() { signatures_.clear(); }
    void sign(const private_key_type&);

public:
    fc::sha256 digest() const;
    public_keys_set restore_keys() const;

private:
    uint16_t        header_;
    segments_type   segments_;
    signatures_type signatures_;

private:
    friend struct fc::reflector<evt_link>;
};

}}}  // namespac evt::chain::contracts

namespace fc {

class variant;
void to_variant(const evt::chain::contracts::evt_link& link, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::contracts::evt_link& link);

}  // namespace fc

FC_REFLECT(evt::chain::contracts::evt_link::segment, (key)(intv)(strv));
FC_REFLECT(evt::chain::contracts::evt_link, (header_)(segments_)(signatures_));
