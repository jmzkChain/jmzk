/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <tuple>
#include <fmt/format.h>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>
#include <jmzk/chain/types.hpp>
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/utilities/common.hpp>

namespace jmzk { namespace chain {

class address : public fc::reflect_init {
public:
    enum addr_type { reserved_t = 0, public_key_t, generated_t };

private:
    enum { prefix_idx = 0, nonce_idx, key_idx };

    using reserved_type = uint8_t;
    using generated_type = std::tuple<name, uint32_t, name128>;
    using storage_type = static_variant<reserved_type, public_key_type, generated_type>;

public:
    address()
        : storage_(reserved_type()) {
        init_cache();
    }

    address(const public_key_type& pkey)
        : storage_(pkey) {
        init_cache();
    }

    address(name prefix, const name128& key, uint32_t nonce)
        : storage_(std::make_tuple(prefix, nonce, key)) {
        init_cache();
    }

    address(const address&) = default;
    address(address&&) noexcept = default;

    explicit address(const char* str) { *this = address::from_string(str); }
    explicit address(const string& str) { *this = address::from_string(str); }

public:
    int type() const { return storage_.which(); }

    bool is_reserved() const { return type() == reserved_t; }
    bool is_public_key() const { return type() == public_key_t; }
    bool is_generated() const { return type() == generated_t; }
    
public:
    const public_key_type&
    get_public_key() const {
        return storage_.get<public_key_type>();
    }

    name
    get_prefix() const {
        return std::get<prefix_idx>(storage_.get<generated_type>());
    }

    const name128&
    get_key() const {
        return std::get<key_idx>(storage_.get<generated_type>());
    }

    uint32_t
    get_nonce() const {
        return std::get<nonce_idx>(storage_.get<generated_type>());
    }

public:
    constexpr size_t get_bytes_size() const { return sizeof(fc::ecc::public_key_shim); }
    void to_bytes(char* buf, size_t sz) const;

    std::string to_string() const;

    explicit
    operator string() const {
        return to_string();
    }

    static address from_string(const std::string& str);

public:
    address&
    operator=(const address& addr) {
        if(this == &addr) {
            return *this;
        }

        storage_ = addr.storage_;
        cache_   = addr.cache_;
        return *this;
    }

    address&
    operator=(address&& addr) {
        if(this == &addr) {
            return *this;
        }

        storage_ = std::move(addr.storage_);
        cache_   = std::move(addr.cache_);
        return *this;
    }

    friend bool
    operator==(const address& a, const address& b) {
        return utilities::common::eq_comparator<storage_type>::apply(a.storage_, b.storage_);
    }

    friend bool
    operator!=(const address& a, const address& b) {
        return !(a == b);
    }

    friend bool
    operator<(const address& a, const address& b) {
        return utilities::common::less_comparator<storage_type>::apply(a.storage_, b.storage_);
    }

    friend bool
    operator==(const address& a, const public_key_type& b) {
        return a.type() == public_key_t && a.get_public_key() == b;
    }

    friend bool
    operator!=(const address& a, const public_key_type& b) {
        return !(a == b);
    }

    friend std::ostream&
    operator<< (std::ostream& s, const address& k) {
        s << k.to_string();
        return s;
    }

public:
    void
    reflector_init() const {
        init_cache();
    }

private:
    void init_cache() const;

private:
    storage_type storage_;
    mutable std::array<char, sizeof(fc::ecc::public_key_shim)> cache_;

private:
    friend struct fc::reflector<address>;
};

}}  // namespace jmzk::chain

namespace fc {

class variant;
void to_variant(const jmzk::chain::address& addr, fc::variant& v);
void from_variant(const fc::variant& v, jmzk::chain::address& addr);

template<>
inline void
to_variant<jmzk::chain::address>(const jmzk::chain::address& addr, fc::variant& v) {
    to_variant(addr, v);
}

template<>
inline void
from_variant<jmzk::chain::address>(const fc::variant& v, jmzk::chain::address& addr) {
    from_variant(v, addr);
}

}  // namespace fc

namespace fmt {

template <>
struct formatter<jmzk::chain::address> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const jmzk::chain::address& addr, FormatContext &ctx) {
        return format_to(ctx.begin(), addr.to_string());
    }
};

}  // namespace fmt

FC_REFLECT(jmzk::chain::address, (storage_))
