#pragma once

// This file is an internal header,
// it is not meant to be included except internally from json.cpp in fc

#include <tuple>
#include <boost/any.hpp>
#include <fc/io/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>

namespace fc { namespace rapidjson {

template<typename T, bool strict>
variant variant_from_stream( T& in, uint32_t max_depth );

class VariantHandler : public BaseReaderHandler<UTF8<>, VariantHandler> {
public:
    VariantHandler(variant& v) : v_(v)
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
        obj_levels_.push(std::make_tuple(kObject, mutable_variant_object()));
        return true;
    }

    bool
    Key(const Ch* str, SizeType len, bool copy) {
        key_ = std::string(str, len);
        key_levels_.emplace_back(std::move(key_));
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
        auto v = std::move(std::get<1>(l));
        obj_levels_.pop();
        if(obj_levels_.empty()) {
            // root
            v_ = std::move(v);
            return true;
        }
        return insert_element(std::move(v));
    }

    bool
    StartArray() {
        obj_levels_.push(std::make_tuple(kArray, variants()));
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
        auto v = std::move(std::get<1>(l));
        obj_levels_.pop();
        if(obj_levels_.empty()) {
            // root
            v_ = std::move(v);
            return true;
        }
        return insert_element(std::move(v));
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

    std::stack<std::string> key_levels_;
    std::stack<obj_level>   obj_levels_;
    variant&                v_;
};

template<typename T, bool strict>
variant
variant_from_stream(T& in, uint32_t max_depth) {
    using namespace rapidjson;

    variant var;

    Reader reader;
    VariantHandler handler(var);

    BasicIStreamWrapper ss(in);
    if(!reader.Parse(ss, handler)) {
        auto e = reader.GetParseErrorCode();
        FC_THROW_EXCEPTION(parse_error_exception, "Unexpected content, err: ${err}, offset: ${offset}",
            ("err",GetParseError_En(e))("offset",reader.GetErrorOffset()));
    }

    return var;
}

}}  // namespace fc::rapidjson
