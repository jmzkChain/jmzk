#pragma once
/**
 * @file log_message.hpp
 * @brief Defines types and helper macros necessary for generating log messages.
 */
#include <memory>
#include <fmt/core.h>
#include <fc/time.hpp>
#include <fc/variant_object.hpp>
#include <fc/container/small_vector_fwd.hpp>

namespace fc {

/**
 * Named scope for log_level enumeration.
 */
class log_level {
public:
    /**
      * @brief Define's the various log levels for reporting.  
      *
      * Each log level includes all higher levels such that 
      * Debug includes Error, but Error does not include Debug.
      */
    enum values {
        all,
        debug,
        info,
        warn,
        error,
        off
    };

    log_level(values v = off)
        : value(v) {}

    explicit log_level(int v)
        : value(static_cast<values>(v)) {}

    operator int() const { return value; }
    string to_string() const;
    values value;
};

void to_variant(log_level e, variant& v);
void from_variant(const variant& e, log_level& ll);

/**
 *  @brief provides information about where and when a log message was generated.
 *  @ingroup AthenaSerializable
 *
 *  @see FC_LOG_CONTEXT
 */
class log_context {
public:
    log_context() = default;
    log_context(log_level   ll,
                const char* file,
                uint64_t    line,
                const char* method);

    explicit log_context(const variant& v);

public:
    void append_context(const fc::string& c);
    string  to_string() const;
    variant to_variant() const;

public:
    log_level  level;
    string     file;
    uint64_t   line;
    string     method;
    string     thread_name;
    string     task_name;
    string     hostname;
    string     context;
    time_point timestamp;
};

void to_variant(const log_context& l, variant& v);
void from_variant(const variant& l, log_context& c);

/**
 *  @brief aggregates a message along with the context and associated meta-information.
 *  @ingroup AthenaSerializable
 *
 *  @note log_message has reference semantics, all copies refer to the same log message
 *  and the message is read-only after construction.
 *
 *  When converted to JSON, log_message has the following form:
 *  @code
 *  {
 *     "context" : { ... },
 *     "format"  : "string with ${keys}",
 *     "data"    : { "keys" : "values" }
 *  }
 *  @endcode
 *
 *  @see FC_LOG_MESSAGE
 */
class log_message {
public:
    log_message();
    /**
     *  @param ctx - generally provided using the FC_LOG_CONTEXT(LEVEL) macro 
     */
    log_message(log_context&& ctx, std::string&& format, variant_object&& args)
        : context(std::move(ctx)), format(std::move(format)), args(std::move(args)) {}

    log_message(log_context&& ctx, std::string&& message)
        : context(std::move(ctx))
        , format(std::move(message)) {}
    
    log_message(const variant& v);

    variant to_variant() const;
    string  get_message() const;

public:
    log_context    context;
    std::string    format;
    variant_object args;
};

void to_variant(const log_message& l, variant& v);
void from_variant(const variant& l, log_message& c);

typedef fc::small_vector<log_message, 4> log_messages;

}  // namespace fc

FC_REFLECT_TYPENAME(fc::log_message);

#ifndef __func__
#define __func__ __FUNCTION__
#endif

/**
 * @def FC_LOG_CONTEXT(LOG_LEVEL)
 * @brief Automatically captures the File, Line, and Method names and passes them to
 *        the constructor of fc::log_context along with LOG_LEVEL
 * @param LOG_LEVEL - a valid log_level::Enum name.
 */
#define FC_LOG_CONTEXT(LOG_LEVEL) \
    fc::log_context(fc::log_level::LOG_LEVEL, __FILE__, __LINE__, __func__)

/**
 * @def FC_LOG_MESSAGE(LOG_LEVEL,FORMAT,...)
 *
 * @brief A helper method for generating log messages.
 *
 * @param LOG_LEVEL a valid log_level::Enum name to be passed to the log_context
 * @param FORMAT A const char* string containing zero or more references to keys as "${key}"
 * @param ...  A set of key/value pairs denoted as ("key",val)("key2",val2)...
 */
#define FC_LOG_MESSAGE(LOG_LEVEL, FORMAT, ...) \
    fc::log_message(FC_LOG_CONTEXT(LOG_LEVEL), FORMAT, fc::mutable_variant_object() __VA_ARGS__)

#define FC_LOG_MESSAGE2(LOG_LEVEL, FORMAT, ...) \
    fc::log_message(FC_LOG_CONTEXT(LOG_LEVEL), fmt::format(FORMAT, __VA_ARGS__))

