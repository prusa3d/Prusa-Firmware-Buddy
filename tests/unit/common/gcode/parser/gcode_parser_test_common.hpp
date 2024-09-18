#pragma once

#include <gcode_basic_parser.hpp>

void fail_test_error_callback(const GCodeBasicParser::ErrorCallbackArgs &args, va_list va) {
    INFO("At " << args.position);
    INFO("Remainder " << args.parser.gcode().substr(args.position));

    std::array<char, 128> err;
    vsnprintf(err.data(), err.size(), args.message, va);
    FAIL(err.data());
}

/// \returns whether a parser \param p error was reported during execution of \param f
bool execution_failed(GCodeBasicParser &p, std::function<void()> f) {
    const auto prev_callback = p.error_callback();
    bool error_reported = false;
    p.set_error_callback([&](auto...) { error_reported = true; });
    f();
    p.set_error_callback(prev_callback);
    return error_reported;
}

/// \returns true if parsing \param gcode fails
template <typename Parser>
bool test_parse_failure(const char *gcode) {
    bool error_reported = false;
    const char *msg = nullptr;

    Parser p;
    p.set_error_callback([&](const auto &data, auto) {
        error_reported = true;
        msg = data.message;
    });
    const bool parse_result = p.parse(gcode);
    CHECK(parse_result == !error_reported);
    UNSCOPED_INFO(msg);
    return !parse_result;
}
