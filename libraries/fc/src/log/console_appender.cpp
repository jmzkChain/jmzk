#include <fc/log/console_appender.hpp>

#include <iomanip>
#include <mutex>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#endif
#include <boost/thread/mutex.hpp>

#include <fmt/format.h>

#include <fc/exception/exception.hpp>
#include <fc/log/log_message.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>

#define COLOR_CONSOLE 1
#include "console_defines.h"

namespace fc {

class console_appender::impl {
public:
    config       cfg;
    boost::mutex log_mutex;
    color::type  lc[log_level::off + 1];
    bool         use_syslog_header{getenv("JOURNAL_STREAM")};
#ifdef WIN32
    HANDLE console_handle;
#endif
};

console_appender::console_appender(const variant& args)
    : my(new impl) {
    configure(args.as<config>());
}

console_appender::console_appender(const config& cfg)
    : my(new impl) {
    configure(cfg);
}

console_appender::console_appender()
    : my(new impl) {}

void
console_appender::configure(const config& console_appender_config) {
    try {
#ifdef WIN32
        my->console_handle = INVALID_HANDLE_VALUE;
#endif
        my->cfg = console_appender_config;
#ifdef WIN32
        if(my->cfg.stream = stream::std_error)
            my->console_handle = GetStdHandle(STD_ERROR_HANDLE);
        else if(my->cfg.stream = stream::std_out)
            my->console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

        for(int i = 0; i < log_level::off + 1; ++i)
            my->lc[i] = color::console_default;
        for(auto itr = my->cfg.level_colors.begin(); itr != my->cfg.level_colors.end(); ++itr)
            my->lc[itr->level] = itr->color;
    }
    FC_CAPTURE_AND_RETHROW((console_appender_config))
}

console_appender::~console_appender() {}

#ifdef WIN32
static WORD
#else
static const char*
#endif
get_console_color(console_appender::color::type t) {
    switch(t) {
    case console_appender::color::red:
        return CONSOLE_RED;
    case console_appender::color::green:
        return CONSOLE_GREEN;
    case console_appender::color::brown:
        return CONSOLE_BROWN;
    case console_appender::color::blue:
        return CONSOLE_BLUE;
    case console_appender::color::magenta:
        return CONSOLE_MAGENTA;
    case console_appender::color::cyan:
        return CONSOLE_CYAN;
    case console_appender::color::white:
        return CONSOLE_WHITE;
    case console_appender::color::console_default:
    default:
        return CONSOLE_DEFAULT;
    }
}

string
fixed_size(size_t s, const string& str) {
    if(str.size() == s)
        return str;
    if(str.size() > s)
        return str.substr(0, s);
    string tmp = str;
    tmp.append(s - str.size(), ' ');
    return tmp;
}

void
console_appender::log(const log_message& m) {
    FILE* out = stream::std_error ? stderr : stdout;

    auto& context = m.context;
    auto  line    = fmt::memory_buffer();

    if(my->use_syslog_header) {
        switch(context.level) {
        case log_level::error: {
            fmt::format_to(line, "<3>");
            break;
        }
        case log_level::warn: {
            fmt::format_to(line, "<4>");
            break;
        }
        case log_level::info: {
            fmt::format_to(line, "<6>");
            break;
        }
        case log_level::debug: {
            fmt::format_to(line, "<7>");
            break;
        }
        }  // switch
    }
    fmt::format_to(line, "{:<5} {} {:<9} {:<28} ",
        context.level.to_string(),
        (std::string)time_point::now(),
        context.thread_name,
        fmt::format("{}:{}", context.file.substr(0, 22), context.line));

    // strip all leading scopes...
    if(!context.method.empty()) {
        auto p = context.method.find_last_of(':');
        if(p == std::string::npos) {
            p = 0;
        }
        else {
            p++;
        }

        fmt::format_to(line, "{:<20}", context.method.substr(p, 20));
    }

    fmt::format_to(line, "] {}", fc::format_string(m.format, m.args));
    
    {
        std::unique_lock<boost::mutex> lock(my->log_mutex);

        print(fmt::to_string(line), my->lc[context.level]);
        fprintf(out, "\n");

        if(my->cfg.flush) {
            fflush(out);
        }
    }
}

void
console_appender::print(const std::string& text, color::type text_color) {
    FILE* out = stream::std_error ? stderr : stdout;

#ifdef WIN32
    if(my->console_handle != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(my->console_handle, get_console_color(text_color));
#else
    if(isatty(fileno(out)))
        fprintf(out, "%s", get_console_color(text_color));
#endif

    if(text.size())
        fprintf(out, "%s", text.c_str());  //fmt_str.c_str() );

#ifdef WIN32
    if(my->console_handle != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(my->console_handle, CONSOLE_DEFAULT);
#else
    if(isatty(fileno(out)))
        fprintf(out, "%s", CONSOLE_DEFAULT);
#endif

    if(my->cfg.flush)
        fflush(out);
}

}  // namespace fc
