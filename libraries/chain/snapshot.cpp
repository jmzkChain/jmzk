#include <evt/chain/snapshot.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/filter/zstd.hpp>

#include <fc/scoped_exit.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

variant_snapshot_writer::variant_snapshot_writer(fc::mutable_variant_object& snapshot)
    : snapshot(snapshot) {
    snapshot.set("sections", fc::variants());
    snapshot.set("version", current_snapshot_version);
}

void
variant_snapshot_writer::write_start_section(const std::string& section_name) {
    current_rows.clear();
    current_section_name = section_name;
}

void
variant_snapshot_writer::write_row(const detail::abstract_snapshot_row_writer& row_writer) {
    current_rows.emplace_back(row_writer.to_variant());
}

void
variant_snapshot_writer::write_end_section() {
    snapshot["sections"].get_array().emplace_back(fc::mutable_variant_object()("name", std::move(current_section_name))("rows", std::move(current_rows)));
}

void
variant_snapshot_writer::finalize() {
}

variant_snapshot_reader::variant_snapshot_reader(const fc::variant& snapshot)
    : snapshot(snapshot)
    , cur_row(0) {
    build_section_indexes();
}

void
variant_snapshot_reader::validate() const {
    EVT_ASSERT(snapshot.is_object(), snapshot_validation_exception,
               "Variant snapshot is not an object");
    const fc::variant_object& o = snapshot.get_object();

    EVT_ASSERT(o.contains("version"), snapshot_validation_exception,
               "Variant snapshot has no version");

    const auto& version = o["version"];
    EVT_ASSERT(version.is_integer(), snapshot_validation_exception,
               "Variant snapshot version is not an integer");

    EVT_ASSERT(version.as_uint64() == (uint64_t)current_snapshot_version, snapshot_validation_exception,
               "Variant snapshot is an unsuppored version.  Expected : ${expected}, Got: ${actual}",
               ("expected", current_snapshot_version)("actual", o["version"].as_uint64()));

    EVT_ASSERT(o.contains("sections"), snapshot_validation_exception,
               "Variant snapshot has no sections");

    const auto& sections = o["sections"];
    EVT_ASSERT(sections.is_array(), snapshot_validation_exception, "Variant snapshot sections is not an array");

    const auto& section_array = sections.get_array();
    for(const auto& section : section_array) {
        EVT_ASSERT(section.is_object(), snapshot_validation_exception, "Variant snapshot section is not an object");

        const auto& so = section.get_object();
        EVT_ASSERT(so.contains("name"), snapshot_validation_exception,
                   "Variant snapshot section has no name");

        EVT_ASSERT(so["name"].is_string(), snapshot_validation_exception,
                   "Variant snapshot section name is not a string");

        EVT_ASSERT(so.contains("rows"), snapshot_validation_exception,
                   "Variant snapshot section has no rows");

        EVT_ASSERT(so["rows"].is_array(), snapshot_validation_exception,
                   "Variant snapshot section rows is not an array");
    }
}

std::vector<std::string>
variant_snapshot_reader::get_section_names(const std::string& prefix) const {
    auto names = std::vector<std::string>();
    for(auto& si : section_indexes) {
        if(boost::starts_with(si.name, prefix)) {
            names.emplace_back(si.name);
        }
    }
    return names;
}

bool
variant_snapshot_reader::has_section(const string& section_name) {
    return std::find_if(section_indexes.cbegin(), section_indexes.cend(), [&](auto& si) {
        return si.name == section_name;
    }) != section_indexes.cend();
}

void
variant_snapshot_reader::set_section(const string& section_name) {
    for(auto& si : section_indexes) {
        if(si.name == section_name) {
            cur_section = &si.ptr;
            return;
        }
    }

    EVT_THROW(snapshot_exception, "Variant snapshot has no section named ${n}", ("n", section_name));
}

size_t
variant_snapshot_reader::get_section_size(const string& section_name) {
    EVT_THROW(snapshot_exception, "Variant snapshot doesn't support this feature");
}

bool
variant_snapshot_reader::read_row(detail::abstract_snapshot_row_reader& row_reader) {
    const auto& rows = (*cur_section)["rows"].get_array();
    row_reader.provide(rows.at(cur_row++));
    return cur_row < rows.size();
}

bool
variant_snapshot_reader::empty() {
    const auto& rows = (*cur_section)["rows"].get_array();
    return rows.empty();
}

void
variant_snapshot_reader::clear_section() {
    cur_section = nullptr;
    cur_row     = 0;
}

void
variant_snapshot_reader::build_section_indexes() {
    const auto& sections = snapshot["sections"].get_array();
    for(auto& section : sections) {
        section_indexes.emplace_back(section_index {
            .name = section["name"].as_string(),
            .ptr  = section.get_object()
        });
    }
}

ostream_snapshot_writer::ostream_snapshot_writer(std::ostream& snapshot)
    : snapshot(snapshot)
    , header_pos(snapshot.tellp())
    , section_pos(-1)
    , row_count(0) {
    // write magic number
    auto totem = magic_number;
    snapshot.write((char*)&totem, sizeof(totem));

    // write version
    auto version = current_snapshot_version;
    snapshot.write((char*)&version, sizeof(version));
}

void
ostream_snapshot_writer::write_start_section(const std::string& section_name) {
    namespace io = boost::iostreams;

    EVT_ASSERT(section_pos == std::streampos(-1), snapshot_exception, "Attempting to write a new section without closing the previous section");
    section_pos = snapshot.tellp();
    row_count   = 0;

    uint64_t placeholder = std::numeric_limits<uint64_t>::max();

    // write a placeholder for the section size
    snapshot.write((char*)&placeholder, sizeof(placeholder));

    // write placeholder for row count
    snapshot.write((char*)&placeholder, sizeof(placeholder));

    // write the section name (null terminated)
    snapshot.write(section_name.data(), section_name.size());
    snapshot.put('\0');

    // setup row stream
    assert(!row_stream.has_value());
    row_stream.emplace();
    row_stream->push(io::zstd_compressor());
    row_stream->push(snapshot.inner);
}

void
ostream_snapshot_writer::write_row(const detail::abstract_snapshot_row_writer& row_writer) {
    try {
        auto wrapper = detail::ostream_wrapper(*row_stream);
        row_writer.write(wrapper);
    }
    catch(...) {
        throw;
    }
    row_count++;
}

void
ostream_snapshot_writer::write_end_section() {
    namespace io = boost::iostreams;

    // flush row stream
    assert(row_stream.has_value());
    io::flush(*row_stream);
    io::close(*row_stream);
    row_stream.reset();

    // seek to section pos
    auto restore = snapshot.tellp();
    uint64_t section_size = restore - section_pos - sizeof(uint64_t);
    snapshot.seekp(section_pos);

    // write a the section size
    snapshot.write((char*)&section_size, sizeof(section_size));

    // write the row count
    snapshot.write((char*)&row_count, sizeof(row_count));
    snapshot.seekp(restore);

    // clear state
    section_pos = std::streampos(-1);
    row_count   = 0;
}

void
ostream_snapshot_writer::finalize() {
    uint64_t end_marker = std::numeric_limits<uint64_t>::max();

    // write a placeholder for the section size
    snapshot.write((char*)&end_marker, sizeof(end_marker));
}

istream_snapshot_reader::istream_snapshot_reader(std::istream& snapshot)
    : snapshot(snapshot)
    , header_pos(snapshot.tellg())
    , num_rows(0)
    , cur_row(0) {
    build_section_indexes();
}

void
istream_snapshot_reader::validate() const {
    // make sure to restore the read pos
    auto restore_pos = fc::make_scoped_exit([this, pos = snapshot.tellg(), ex = snapshot.exceptions()]() {
        snapshot.seekg(pos);
        snapshot.exceptions(ex);
    });

    snapshot.exceptions(std::istream::failbit | std::istream::eofbit);

    try {
        // validate totem
        auto                     expected_totem = ostream_snapshot_writer::magic_number;
        decltype(expected_totem) actual_totem;
        snapshot.read((char*)&actual_totem, sizeof(actual_totem));
        EVT_ASSERT(actual_totem == expected_totem, snapshot_exception,
                   "Binary snapshot has unexpected magic number!");

        // validate version
        auto                       expected_version = current_snapshot_version;
        decltype(expected_version) actual_version;
        snapshot.read((char*)&actual_version, sizeof(actual_version));
        EVT_ASSERT(actual_version == expected_version, snapshot_exception,
                   "Binary snapshot is an unsuppored version.  Expected : ${expected}, Got: ${actual}",
                   ("expected", expected_version)("actual", actual_version));

        while(validate_section()) {
        }
    }
    catch(const std::exception& e) {
        snapshot_exception fce(FC_LOG_MESSAGE(warn, "Binary snapshot validation threw IO exception (${what})", ("what", e.what())));
        throw fce;
    }
}

bool
istream_snapshot_reader::validate_section() const {
    uint64_t section_size = 0;
    snapshot.read((char*)&section_size, sizeof(section_size));

    // stop when we see the end marker
    if(section_size == std::numeric_limits<uint64_t>::max()) {
        return false;
    }

    // seek past the section
    snapshot.seekg(snapshot.tellg() + std::streamoff(section_size));

    return true;
}

std::vector<std::string>
istream_snapshot_reader::get_section_names(const std::string& prefix) const {
    auto names = std::vector<std::string>();
    for(auto& si : section_indexes) {
        if(boost::starts_with(si.name, prefix)) {
            names.emplace_back(si.name);
        }
    }
    return names;
}

bool
istream_snapshot_reader::has_section(const string& section_name) {
    return std::find_if(section_indexes.cbegin(), section_indexes.cend(), [&](auto& si) {
        return si.name == section_name;
    }) != section_indexes.cend();
}

void
istream_snapshot_reader::set_section(const string& section_name) {
    namespace io = boost::iostreams;

    if(row_stream.has_value()) {
        io::close(*row_stream);
        row_stream.reset();
    }

    for(auto& si : section_indexes) {
        if(si.name == section_name) {
            snapshot.seekg(si.pos);
            cur_row  = 0;
            num_rows = si.row_count;

            // setup row stream
            assert(!row_stream.has_value());
            row_stream.emplace();
            row_stream->push(io::zstd_decompressor());
            row_stream->push(snapshot);

            return;
        }
    }

    EVT_THROW(snapshot_exception, "Binary snapshot has no section named ${n}", ("n", section_name));
}

size_t
istream_snapshot_reader::get_section_size(const string& section_name) {
    for(auto& si : section_indexes) {
        if(si.name == section_name) {
            return si.size;
        }
    }

    EVT_THROW(snapshot_exception, "Binary snapshot has no section named ${n}", ("n", section_name));
}

bool
istream_snapshot_reader::read_row(detail::abstract_snapshot_row_reader& row_reader) {
    row_reader.provide(*row_stream);
    return ++cur_row < num_rows;
}

bool
istream_snapshot_reader::empty() {
    return num_rows == 0;
}

bool
istream_snapshot_reader::eof() {
    return cur_row >= num_rows;
}

void
istream_snapshot_reader::clear_section() {
    num_rows = 0;
    cur_row  = 0;
}

void
istream_snapshot_reader::build_section_indexes() {
    auto restore_pos = fc::make_scoped_exit([this, pos = snapshot.tellg()]() {
        snapshot.seekg(pos);
    });

    const std::streamoff header_size = sizeof(ostream_snapshot_writer::magic_number) + sizeof(current_snapshot_version);

    auto next_section_pos = header_pos + header_size;

    while(true) {
        snapshot.seekg(next_section_pos);
        uint64_t section_size = 0;
        snapshot.read((char*)&section_size, sizeof(section_size));
        if(section_size == std::numeric_limits<uint64_t>::max()) {
            break;
        }
        if(snapshot.eof()) {
            snapshot.clear();
            snapshot.seekg(0);
            break;
        }

        next_section_pos = snapshot.tellg() + std::streamoff(section_size);

        uint64_t row_count = 0;
        snapshot.read((char*)&row_count, sizeof(row_count));

        auto name = std::string();
        char c;
        while((c = snapshot.get()) != '\0') {
            EVT_ASSERT(c != std::char_traits<char>::eof(), snapshot_validation_exception, "Not valid section name");
            name.push_back(c);
        }

        section_indexes.emplace_back(section_index {
            .name      = name,
            .pos       = (size_t)snapshot.tellg(),
            .row_count = row_count,
            .size      = (size_t)(next_section_pos - snapshot.tellg())
        });
    }
}

integrity_hash_snapshot_writer::integrity_hash_snapshot_writer(fc::sha256::encoder& enc)
    : enc(enc) {
}

void
integrity_hash_snapshot_writer::write_start_section(const std::string&) {
    // no-op for structural details
}

void
integrity_hash_snapshot_writer::write_row(const detail::abstract_snapshot_row_writer& row_writer) {
    row_writer.write(enc);
}

void
integrity_hash_snapshot_writer::write_end_section() {
    // no-op for structural details
}

void
integrity_hash_snapshot_writer::finalize() {
    // no-op for structural details
}

}}  // namespace evt::chain