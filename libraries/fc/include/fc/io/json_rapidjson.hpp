#pragma once

// This file is an internal header,
// it is not meant to be included except internally from json.cpp in fc

#include <stack>
#include <vector>
#include <tuple>
#include <boost/any.hpp>
#include <fc/io/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

namespace fc { namespace rapidjson {

template<typename T, bool strict>
variant variant_from_stream( T& in, uint32_t max_depth );

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
        obj_levels_.emplace(std::make_tuple(kObject, mutable_variant_object()));
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
        if(std::get<0>(l) != kObject) {
            return false;
        }
        auto  v   = std::move(std::get<1>(l));
        auto& obj = boost::any_cast<mutable_variant_object&>(v);

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
        obj_levels_.push(std::make_tuple(kArray, variants()));
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
        if(std::get<0>(l) != kArray) {
            return false;
        }
        auto v    = std::move(std::get<1>(l));
        auto& arr = boost::any_cast<variants&>(v);

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

        switch(std::get<0>(l)) {
        case kObject: {
            auto& obj = boost::any_cast<mutable_variant_object&>(std::get<1>(l));
            FC_ASSERT(!key_levels_.empty());
            auto& key = key_levels_.top();
            obj(std::move(key), std::move(v));
            key_levels_.pop();
            break;
        }
        case kArray: {
            auto& arr = boost::any_cast<variants&>(std::get<1>(l));
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
    using obj_level = std::tuple<int, boost::any>;

    template<typename T>
    using stack = std::stack<T, std::vector<T>>;

    stack<std::string> key_levels_;
    stack<obj_level>   obj_levels_;
    variant&           v_;
    uint32_t           max_depth_;
};

template<typename T, bool strict>
variant
variant_from_stream(T& in, uint32_t max_depth) {
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

}}  // namespace fc::rapidjson
