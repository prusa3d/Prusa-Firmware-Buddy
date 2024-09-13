#include "gcode_parser_helper.hpp"
#include "gcode_basic_parser.hpp"

#include <cstdarg>
#include <cinttypes>

GCodeParserHelper::GCodeParserHelper(const GCodeBasicParser &parent, const std::string_view &str)
    : parent(parent) //
{
    reset(str);
    ch_ = (ch_ptr_ < ch_end) ? *ch_ptr_ : '\0';
}

void GCodeParserHelper::reset(const std::string_view &str) {
    ch_begin = str.begin();
    ch_ptr_ = str.begin();
    ch_end = str.end();
}

void GCodeParserHelper::report_error(const char *msg, ...) {
    if (!parent.error_callback_) {
        return;
    }

    va_list args;
    va_start(args, msg);
    parent.error_callback_(GCodeBasicParser::ErrorCallbackArgs {
                               .parser = parent,
                               .position = pos(),
                               .message = msg,
                           },
        args);
    va_end(args);
}

size_t GCodeParserHelper::pos() const {
    return ch_ptr_ - parent.gcode().data();
}

void GCodeParserHelper::advance() {
    if (ch_ptr_ < ch_end) {
        ch_ptr_++;
    }

    ch_ = (ch_ptr_ == ch_end) ? '\0' : *ch_ptr_;
}

void GCodeParserHelper::skip_whitespaces() {
    while (isspace(ch())) {
        advance();
    }
}

std::optional<GCodeParserHelper::ParseIntegerValue> GCodeParserHelper::parse_integer_impl(ParseIntegerValue min_value, ParseIntegerValue max_value) {
    const char *start = ch_ptr();

    // Accept anything that might look like a number, let std::from_chars figure out the rest
    while (isdigit(ch()) || ch() == '-') {
        advance();
    }

    ParseIntegerValue val;
    const auto err = std::from_chars(start, ch_ptr(), val, 10);

    if (err.ec != std::errc {}) {
        report_error("Integer parsing failed");
        return std::nullopt;

    } else if (val < min_value || val > max_value) {
        static_assert(std::is_same_v<ParseIntegerValue, int32_t>);
        report_error("Numeric value %" PRIi32 " out of bounds <%" PRIi32 ", %" PRIi32 ">", val, min_value, max_value);
        return std::nullopt;
    }

    return val;
}
