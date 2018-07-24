/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fc/container/flat_fwd.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

class evt_link {
public:
    struct segment {
        segment(uint8_t key, uint32_t intv)
            : key(key), intv(intv) {}

        segment(uint8_t key, const std::string& strv)
            : key(key), intv(0), strv(strv) {}

        uint8_t     key;
        uint32_t    intv;
        std::string strv;
    };

public:
    static evt_link parse_from_evtli(const std::string& str);

public:
    const segment& get_segment(uint32_t key) const;
    bool has_segment(uint32_t key) const;

    const fc::flat_map<uint32_t, segment>& get_segments() const { return segments_; }
    const fc::flat_set<signature_type>& get_signatures() const {return signatures_; }

public:
    fc::flat_set<public_key_type> restore_keys() const;

private:
    fc::flat_map<uint32_t, segment> segments_;
    fc::flat_set<signature_type>    signatures_;

    bytes segments_bytes_;
};

}}}  // namespac evt::chain::contracts
