#include <fc/log/log_message.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>

namespace fc {

const string& get_thread_name();

log_context::log_context(const variant& v) {
    auto& obj = v.get_object();

    level       = obj["level"].as<log_level>();
    file        = obj["file"].as_string();
    line        = obj["line"].as_uint64();
    method      = obj["method"].as_string();
    hostname    = obj["hostname"].as_string();
    thread_name = obj["thread_name"].as_string();
    timestamp   = obj["timestamp"].as<time_point>();
    
    if(obj.contains("task_name")) {
        task_name = obj["task_name"].as_string();
    }
    if(obj.contains("context")) {
        context = obj["context"].as<string>();
    }
}

fc::string
log_context::to_string() const {
    return fmt::format("{} {}:{} {}", thread_name, file, line, method);
}

void
log_context::append_context(const fc::string& s) {
    if(!context.empty()) {
        context.append(" -> ");
    }
    context.append(s);
}

void
to_variant(const log_context& l, variant& v) {
    v = l.to_variant();
}

void
from_variant(const variant& l, log_context& c) {
    c = log_context(l);
}

void
from_variant(const variant& l, log_message& c) {
    c = log_message(l);
}

void
to_variant(const log_message& m, variant& v) {
    v = m.to_variant();
}

void
to_variant(log_level e, variant& v) {
    switch(e) {
    case log_level::all:
        v = "all";
        return;
    case log_level::debug:
        v = "debug";
        return;
    case log_level::info:
        v = "info";
        return;
    case log_level::warn:
        v = "warn";
        return;
    case log_level::error:
        v = "error";
        return;
    case log_level::off:
        v = "off";
        return;
    }
}

void
from_variant(const variant& v, log_level& e) {
    try {
        if(v.as_string() == "all")
            e = log_level::all;
        else if(v.as_string() == "debug")
            e = log_level::debug;
        else if(v.as_string() == "info")
            e = log_level::info;
        else if(v.as_string() == "warn")
            e = log_level::warn;
        else if(v.as_string() == "error")
            e = log_level::error;
        else if(v.as_string() == "off")
            e = log_level::off;
        else
            FC_THROW_EXCEPTION(bad_cast_exception, "Failed to cast from Variant to log_level");
    }
    FC_RETHROW_EXCEPTIONS(error,
                          "Expected 'all|debug|info|warn|error|off', but got '${variant}'",
                          ("variant", v));
}

string
log_level::to_string() const {
    switch(value) {
    case log_level::all:
        return "all";
    case log_level::debug:
        return "debug";
    case log_level::info:
        return "info";
    case log_level::warn:
        return "warn";
    case log_level::error:
        return "error";
    case log_level::off:
        return "off";
    default:
        return "";
    }
}

log_context::log_context(log_level ll, const char* file, uint64_t line, const char* method)
    : level(ll)
    , file(fc::path(file).filename().generic_string())
    , line(line)
    , method(method)
    , thread_name(fc::get_thread_name())
    , timestamp(time_point::now()) {}

variant
log_context::to_variant() const {
    auto o = mutable_variant_object();
    o("level", variant(level))
     ("file", file)
     ("line", line)
     ("method", method)
     ("hostname", hostname)
     ("thread_name", thread_name)
     ("timestamp", variant(timestamp));

    if(!context.empty()) {
        o("context", context);
    }
    return o;
}

log_message::log_message(const variant& v) {
    auto& obj = v.get_object();

    context = log_context(v["context"]);
    format  = obj["format"].as_string();
    args    = obj["data"].get_object();
}

variant
log_message::to_variant() const {
    return mutable_variant_object("context", context)
                                 ("format", format)
                                 ("data", args);
}

string
log_message::get_message() const {
    return format_string(format, args);
}

}  // namespace fc
