#pragma once
#include <fc/variant.hpp>
#include <fc/container/small_vector_fwd.hpp>

namespace fc {
class mutable_variant_object;

/**
    *  @ingroup Serializable
    *
    *  @brief An order-perserving dictionary of variant's.  
    *
    *  Keys are kept in the order they are inserted.
    *  This dictionary implements copy-on-write
    *
    *  @note This class is not optimized for random-access on large
    *        sets of key-value pairs.
    */
class variant_object {
public:
    /** @brief a key/value pair */
    class entry {
    public:
        entry();
        entry(string k, variant v);
        entry(entry&& e);
        entry(const entry& e);
        entry& operator=(const entry&);
        entry& operator=(entry&&);

        const string&  key() const;
        const variant& value() const;
        void           set(variant v);

        variant& value();

        friend bool
        operator==(const entry& a, const entry& b) {
            return a._key == b._key && a._value == b._value;
        }

        friend bool
        operator!=(const entry& a, const entry& b) {
            return !(a == b);
        }

    private:
        string  _key;
        variant _value;
    };

    using entry_vec      = small_vector<entry, 12>;
    using iterator       = entry_vec::iterator;
    using const_iterator = entry_vec::const_iterator;

    /**
         * @name Immutable Interface
         *
         * Calling these methods will not result in copies of the
         * underlying type.
         */
    ///@{
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator find(const string& key) const;
    const_iterator find(const char* key) const;
    const variant& operator[](const string& key) const;
    const variant& operator[](const char* key) const;
    size_t         size() const;
    
    bool contains(const char* key) const { return find(key) != end(); }
    ///@}

    variant_object() = default;

    variant_object(const variant_object& obj) = default;

    variant_object(variant_object&& obj) = default;

    variant_object(const mutable_variant_object& obj);

    variant_object(mutable_variant_object&& obj);

    template <typename T, typename = typename std::enable_if<!std::is_same_v<variant_object, std::decay_t<T>>>::type>
    explicit variant_object(T&& v) {
        fc::to_variant(fc::forward<T>(v), *this);
    }

    template <typename Key, typename T>
    variant_object(Key&& key, T&& val) {
        _key_value.emplace_back(std::forward<Key>(key), variant(std::forward<T>(val)));
    }

    variant_object& operator=(variant_object&&);
    variant_object& operator=(const variant_object&);

    variant_object& operator=(mutable_variant_object&&);
    variant_object& operator=(const mutable_variant_object&);

private:
    entry_vec _key_value;
    friend class mutable_variant_object;
};

/** @ingroup Serializable */
void to_variant(const variant_object& var, variant& vo);
/** @ingroup Serializable */
void from_variant(const variant& var, variant_object& vo);

/**
   *  @ingroup Serializable
   *
   *  @brief An order-perserving dictionary of variant's.  
   *
   *  Keys are kept in the order they are inserted.
   *  This dictionary implements copy-on-write
   *
   *  @note This class is not optimized for random-access on large
   *        sets of key-value pairs.
   */
class mutable_variant_object {
public:
    /** @brief a key/value pair */
    using entry          = variant_object::entry;
    using entry_vec      = small_vector<entry, 12>;
    using iterator       = entry_vec::iterator;
    using const_iterator = entry_vec::const_iterator;

    /**
         * @name Immutable Interface
         *
         * Calling these methods will not result in copies of the
         * underlying type.
         */
    ///@{
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator find(const string& key) const;
    const_iterator find(const char* key) const;
    const variant& operator[](const string& key) const;
    const variant& operator[](const char* key) const;
    size_t         size() const;
    ///@}
    variant& operator[](const string& key);
    variant& operator[](const char* key);

    /**
         * @name mutable Interface
         *
         * Calling these methods will result in a copy of the underlying type 
         * being created if there is more than one reference to this object.
         */
    ///@{
    void     reserve(size_t s);
    iterator begin();
    iterator end();
    void     erase(const string& key);
    /**
         *
         * @return end() if key is not found
         */
    iterator find(const string& key);
    iterator find(const char* key);

    /** replaces the value at \a key with \a var or insert's \a key if not found */
    mutable_variant_object& set(string key, variant var);

    /**
      *  Convenience method to simplify the manual construction of
      *  variant_object's
      *
      *  Instead of:
      *    <code>mutable_variant_object("c",c).set("a",a).set("b",b);</code>
      *
      *  You can use:
      *    <code>mutable_variant_object( "c", c )( "b", b)( "c",c )</code>
      *
      *  @return *this;
      */
    mutable_variant_object& operator()(string key, variant var);

    template <typename T>
    mutable_variant_object&
    operator()(string key, T&& var) {
        set(std::move(key), variant(fc::forward<T>(var)));
        return *this;
    }
    /**
       * Copy a variant_object into this mutable_variant_object.
       */
    mutable_variant_object& operator()(const variant_object& vo);
    /**
       * Copy another mutable_variant_object into this mutable_variant_object.
       */
    mutable_variant_object& operator()(const mutable_variant_object& mvo);
    ///@}

    mutable_variant_object() = default;

    mutable_variant_object(const mutable_variant_object& obj) = default;

    mutable_variant_object(mutable_variant_object&& obj) = default;

    mutable_variant_object(const variant_object& obj)
        : _key_value(obj._key_value) {
    }

    template <typename T, typename = typename std::enable_if<!std::is_same_v<mutable_variant_object, std::decay_t<T>>>::type>
    explicit mutable_variant_object(T&& v) {
        *this = variant(fc::forward<T>(v)).get_object();
    }

    template <typename Key, typename T>
    mutable_variant_object(Key&& key, T&& val) {
        _key_value.emplace_back(std::forward<Key>(key), variant(std::forward<T>(val)));
    }

    mutable_variant_object& operator=(mutable_variant_object&&);
    mutable_variant_object& operator=(const mutable_variant_object&);
    mutable_variant_object& operator=(const variant_object&);

private:
    entry_vec _key_value;
    friend class variant_object;
};

/** @ingroup Serializable */
void to_variant(const mutable_variant_object& var, variant& vo);
/** @ingroup Serializable */
void from_variant(const variant& var, mutable_variant_object& vo);

}  // namespace fc
