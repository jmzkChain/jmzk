/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <fc/crypto/sha256.hpp>

struct hello;

namespace fc {
class variant;
}  // namespace fc

namespace jmzk {

class net_plugin_impl;
struct handshake_message;

namespace chain_apis {
class read_only;
}

namespace chain {

struct chain_id_type : public fc::sha256 {
    using fc::sha256::sha256;

    chain_id_type(const fc::sha256& v) : fc::sha256(v) {}
    chain_id_type(fc::sha256&& v) : fc::sha256(std::move(v)) {}

    template <typename T>
    inline friend T&
    operator<<(T& ds, const chain_id_type& cid) {
        ds.write(cid.data(), cid.data_size());
        return ds;
    }

    template <typename T>
    inline friend T&
    operator>>(T& ds, chain_id_type& cid) {
        ds.read(cid.data(), cid.data_size());
        return ds;
    }

    void reflector_init() const;

private:
    chain_id_type() = default;

    friend class fc::variant;

    friend class jmzk::chain_apis::read_only;

    friend class jmzk::net_plugin_impl;
    friend struct jmzk::handshake_message;

    friend struct ::hello; // TODO: Rushed hack to support bnet_plugin. Need a better solution.
};

}  // namespace chain
}  // namespace jmzk

namespace fc {
class variant;
void to_variant(const jmzk::chain::chain_id_type& cid, fc::variant& v);
void from_variant(const fc::variant& v, jmzk::chain::chain_id_type& cid);
}  // namespace fc