#pragma once

// This file is an internal header,
// it is not meant to be included except internally from json.cpp in fc

#include <stack>
#include <vector>
#include <tuple>
#include <fc/io/json.hpp>
#include <fc/container/small_vector_fwd.hpp>
#include <fc/static_variant.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

namespace fc { namespace rapidjson {

template<typename T, bool strict>
variant variant_from_stream(T& in, uint32_t max_depth);

template<typename T>
void to_stream(T& out, const variant& v);

template<typename T>
void to_stream_pretty(T& out, const variant& v);

namespace __internal {

class VariantHandler : public BaseReaderHandler<UTF8<>, VariantHandler> {
public:
    VariantHandler(variant& v, uint32_t max_depth) : v_(v),
                                                     max_depth_(max_depth)
    { }

public:
    bool
    Null() {
        return insert_element(variant());
    }

    bool
    Bool(bool b) {
        return insert_element(b);
    }

    bool
    Int(int i) {
        return insert_element(i);
    }

    bool
    Uint(unsigned int i) {
        return insert_element(i);
    }

    bool
    Int64(int64_t i) {
        return insert_element(i);
    }

    bool
    Uint64(uint64_t i) {
        return insert_element(i);
    }
    bool
    Double(double d) {
        return insert_element(d);
    }

    bool
    RawNumber(const Ch* str, SizeType len, bool copy) {
        FC_THROW_EXCEPTION(parse_error_exception, "Not supported raw number");
    }

    bool
    String(const Ch* ch, SizeType len, bool copy) {
        std::string str(ch, len);
        return insert_element(str);
    }

    bool
    StartObject() {
        obj_levels_.emplace(mutable_variant_object());
        if(obj_levels_.size() > max_depth_) {
            FC_THROW_EXCEPTION(parse_error_exception, "Exceed max depth limit");
        }
        return true;
    }

    bool
    Key(const Ch* str, SizeType len, bool copy) {
        auto key = std::string(str, len);
        key_levels_.emplace(std::move(key));
        return true;
    }

    bool
    EndObject(SizeType) {
        if(obj_levels_.empty()) {
            return false;
        }
        auto& l = obj_levels_.top();
        if(l.which() != kObject) {
            return false;
        }
        auto obj = std::move(l.get<mutable_variant_object>());

        obj_levels_.pop();
        if(obj_levels_.empty()) {
            // root
            v_ = std::move(obj);
            return true;
        }
        return insert_element(std::move(obj));
    }

    bool
    StartArray() {
        obj_levels_.push(variants());
        if(obj_levels_.size() > max_depth_) {
            FC_THROW_EXCEPTION(parse_error_exception, "Exceed max depth limit");
        }
        return true;
    }

    bool
    EndArray(SizeType) {
        if(obj_levels_.empty()) {
            return false;
        }
        auto& l = obj_levels_.top();
        if(l.which() != kArray) {
            return false;
        }
        auto arr = std::move(l.get<variants>());

        obj_levels_.pop();
        if(obj_levels_.empty()) {
            // root
            v_ = std::move(arr);
            return true;
        }
        return insert_element(std::move(arr));
    }

private:
    template<typename T>
    inline bool
    insert_element(T&& v) {
        if(obj_levels_.empty()) {
            return false;
        }
        auto& l = obj_levels_.top();

        switch(l.which()) {
        case kObject: {
            auto& obj = l.get<mutable_variant_object>();
            FC_ASSERT(!key_levels_.empty());
            auto& key = key_levels_.top();
            obj(std::move(key), std::move(v));
            key_levels_.pop();
            break;
        }
        case kArray: {
            auto& arr = l.get<variants>();
            arr.push_back(variant(std::move(v)));
            break;
        }
        default: {
            FC_ASSERT(false);
        }
        } // switch
        return true;
    }

    void
    check_not_root() {
        if(obj_levels_.empty()) {
            FC_THROW_EXCEPTION(parse_error_exception, "Unexpected root type");
        }
    }

private:
    enum LevelType {
        kObject,
        kArray
    };
    using obj_level = static_variant<mutable_variant_object, variants>;

    template<typename T>
    using stack = std::stack<T, fc::small_vector<T, 8>>;

    stack<std::string> key_levels_;
    stack<obj_level>   obj_levels_;
    variant&           v_;
    uint32_t           max_depth_;
};

}  // namespace __internal

template<typename T, bool strict>
variant
variant_from_stream(T& in, uint32_t max_depth) {
    using namespace __internal;

    variant var;

    Reader reader;
    VariantHandler handler(var, max_depth);

    BasicIStreamWrapper<T> ss(in);
    if(!reader.Parse(ss, handler)) {
        auto e = reader.GetParseErrorCode();
        FC_THROW_EXCEPTION(parse_error_exception, "Unexpected content, err: ${err}, offset: ${offset}",
            ("err",GetParseError_En(e))("offset",reader.GetErrorOffset()));
    }

    return var;
}

namespace __internal {

template<typename W>
void
serialize(W& writer, const variant& v) {
    switch(v.get_type()) {
    case variant::null_type: {
        writer.Null();
        break;
    }
    case variant::int64_type: {
        writer.Int64(v.as_int64());
        break;
    }
    case variant::uint64_type: {
        writer.Uint64(v.as_uint64());
        break;
    }
    case variant::double_type: {
        writer.Double(v.as_double());
        break;
    }
    case variant::bool_type: {
        writer.Bool(v.as_bool());
        break;
    }
    case variant::string_type: {
        auto& str = v.get_string();
        writer.String(str.c_str(), str.size());
        break;
    }
    case variant::blob_type: {
        auto& blob = v.get_blob();
        writer.String(&blob.data[0], blob.data.size());
        break;
    }
    case variant::array_type: {
        auto& arr = v.get_array();

        writer.StartArray();
        for(auto& a : arr) {
            serialize(writer, a);
        }
        writer.EndArray();
        break;
    }
    case variant::object_type: {
        auto& obj = v.get_object();

        writer.StartObject();
        for(auto& it : obj) {
            auto& key = it.key();
            writer.Key(key.c_str(), key.size());
            serialize(writer, it.value());
        }
        writer.EndObject();
        break;
    }
    default: {
        FC_THROW_EXCEPTION(fc::invalid_arg_exception, "Unsupported variant type: " + std::to_string(v.get_type()));
    }
    }  // switch
}

}  // namespace __internal

template<typename T>
void
to_stream(T& out, const variant& v) {
    using namespace __internal;

    BasicOStreamWrapper<T> ss(out);

    Writer<BasicOStreamWrapper<T>> writer(ss);
    serialize(writer, v);
}

template<typename T>
void
to_stream_pretty(T& out, const variant& v) {
    using namespace __internal;

    BasicOStreamWrapper<T> ss(out);

    PrettyWriter<BasicOStreamWrapper<T>> writer(ss);
    serialize(writer, v);
}

}}  // namespace fc::rapidjson
