/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/abi_serializer.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/varint.hpp>

#include <evt/chain/chain_config.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/execution_context.hpp>

using namespace boost;

namespace evt { namespace chain { namespace contracts {

const size_t abi_serializer::max_recursion_depth;

using boost::algorithm::ends_with;
using std::string;

template <typename T>
inline fc::variant
variant_from_stream(fc::datastream<const char*>& stream) {
    T temp;
    fc::raw::unpack(stream, temp);
    return fc::variant(temp);
}

template <typename T>
auto
pack_unpack() {
    return std::make_pair<abi_serializer::unpack_function, abi_serializer::pack_function>(
        [](fc::datastream<const char*>& stream, bool is_array, bool is_optional) -> fc::variant {
            if(is_array) {
                return variant_from_stream<fc::small_vector<T, 4>>(stream);
            }
            else if(is_optional) {
                return variant_from_stream<optional<T>>(stream);
            }
            return variant_from_stream<T>(stream);
        },
        [](const fc::variant& var, fc::datastream<char*>& ds, bool is_array, bool is_optional) {
            if(is_array) {
                fc::raw::pack(ds, var.as<fc::small_vector<T, 4>>());
            }
            else if(is_optional) {
                fc::raw::pack(ds, var.as<optional<T>>());
            }
            else {
                fc::raw::pack(ds, var.as<T>());
            }
        });
}

abi_serializer::abi_serializer(const abi_def& abi, const std::chrono::microseconds max_serialization_time_)
    : max_serialization_time_(max_serialization_time_) {
    configure_built_in_types();
    set_abi(abi);
}

void
abi_serializer::add_specialized_unpack_pack(const string& name,
                                            std::pair<abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack) {
    built_in_types_[name] = std::move(unpack_pack);
}

void
abi_serializer::configure_built_in_types() {
    built_in_types_.emplace("public_key", pack_unpack<public_key_type>());
    built_in_types_.emplace("signature", pack_unpack<signature_type>());

    built_in_types_.emplace("address", pack_unpack<address>());
    built_in_types_.emplace("symbol", pack_unpack<symbol>());
    built_in_types_.emplace("asset", pack_unpack<asset>());

    built_in_types_.emplace("bytes", pack_unpack<bytes>());
    built_in_types_.emplace("string", pack_unpack<string>());
    
    built_in_types_.emplace("time_point", pack_unpack<fc::time_point>());
    built_in_types_.emplace("time_point_sec", pack_unpack<fc::time_point_sec>());
    built_in_types_.emplace("block_timestamp_type", pack_unpack<block_timestamp_type>());
    
    built_in_types_.emplace("checksum160", pack_unpack<checksum160_type>());
    built_in_types_.emplace("checksum256", pack_unpack<checksum256_type>());
    built_in_types_.emplace("checksum512", pack_unpack<checksum512_type>());
    
    built_in_types_.emplace("bool", pack_unpack<uint8_t>());
    built_in_types_.emplace("int8", pack_unpack<int8_t>());
    built_in_types_.emplace("uint8", pack_unpack<uint8_t>());
    built_in_types_.emplace("int16", pack_unpack<int16_t>());
    built_in_types_.emplace("uint16", pack_unpack<uint16_t>());
    built_in_types_.emplace("int32", pack_unpack<int32_t>());
    built_in_types_.emplace("uint32", pack_unpack<uint32_t>());
    built_in_types_.emplace("int64", pack_unpack<int64_t>());
    built_in_types_.emplace("uint64", pack_unpack<uint64_t>());
    built_in_types_.emplace("uint128", pack_unpack<uint128_t>());

    built_in_types_.emplace("float32", pack_unpack<float>());
    built_in_types_.emplace("float64", pack_unpack<double>());
    built_in_types_.emplace("float128", pack_unpack<uint128_t>());

    built_in_types_.emplace("name", pack_unpack<name>());
    built_in_types_.emplace("name128", pack_unpack<name128>());
    built_in_types_.emplace("group", pack_unpack<group>());
    built_in_types_.emplace("authorizer_ref", pack_unpack<authorizer_ref>());
    built_in_types_.emplace("producer_schedule", pack_unpack<producer_schedule_type>());
    built_in_types_.emplace("extensions", pack_unpack<extensions_type>());
    built_in_types_.emplace("evt_link", pack_unpack<evt_link>());
    built_in_types_.emplace("percent", pack_unpack<percent_type>());
}

void
abi_serializer::set_abi(const abi_def& abi) {
    typedefs_.clear();
    structs_.clear();
    variants_.clear();
    enums_.clear();

    for(auto& st : abi.structs) {
        structs_[st.name] = st;
    }

    for(auto& vt : abi.variants) {
        variants_[vt.name] = vt;
    }

    for(auto& et : abi.enums) {
        enums_[et.name] = et;
    }

    for(const auto& td : abi.types) {
        EVT_ASSERT(_is_type(td.type), invalid_type_inside_abi, "invalid type ${type}", ("type", td.type));
        EVT_ASSERT(!_is_type(td.new_type_name), duplicate_abi_type_def_exception, "type already exists", ("new_type_name", td.new_type_name));
        typedefs_[td.new_type_name] = td.type;
    }

    EVT_ASSERT(typedefs_.size() == abi.types.size(), duplicate_abi_type_def_exception, "duplicate type definition detected");
    EVT_ASSERT(structs_.size() == abi.structs.size(), duplicate_abi_struct_def_exception, "duplicate struct definition detected");
    EVT_ASSERT(variants_.size() == abi.variants.size(), duplicate_abi_variant_def_exception, "duplicate variant definition detected");
    EVT_ASSERT(enums_.size() == abi.enums.size(), duplicate_abi_enum_def_exception, "duplicate enum definition detected");

    validate();
}

bool
abi_serializer::is_builtin_type(const type_name& type) const {
    return built_in_types_.find(type) != built_in_types_.end();
}

bool
abi_serializer::is_integer(const type_name& type) const {
    auto stype = type;
    return boost::starts_with(stype, "uint") || boost::starts_with(stype, "int");
}

int
abi_serializer::get_integer_size(const type_name& type) const {
    auto stype = type;
    EVT_ASSERT(is_integer(type), invalid_type_inside_abi, "${stype} is not an integer type", ("stype", stype));
    if(boost::starts_with(stype, "uint")) {
        return boost::lexical_cast<int>(stype.substr(4));
    }
    else {
        return boost::lexical_cast<int>(stype.substr(3));
    }
}

bool
abi_serializer::is_struct(const type_name& type) const {
    return structs_.find(resolve_type(type)) != structs_.cend();
}

bool
abi_serializer::is_variant(const type_name& type) const {
    return variants_.find(resolve_type(type)) != variants_.cend();   
}

bool
abi_serializer::is_enum(const type_name& type) const {
    return enums_.find(resolve_type(type)) != enums_.cend();   
}

bool
abi_serializer::is_array(const type_name& type) const {
    return ends_with(string(type), "[]");
}

bool
abi_serializer::is_optional(const type_name& type) const {
    return ends_with(string(type), "?");
}

bool
abi_serializer::is_type(const type_name& type) const {
    return _is_type(type);
}

type_name
abi_serializer::fundamental_type(const type_name& type) const {
    if(is_array(type)) {
        return type_name(string(type).substr(0, type.size() - 2));
    }
    else if(is_optional(type)) {
        return type_name(string(type).substr(0, type.size() - 1));
    }
    else {
        return type;
    }
}

bool
abi_serializer::_is_type(const type_name& rtype) const {
    auto type = fundamental_type(rtype);
    if(built_in_types_.find(type) != built_in_types_.cend()) {
        return true;
    }
    if(typedefs_.find(type) != typedefs_.cend()) {
        return _is_type(typedefs_.find(type)->second);
    }
    if(structs_.find(type) != structs_.cend()) {
        return true;
    }
    if(variants_.find(type) != variants_.cend()) {
        return true;
    }
    if(enums_.find(type) != enums_.cend()) {
        return true;
    }
    return false;
}

const struct_def&
abi_serializer::get_struct(const type_name& type) const {
    auto itr = structs_.find(resolve_type(type));
    EVT_ASSERT(itr != structs_.end(), invalid_type_inside_abi, "Unknown struct ${type}", ("type", type));
    return itr->second;
}

void
abi_serializer::validate() const {
    for(const auto& t : typedefs_) {
        try {
            auto types_seen = vector<type_name>{t.first, t.second};
            auto itr        = typedefs_.find(t.second);
            while(itr != typedefs_.end()) {
                EVT_ASSERT(find(types_seen.begin(), types_seen.end(), itr->second) == types_seen.end(), abi_circular_def_exception, "Circular reference in type ${type}", ("type", t.first));
                types_seen.emplace_back(itr->second);
                itr = typedefs_.find(itr->second);
            }
        }
        FC_CAPTURE_AND_RETHROW((t))
    }
    for(const auto& t : typedefs_) {
        try {
            EVT_ASSERT(_is_type(t.second), invalid_type_inside_abi, "${type}", ("type", t.second));
        }
        FC_CAPTURE_AND_RETHROW((t))
    }
    for(const auto& s : structs_) {
        try {
            if(s.second.base != type_name()) {
                auto current    = s.second;
                auto types_seen = vector<type_name>{current.name};
                while(current.base != type_name()) {
                    const auto& base = get_struct(current.base);  //<-- force struct to inherit from another struct
                    EVT_ASSERT(find(types_seen.begin(), types_seen.end(), base.name) == types_seen.end(), abi_circular_def_exception, "Circular reference in struct ${type}", ("type", s.second.name));
                    types_seen.emplace_back(base.name);
                    current = base;
                }
            }
            for(const auto& field : s.second.fields) {
                try {
                    EVT_ASSERT(_is_type(field.type), invalid_type_inside_abi, "${type}", ("type", field.type));
                }
                FC_CAPTURE_AND_RETHROW((field))
            }
        }
        FC_CAPTURE_AND_RETHROW((s.second))
    }
    for(const auto& v : variants_) {
        for(const auto& field : v.second.fields) {
            try {
                EVT_ASSERT(_is_type(field.type), invalid_type_inside_abi, "${type}", ("type", field.type));
            }
            FC_CAPTURE_AND_RETHROW((field))
        }
    }
    for(const auto& et : enums_) {
        try {
            EVT_ASSERT(_is_type(et.second.integer), invalid_type_inside_abi, "${type}", ("type", et.second.integer));
        }
        FC_CAPTURE_AND_RETHROW((et.second))
    }
}

type_name
abi_serializer::resolve_type(const type_name& type) const {
    auto itr = typedefs_.find(type);
    if(itr != typedefs_.end()) {
        for(auto i = typedefs_.size(); i > 0; --i) {  // avoid infinite recursion
            auto& t = itr->second;
            itr     = typedefs_.find(t);
            if(itr == typedefs_.end()) {
                return t;
            }
        }
    }
    return type;
}

void
abi_serializer::_binary_to_variant(const type_name& type, fc::datastream<const char*>& stream,
                                   fc::mutable_variant_object& obj, impl::binary_to_variant_context& ctx) const {
    auto h     = ctx.enter_scope();
    auto s_itr = structs_.find(type);
    EVT_ASSERT(s_itr != structs_.end(), invalid_type_inside_abi, "Unknown type ${type}", ("type", ctx.maybe_shorten(type)));

    ctx.hint_struct_type_if_in_array(s_itr);
    const auto& st = s_itr->second;
    if(st.base != type_name()) {
        _binary_to_variant(resolve_type(st.base), stream, obj, ctx);
    }

    for(auto i = 0u; i < st.fields.size(); ++i) {
        const auto& field = st.fields[i];
        if(!stream.remaining()) {
            EVT_THROW(unpack_exception, "Stream unexpectedly ended; unable to unpack field '${f}' of struct '${p}'",
                      ("f", ctx.maybe_shorten(field.name))("p", ctx.get_path_string()));
        }
        auto h1 = ctx.push_to_path(impl::field_path_item{.parent_itr = s_itr, .field_ordinal = i});
        obj(field.name, _binary_to_variant(resolve_type(field.type), stream, ctx));
    }
}

fc::variant
abi_serializer::_binary_to_variant(const type_name& type, fc::datastream<const char*>& stream,
                                   impl::binary_to_variant_context& ctx) const {
    auto h     = ctx.enter_scope();
    auto rtype = resolve_type(type);
    auto ftype = fundamental_type(rtype);
    auto btype = built_in_types_.find(ftype);

    if(btype != built_in_types_.end()) {
        try {
            return btype->second.first(stream, is_array(rtype), is_optional(rtype));
        }
        EVT_RETHROW_EXCEPTIONS(unpack_exception, "Unable to unpack ${class} type '${type}' while processing '${p}'",
                               ("class", is_array(rtype) ? "array of built-in" : is_optional(rtype) ? "optional of built-in" : "built-in")("type", ftype)("p", ctx.get_path_string()))
    }

    if(is_array(rtype)) {
        ctx.hint_array_type_if_in_array();

        auto size = fc::unsigned_int();
        try {
            fc::raw::unpack(stream, size);
        }
        EVT_RETHROW_EXCEPTIONS(unpack_exception, "Unable to unpack size of array '${p}'", ("p", ctx.get_path_string()))

        auto vars = fc::small_vector<fc::variant, 4>();
        auto h1   = ctx.push_to_path(impl::array_index_path_item{});
        for(decltype(size.value) i = 0; i < size; ++i) {
            ctx.set_array_index_of_path_back(i);
            auto v = _binary_to_variant(ftype, stream, ctx);
            // QUESTION: Is it actually desired behavior to require the returned variant to not be null?
            //           This would disallow arrays of optionals in general (though if all optionals in the array were present it would be allowed).
            //           Is there any scenario in which the returned variant would be null other than in the case of an empty optional?
            EVT_ASSERT(!v.is_null(), unpack_exception, "Invalid packed array '${p}'", ("p", ctx.get_path_string()));
            vars.emplace_back(std::move(v));
        }
        // QUESTION: Why would the assert below ever fail?
        EVT_ASSERT(vars.size() == size.value, unpack_exception,
                   "packed size does not match unpacked array size, packed size ${p} actual size ${a}", ("p", size)("a", vars.size()));
        
        return fc::variant(std::move(vars));
    }
    else if(is_optional(rtype)) {
        char flag;
        try {
            fc::raw::unpack(stream, flag);
        }
        EVT_RETHROW_EXCEPTIONS(unpack_exception, "Unable to unpack presence flag of optional '${p}'", ("p", ctx.get_path_string()))
        return flag ? _binary_to_variant(ftype, stream, ctx) : fc::variant();
    }
    else if(is_variant(rtype)) {
        auto v_itr = variants_.find(rtype);
        ctx.hint_variant_type_if_in_array(v_itr);

        auto i = fc::unsigned_int();
        try {
            fc::raw::unpack(stream, i);
        }
        EVT_RETHROW_EXCEPTIONS(unpack_exception, "Unable to unpack index of variant '${p}'", ("p", ctx.get_path_string()));

        auto& vt = v_itr->second;
        EVT_ASSERT2((uint32_t)i < vt.fields.size(), unpack_exception, "Index of variant '{}' if not valid", ctx.get_path_string());

        auto vo = mutable_variant_object();
        auto h1 = ctx.push_to_path(impl::variant_path_item{.parent_itr = v_itr, .index = i});

        vo["type"] = vt.fields[i].name;
        vo["data"] = _binary_to_variant(vt.fields[i].type, stream, ctx);

        return fc::variant(std::move(vo));
    }
    else if(is_enum(rtype)) {
        auto e_itr = enums_.find(rtype);
        ctx.hint_enum_type_if_in_array(e_itr);

        auto& et = e_itr->second;
        auto  ev = _binary_to_variant(et.integer, stream, ctx);
        // we assume the enum is start at 0 and each item is increased by 1
        EVT_ASSERT2(ev.as_uint64() < et.fields.size(), unpack_exception, "Value of enum '{}' is not valid", ctx.get_path_string());

        return fc::variant(et.fields[ev.as_uint64()]);
    }

    auto mvo = fc::mutable_variant_object();
    _binary_to_variant(rtype, stream, mvo, ctx);
    
    return fc::variant(std::move(mvo));
}

fc::variant
abi_serializer::_binary_to_variant(const type_name& type, const bytes& binary, impl::binary_to_variant_context& ctx) const {
    auto h   = ctx.enter_scope();
    auto ds  = fc::datastream(binary.data(), binary.size());
    auto var = _binary_to_variant(type, ds, ctx);
    if(ds.remaining() > 0) {
        EVT_THROW2(unpack_exception, "Binary buffer is not EOF after unpack variable, remaining: {} bytes.", ds.remaining());
    }
    return var;
}

fc::variant
abi_serializer::binary_to_variant(const type_name& type, const bytes& binary, const execution_context& exec_ctx, bool short_path) const {
    auto ctx = impl::binary_to_variant_context(*this, exec_ctx, type);
    ctx.short_path = short_path;
    return _binary_to_variant(type, binary, ctx);
}

fc::variant
abi_serializer::binary_to_variant(const type_name& type, fc::datastream<const char*>& binary, const execution_context& exec_ctx, bool short_path) const {
    auto ctx = impl::binary_to_variant_context(*this, exec_ctx, type);
    ctx.short_path = short_path;
    return _binary_to_variant(type, binary, ctx);
}

void
abi_serializer::_variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char*>& ds, impl::variant_to_binary_context& ctx) const {
    try {
        auto h     = ctx.enter_scope();
        auto rtype = resolve_type(type);

        auto btype = built_in_types_.find(fundamental_type(rtype));
        if(btype != built_in_types_.end()) {
            btype->second.second(var, ds, is_array(rtype), is_optional(rtype));
        }
        else if(is_array(rtype)) {
            ctx.hint_array_type_if_in_array();
            auto& vars = var.get_array();
            fc::raw::pack(ds, (fc::unsigned_int)vars.size());

            auto h1 = ctx.push_to_path(impl::array_index_path_item{});

            int64_t i = 0;
            for(const auto& var : vars) {
                ctx.set_array_index_of_path_back(i);
                _variant_to_binary(fundamental_type(rtype), var, ds, ctx);
                ++i;
            }
        }
        else if(is_optional(rtype)) {
            char flag = 1;
            if(var.is_null()) {
                flag = 0;
            }
            fc::raw::pack(ds, flag);
            if(flag) {
                _variant_to_binary(fundamental_type(rtype), var, ds, ctx);
            }
        }
        else if(is_variant(rtype)) {
            auto v_itr = variants_.find(rtype);
            ctx.hint_variant_type_if_in_array(v_itr);

            auto& vt = v_itr->second; 
            auto& vo = var.get_object();

            auto check_field = [&](auto& vo, auto name, auto type) {
                EVT_ASSERT2(vo.contains(name), pack_exception,
                    "Missing field '{}' in input object while processing variant '{}'", name, ctx.get_path_string());
                if(type == "string") {
                    EVT_ASSERT2(vo[name].is_string(), pack_exception,
                        "Invalid field '{}' in input object while processing variant '{}', it must be string type", name, ctx.get_path_string()); 
                }
                else if(type == "object") {
                    EVT_ASSERT2(vo[name].is_object(), pack_exception,
                        "Invalid field '{}' in input object while processing variant '{}', it must be object type", name, ctx.get_path_string()); 
                }
            };
            check_field(vo, "type", "string");
            check_field(vo, "data", "object");

            auto dtype = vo["type"].get_string();
            auto index = 0u;

            for(auto& field : vt.fields) {
                if(field.name == dtype) {
                    break;
                }
                index++;
            }
            EVT_ASSERT2(index < vt.fields.size(), pack_exception, "Invalid 'type' value of variant '{}'", ctx.get_path_string());

            fc::raw::pack(ds, (fc::unsigned_int)index);

            auto h1 = ctx.push_to_path(impl::variant_path_item{.parent_itr = v_itr, .index = index});
            _variant_to_binary(vt.fields[index].type, vo["data"].get_object(), ds, ctx);
        }
        else if(is_enum(rtype)) {
            auto e_itr = enums_.find(rtype);
            ctx.hint_enum_type_if_in_array(e_itr);

            auto& et = e_itr->second;
            auto& es = var.get_string();

            auto index = 0u;
            for(auto& field : et.fields) {
                if(field == es) {
                    break;
                }
                index++;
            }
            EVT_ASSERT2(index < et.fields.size(), pack_exception, "Invalid value of enum '{}'", ctx.get_path_string());

            _variant_to_binary(et.integer, fc::variant(index), ds, ctx);
        }
        else if(is_struct(rtype)) {
            auto s_itr = structs_.find(rtype);
            ctx.hint_struct_type_if_in_array(s_itr);

            auto& st = s_itr->second;
            if(var.is_object()) {
                const auto& vo = var.get_object();

                if(st.base != type_name()) {
                    _variant_to_binary(resolve_type(st.base), var, ds, ctx);
                }
                for(uint32_t i = 0; i < st.fields.size(); ++i) {
                    const auto& field = st.fields[i];
                    if(vo.contains(string(field.name).c_str())) {
                        auto h1 = ctx.push_to_path(impl::field_path_item{.parent_itr = s_itr, .field_ordinal = i});
                        _variant_to_binary(field.type, vo[field.name], ds, ctx);
                    }
                    else if(is_optional(field.type)) {
                        auto h1 = ctx.push_to_path(impl::field_path_item{.parent_itr = s_itr, .field_ordinal = i});
                        _variant_to_binary(field.type, fc::variant(), ds, ctx);
                    }
                    else {
                        EVT_THROW(pack_exception, "Missing field '${f}' in input object while processing struct '${p}'",
                                  ("f", ctx.maybe_shorten(field.name))("p", ctx.get_path_string()));
                    }
                }
            }
            else if(var.is_array()) {
                const auto& va = var.get_array();
                EVT_ASSERT(st.base == type_name(), invalid_type_inside_abi,
                           "Using input array to specify the fields of the derived struct '${p}'; input arrays are currently only allowed for structs_ without a base",
                           ("p", ctx.get_path_string()));
                for(uint32_t i = 0; i < st.fields.size(); ++i) {
                    const auto& field = st.fields[i];
                    if(va.size() > i) {
                        auto h1 = ctx.push_to_path(impl::field_path_item{.parent_itr = s_itr, .field_ordinal = i});
                        _variant_to_binary(field.type, va[i], ds, ctx);
                    }
                    else {
                        EVT_THROW(pack_exception, "Early end to input array specifying the fields of struct '${p}'; require input for field '${f}'",
                                  ("p", ctx.get_path_string())("f", ctx.maybe_shorten(field.name)));
                    }
                }
            }
            else {
                EVT_THROW(pack_exception, "Unexpected input encountered while processing struct '${p}'", ("p", ctx.get_path_string()));
            }
        }
        else {
            EVT_THROW(invalid_type_inside_abi, "Unknown type ${type}", ("type", ctx.maybe_shorten(type)));
        }
    }
    FC_CAPTURE_AND_RETHROW((type)(var))
}

bytes
abi_serializer::_variant_to_binary(const type_name& type, const fc::variant& var, impl::variant_to_binary_context& ctx) const {
    try {
        auto h = ctx.enter_scope();
        EVT_ASSERT2(_is_type(type), unknown_abi_type_exception, "Unknown type: {} in ABI", type);

        auto temp = bytes(1024 * 1024);
        auto ds   = fc::datastream<char*>(temp.data(), temp.size());

        _variant_to_binary(type, var, ds, ctx);
        temp.resize(ds.tellp());
        return temp;
    }
    FC_CAPTURE_AND_RETHROW((type)(var))
}

bytes
abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var, const execution_context& exec_ctx, bool short_path) const {
    auto ctx = impl::variant_to_binary_context(*this, exec_ctx, type);
    ctx.short_path = short_path;
    return _variant_to_binary(type, var, ctx);
}

void
abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char*>& ds, const execution_context& exec_ctx, bool short_path) const {
    auto ctx = impl::variant_to_binary_context(*this, exec_ctx, type);
    ctx.short_path = short_path;
    _variant_to_binary(type, var, ds, ctx);
}

namespace impl {

void
abi_traverse_context::check_deadline() const {
    EVT_ASSERT(std::chrono::steady_clock::now() < deadline, abi_serialization_deadline_exception,
               "serialization time limit ${t}us exceeded", ("t", max_serialization_time.count()));
}

fc::scoped_exit<std::function<void()>>
abi_traverse_context::enter_scope() {
    std::function<void()> callback = [old_recursion_depth = recursion_depth, this]() {
        recursion_depth = old_recursion_depth;
    };

    ++recursion_depth;
    EVT_ASSERT(recursion_depth < abi_serializer::max_recursion_depth, abi_recursion_depth_exception,
               "recursive definition, max_recursion_depth ${r} ", ("r", abi_serializer::max_recursion_depth));

    check_deadline();

    return {std::move(callback)};
}

void
abi_traverse_context_with_path::set_path_root(const type_name& type) {
    auto rtype = self.resolve_type(type);

    if(self.is_array(rtype)) {
        root_of_path = array_type_path_root{};
    }
    else {
        auto itr1 = self.structs_.find(rtype);
        if(itr1 != self.structs_.end()) {
            root_of_path = struct_type_path_root{.itr = itr1};
        }
    }
}

fc::scoped_exit<std::function<void()>>
abi_traverse_context_with_path::push_to_path(const path_item& item) {
    std::function<void()> callback = [this]() {
        EVT_ASSERT(path.size() > 0, abi_exception,
                   "invariant failure in variant_to_binary_context: path is empty on scope exit");
        path.pop_back();
    };
    path.push_back(item);

    return {std::move(callback)};
}

void
abi_traverse_context_with_path::set_array_index_of_path_back(uint32_t i) {
    EVT_ASSERT(path.size() > 0, abi_exception, "path is empty");

    auto& b = path.back();

    EVT_ASSERT(b.contains<array_index_path_item>(), abi_exception, "trying to set array index without first pushing new array index item");

    b.get<array_index_path_item>().array_index = i;
}

void
abi_traverse_context_with_path::hint_array_type_if_in_array() {
    if(path.size() == 0 || !path.back().contains<array_index_path_item>()) {
        return;
    }
    path.back().get<array_index_path_item>().type_hint = array_type_path_root{};
}

void
abi_traverse_context_with_path::hint_struct_type_if_in_array(const map<type_name, struct_def>::const_iterator& itr) {
    if(path.size() == 0 || !path.back().contains<array_index_path_item>()) {
        return;
    }
    path.back().get<array_index_path_item>().type_hint = struct_type_path_root{.itr = itr};
}

void
abi_traverse_context_with_path::hint_variant_type_if_in_array(const map<type_name, variant_def>::const_iterator& itr) {
    if(path.size() == 0 || !path.back().contains<array_index_path_item>()) {
        return;
    }
    path.back().get<array_index_path_item>().type_hint = variant_type_path_root{.itr = itr};
}

void
abi_traverse_context_with_path::hint_enum_type_if_in_array(const map<type_name, enum_def>::const_iterator& itr) {
    if(path.size() == 0 || !path.back().contains<array_index_path_item>()) {
        return;
    }
    path.back().get<array_index_path_item>().type_hint = enum_type_path_root{.itr = itr};
}

void
output_name(std::ostream& s, const string& str, bool shorten, size_t max_length = 64) {
    constexpr size_t      min_num_characters_at_ends        = 4;
    constexpr size_t      preferred_num_tail_end_characters = 6;
    constexpr const char* fill_in                           = "...";

    static_assert(min_num_characters_at_ends <= preferred_num_tail_end_characters,
                  "preferred number of tail end characters cannot be less than the imposed absolute minimum");

    constexpr size_t fill_in_length       = __builtin_strlen(fill_in);
    constexpr size_t min_length           = fill_in_length + 2 * min_num_characters_at_ends;
    constexpr size_t preferred_min_length = fill_in_length + 2 * preferred_num_tail_end_characters;

    max_length = std::max(max_length, min_length);

    if(!shorten || str.size() <= max_length) {
        s << str;
        return;
    }

    size_t actual_num_tail_end_characters = preferred_num_tail_end_characters;
    if(max_length < preferred_min_length) {
        actual_num_tail_end_characters = min_num_characters_at_ends + (max_length - min_length) / 2;
    }

    s.write(str.data(), max_length - fill_in_length - actual_num_tail_end_characters);
    s.write(fill_in, fill_in_length);
    s.write(str.data() + (str.size() - actual_num_tail_end_characters), actual_num_tail_end_characters);
}

struct generate_path_string_visitor {
public:
    using result_type = void;

    generate_path_string_visitor(bool shorten_names, bool track_only)
        : shorten_names(shorten_names)
        , track_only(track_only) {}

private:
    std::stringstream s;
    bool              shorten_names = false;
    bool              track_only    = false;
    path_item         last_path_item;

public:
    void
    add_dot() {
        s << ".";
    }

    void operator()(const empty_path_item& item) {}

    void
    operator()(const array_index_path_item& item) {
        if(track_only) {
            last_path_item = item;
            return;
        }

        s << "[" << item.array_index << "]";
    }

    void
    operator()(const field_path_item& item) {
        if(track_only) {
            last_path_item = item;
            return;
        }

        const auto& str = item.parent_itr->second.fields.at(item.field_ordinal).name;
        output_name(s, str, shorten_names);
    }

    void
    operator()(const variant_path_item& item) {
        if(track_only) {
            last_path_item = item;
            return;
        }

        const auto& str = item.parent_itr->second.fields.at(item.index).name;
        output_name(s, str, shorten_names);
    }

    void
    operator()(const empty_path_root& item) {}

    void
    operator()(const array_type_path_root& item) {
        s << "ARRAY";
    }

    void
    operator()(const struct_type_path_root& item) {
        const auto& str = item.itr->first;
        output_name(s, str, shorten_names);
    }

    void
    operator()(const variant_type_path_root& item) {
        const auto& str = item.itr->first;
        output_name(s, str, shorten_names);
    }

    void
    operator()(const enum_type_path_root& item) {
        const auto& str = item.itr->first;
        output_name(s, str, shorten_names);
    }

    std::string
    to_string() const {
        return s.str();
    }

private:
    friend struct abi_traverse_context_with_path;
};

struct path_item_type_visitor {
public:
    using result_type = void;

    path_item_type_visitor(std::stringstream& s, bool shorten_names)
        : s(s)
        , shorten_names(shorten_names) {}

private:
    std::stringstream& s;
    bool               shorten_names = false;

public:
    void
    operator()(const empty_path_item& item) {}

    void
    operator()(const array_index_path_item& item) {
        const auto& th = item.type_hint;
        if(th.contains<struct_type_path_root>()) {
            const auto& str = th.get<struct_type_path_root>().itr->first;
            output_name(s, str, shorten_names);
        }
        else if(th.contains<array_type_path_root>()) {
            s << "ARRAY";
        }
        else {
            s << "UNKNOWN";
        }
    }

    void
    operator()(const field_path_item& item) {
        const auto& str = item.parent_itr->second.fields.at(item.field_ordinal).type;
        output_name(s, str, shorten_names);
    }

    void
    operator()(const variant_path_item& item) {
        const auto& str = item.parent_itr->second.fields.at(item.index).type;
        output_name(s, str, shorten_names);
    }

    std::string
    to_string() const {
        return s.str();
    }
};

string
abi_traverse_context_with_path::get_path_string() const {
    bool full_path     = !short_path;
    bool shorten_names = short_path;

    auto visitor = generate_path_string_visitor(shorten_names, !full_path);
    if(full_path) {
        root_of_path.visit(visitor);
    }
    for(size_t i = 0, n = path.size(); i < n; ++i) {
        if(full_path && !path[i].contains<array_index_path_item>()) {
            visitor.add_dot();
        }
        path[i].visit(visitor);
    }

    if(!full_path) {
        if(visitor.last_path_item.contains<empty_path_item>()) {
            root_of_path.visit(visitor);
        }
        else {
            auto vis2 = path_item_type_visitor(visitor.s, shorten_names);
            visitor.last_path_item.visit(vis2);
        }
    }

    return visitor.to_string();
}

string
abi_traverse_context_with_path::maybe_shorten(const string& str) {
    if(!short_path) {
        return str;
    }

    std::stringstream s;
    output_name(s, str, true);
    return s.str();
}

}  // namespace impl

}}}  // namespace evt::chain::contracts
