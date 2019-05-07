/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <ostream>
#include <optional>
#include <evt/chain/database_utils.hpp>
#include <evt/chain/exceptions.hpp>
#include <fc/variant_object.hpp>
#include <boost/core/demangle.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace evt { namespace chain {
/**
 * History:
 * Version 1: initial version with string identified sections and rows
 * Version 2: Token database upgrades to binary format
 * Version 3: Postgres upgrades to binary format and use Zstd stream
 */
static const uint32_t current_snapshot_version = 3;

namespace detail {
template <typename T>
struct snapshot_section_traits {
    static std::string
    section_name() {
        return boost::core::demangle(typeid(T).name());
    }
};

template <typename T>
struct snapshot_row_traits {
    using value_type    = std::decay_t<T>;
    using snapshot_type = value_type;

    static const snapshot_type&
    to_snapshot_row(const value_type& value, const chainbase::database&) {
        return value;
    };

    static const snapshot_type&
    to_snapshot_row(const value_type& value) {
        return value;
    };
};

/**
 * Due to a pattern in our code of overloading `operator << ( std::ostream&, ... )` to provide
 * human-readable string forms of data, we cannot directly use ostream as those operators will
 * be used instead of the expected operators.  In otherwords:
 * fc::raw::pack(fc::datastream...)
 * will end up calling _very_ different operators than
 * fc::raw::pack(std::ostream...)
 */
struct ostream_wrapper {
    explicit ostream_wrapper(std::ostream& s)
        : inner(s) {
    }

    ostream_wrapper(ostream_wrapper&&)      = default;
    ostream_wrapper(const ostream_wrapper&) = default;

    auto&
    write(const char* d, size_t s) {
        return inner.write(d, s);
    }

    auto&
    put(char c) {
        return inner.put(c);
    }

    auto
    tellp() const {
        return inner.tellp();
    }

    auto&
    seekp(std::ostream::pos_type p) {
        return inner.seekp(p);
    }

    std::ostream& inner;
};

struct abstract_snapshot_row_writer {
    virtual void        write(ostream_wrapper& out) const     = 0;
    virtual void        write(fc::sha256::encoder& out) const = 0;
    virtual variant     to_variant() const                    = 0;
    virtual std::string row_type_name() const                 = 0;
};

template <typename T>
struct snapshot_row_writer : abstract_snapshot_row_writer {
    explicit snapshot_row_writer(const T& data)
        : data(data) {}

    template <typename DataStream>
    void
    write_stream(DataStream& out) const {
        fc::raw::pack(out, data);
    }

    void
    write(ostream_wrapper& out) const override {
        write_stream(out);
    }

    void
    write(fc::sha256::encoder& out) const override {
        write_stream(out);
    }

    fc::variant
    to_variant() const override {
        variant var;
        fc::to_variant(data, var);
        return var;
    }

    std::string
    row_type_name() const override {
        return boost::core::demangle(typeid(T).name());
    }

    const T& data;
};

template <typename T>
snapshot_row_writer<T>
make_row_writer(const T& data) {
    return snapshot_row_writer<T>(data);
}

struct snapshot_row_raw_writer : abstract_snapshot_row_writer {
    explicit snapshot_row_raw_writer(const char* data, size_t sz)
        : data_(data), sz_(sz) {}

    template <typename DataStream>
    void
    write_stream(DataStream& out) const {
        out.write(data_, sz_);
    }

    void
    write(ostream_wrapper& out) const override {
        write_stream(out);
    }

    void
    write(fc::sha256::encoder& out) const override {
        write_stream(out);
    }

    fc::variant
    to_variant() const override {
        auto var = variant();
        var = std::string(data_, sz_);
        return var;
    }

    std::string
    row_type_name() const override {
        return "raw";
    }

    const char* data_;
    size_t      sz_;
};

}  // namespace detail

class snapshot_writer {
public:
    class section_writer {
    public:
        template <typename T>
        void
        add_row(const T& row, const chainbase::database& db) {
            _writer.write_row(detail::make_row_writer(detail::snapshot_row_traits<T>::to_snapshot_row(row, db)));
        }

        template <typename T>
        void
        add_row(const T& row) {
            _writer.write_row(detail::make_row_writer(detail::snapshot_row_traits<T>::to_snapshot_row(row)));
        }

        void
        add_row(const char* data, size_t sz) {
            _writer.write_row(detail::snapshot_row_raw_writer(data, sz));
        }

    private:
        friend class snapshot_writer;
        section_writer(snapshot_writer& writer)
            : _writer(writer) {
        }
        snapshot_writer& _writer;
    };

    template <typename F>
    void
    write_section(const std::string section_name, F f) {
        write_start_section(section_name);
        auto section = section_writer(*this);
        f(section);
        write_end_section();
    }

    template <typename T, typename F>
    void
    write_section(F f) {
        write_section(detail::snapshot_section_traits<T>::section_name(), f);
    }

    virtual ~snapshot_writer(){};

protected:
    virtual void write_start_section(const std::string& section_name)              = 0;
    virtual void write_row(const detail::abstract_snapshot_row_writer& row_writer) = 0;
    virtual void write_end_section()                                               = 0;
};

using snapshot_writer_ptr = std::shared_ptr<snapshot_writer>;

namespace detail {
struct abstract_snapshot_row_reader {
    virtual void        provide(std::istream& in) const   = 0;
    virtual void        provide(const fc::variant&) const = 0;
    virtual std::string row_type_name() const             = 0;
};

template <typename T>
struct is_chainbase_object {
    static constexpr bool value = false;
};

template <uint16_t TypeNumber, typename Derived>
struct is_chainbase_object<chainbase::object<TypeNumber, Derived>> {
    static constexpr bool value = true;
};

template <typename T>
constexpr bool is_chainbase_object_v = is_chainbase_object<T>::value;

struct row_validation_helper {
    template <typename T, typename F>
    static auto
    apply(const T& data, F f) -> std::enable_if_t<is_chainbase_object_v<T>> {
        auto orig = data.id;
        f();
        EVT_ASSERT(orig == data.id, snapshot_exception,
                   "Snapshot for ${type} mutates row member \"id\" which is illegal",
                   ("type", boost::core::demangle(typeid(T).name())));
    }

    template <typename T, typename F>
    static auto
    apply(const T&, F f) -> std::enable_if_t<!is_chainbase_object_v<T>> {
        f();
    }
};

template <typename T>
struct snapshot_row_reader : abstract_snapshot_row_reader {
    explicit snapshot_row_reader(T& data)
        : data(data) {}

    void
    provide(std::istream& in) const override {
        row_validation_helper::apply(data, [&in, this]() {
            fc::raw::unpack(in, data);
        });
    }

    void
    provide(const fc::variant& var) const override {
        row_validation_helper::apply(data, [&var, this]() {
            fc::from_variant(var, data);
        });
    }

    std::string
    row_type_name() const override {
        return boost::core::demangle(typeid(T).name());
    }

    T& data;
};

struct snapshot_row_raw_reader : abstract_snapshot_row_reader {
    snapshot_row_raw_reader(char* out, size_t sz)
        : out_(out), sz_(sz) {}

    void
    provide(std::istream& in) const override {
        FC_ASSERT(!in.eof());
        in.read(out_, sz_);
        FC_ASSERT(in.gcount() == std::streamsize(sz_));
    }

    void
    provide(const fc::variant& var) const override {
        auto s = var.as_string();
        FC_ASSERT(s.size() == sz_);

        memcpy(out_, s.c_str(), sz_);
    }

    std::string
    row_type_name() const override {
        return "raw";
    }

    char*  out_;
    size_t sz_;
};

template <typename T>
snapshot_row_reader<T>
make_row_reader(T& data) {
    return snapshot_row_reader<T>(data);
}

}  // namespace detail

class snapshot_reader {
public:
    class section_reader {
    public:
        template <typename T>
        auto
        read_row(T& out) -> std::enable_if_t<std::is_same<std::decay_t<T>, typename detail::snapshot_row_traits<T>::snapshot_type>::value, bool> {
            auto reader = detail::make_row_reader(out);
            return _reader.read_row(reader);
        }

        template <typename T>
        auto
        read_row(T& out, chainbase::database&) -> std::enable_if_t<std::is_same<std::decay_t<T>, typename detail::snapshot_row_traits<T>::snapshot_type>::value, bool> {
            return read_row(out);
        }

        template <typename T>
        auto
        read_row(T& out, chainbase::database& db) -> std::enable_if_t<!std::is_same<std::decay_t<T>, typename detail::snapshot_row_traits<T>::snapshot_type>::value, bool> {
            auto temp   = typename detail::snapshot_row_traits<T>::snapshot_type();
            auto reader = detail::make_row_reader(temp);
            bool result = _reader.read_row(reader);
            detail::snapshot_row_traits<T>::from_snapshot_row(std::move(temp), out, db);
            return result;
        }

        auto
        read_row(char* out, size_t sz) {
            auto reader = detail::snapshot_row_raw_reader(out, sz);
            return _reader.read_row(reader);
        }

        bool
        empty() {
            return _reader.empty();
        }

        bool
        eof() {
            return _reader.eof();
        }

    private:
        friend class snapshot_reader;
        section_reader(snapshot_reader& _reader)
            : _reader(_reader) {}

        snapshot_reader& _reader;
    };

public:
    template <typename F>
    void
    read_section(const std::string& section_name, F f) {
        set_section(section_name);
        auto section = section_reader(*this);
        f(section);
        clear_section();
    }

    template <typename T, typename F>
    void
    read_section(F f) {
        read_section(detail::snapshot_section_traits<T>::section_name(), f);
    }

    template <typename T>
    bool
    has_section(const std::string& suffix = std::string()) {
        return has_section(suffix + detail::snapshot_section_traits<T>::section_name());
    }

    virtual void validate() const = 0;
    virtual bool has_section(const std::string& section_name) = 0;
    virtual size_t get_section_size(const string& section_name) = 0;
    virtual std::vector<std::string> get_section_names(const std::string& prefix) const = 0;

    virtual ~snapshot_reader(){};

protected:
    virtual void set_section(const std::string& section_name)               = 0;
    virtual bool read_row(detail::abstract_snapshot_row_reader& row_reader) = 0;
    virtual bool empty()                                                    = 0;
    virtual bool eof()                                                      = 0;
    virtual void clear_section()                                            = 0;
    virtual void build_section_indexes()                                    = 0;
};

using snapshot_reader_ptr = std::shared_ptr<snapshot_reader>;

class variant_snapshot_writer : public snapshot_writer {
public:
    variant_snapshot_writer(fc::mutable_variant_object& snapshot);

    void write_start_section(const std::string& section_name) override;
    void write_row(const detail::abstract_snapshot_row_writer& row_writer) override;
    void write_end_section() override;
    void finalize();

private:
    fc::mutable_variant_object& snapshot;
    std::string                 current_section_name;
    fc::variants                current_rows;
};

class variant_snapshot_reader : public snapshot_reader {
private:
    struct section_index {
        std::string               name;
        const fc::variant_object& ptr;
    };

public:
    explicit variant_snapshot_reader(const fc::variant& snapshot);

    void validate() const override;
    std::vector<std::string> get_section_names(const std::string& prefix) const override;
    bool has_section(const string& section_name) override;
    void set_section(const string& section_name) override;
    size_t get_section_size(const string& section_name) override;
    bool read_row(detail::abstract_snapshot_row_reader& row_reader) override;
    bool empty() override;
    void clear_section() override;

private:
    void build_section_indexes() override;

private:
    const fc::variant&         snapshot;
    const fc::variant_object*  cur_section;
    uint64_t                   cur_row;
    std::vector<section_index> section_indexes;
};

class ostream_snapshot_writer : public snapshot_writer {
public:
    explicit ostream_snapshot_writer(std::ostream& snapshot);

    void write_start_section(const std::string& section_name) override;
    void write_row(const detail::abstract_snapshot_row_writer& row_writer) override;
    void write_end_section() override;
    void finalize();

    static const uint32_t magic_number = 0x30510550;

private:
    detail::ostream_wrapper                            snapshot;
    std::optional<boost::iostreams::filtering_ostream> row_stream;

    std::streampos          header_pos;
    std::streampos          section_pos;
    uint64_t                row_count;
};

class istream_snapshot_reader : public snapshot_reader {
public:
    struct section_index {
        std::string name;
        size_t      pos;
        size_t      row_count;
        size_t      size;
    };

public:
    explicit istream_snapshot_reader(std::istream& snapshot);

    void validate() const override;
    std::vector<std::string> get_section_names(const std::string& prefix) const override;
    bool has_section(const string& section_name) override;
    void set_section(const string& section_name) override;
    size_t get_section_size(const string& section_name) override;
    bool read_row(detail::abstract_snapshot_row_reader& row_reader) override;
    bool empty() override;
    bool eof() override;
    void clear_section() override;

private:
    void build_section_indexes() override;
    bool validate_section() const;

    std::istream&                                      snapshot;
    std::optional<boost::iostreams::filtering_istream> row_stream;

    std::streampos             header_pos;
    uint64_t                   num_rows;
    uint64_t                   cur_row;
    std::vector<section_index> section_indexes;
};

class integrity_hash_snapshot_writer : public snapshot_writer {
public:
    explicit integrity_hash_snapshot_writer(fc::sha256::encoder& enc);

    void write_start_section(const std::string& section_name) override;
    void write_row(const detail::abstract_snapshot_row_writer& row_writer) override;
    void write_end_section() override;
    void finalize();

private:
    fc::sha256::encoder& enc;
};

}}  // namespace evt::chain
