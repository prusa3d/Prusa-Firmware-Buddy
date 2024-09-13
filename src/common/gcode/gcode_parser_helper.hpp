#pragma once

#include <optional>
#include <string_view>
#include <cstdint>
#include <cctype>
#include <charconv>

class GCodeBasicParser;

/// Internal class for GCodeBasicParser to assist with the parsing.
/// Put into a separate file so that subclasses of GCodeBasicParser can use it, too
class GCodeParserHelper {

public:
    GCodeParserHelper(const GCodeBasicParser &parent, const std::string_view &str = {});

    void reset(const std::string_view &str);

    /// Reports an error to the \p parent
    /// Note: the first argument of this function is actually the implicit \p this, so the 2, 3 in the \p __attribute__ are correct
    void report_error(const char *msg, ...) __attribute__((format(printf, 2, 3)));

    /// \returns character the parser is currently on (or \0 when reached end of string)
    inline char ch() const {
        return ch_;
    }

    /// \returns current position of the parser within the \p parent.gcode()
    size_t pos() const;

    /// \returns pointer to the currently parsed character
    inline const char *ch_ptr() const {
        return ch_ptr_;
    }

    /// Advances to the next character in the string.
    /// Sets \p ch to the new character of \0 if out of bounds.
    void advance();

    /// Advances over whitespaces
    void skip_whitespaces();

    using ParseIntegerValue = int32_t;

    template <std::integral T>
    inline std::optional<T> parse_integer(T min_value = std::numeric_limits<T>::min(), T max_value = std::numeric_limits<T>::max()) {
        return parse_integer_impl(min_value, max_value).transform([](auto val) { return static_cast<T>(val); });
    }

protected:
    const GCodeBasicParser &parent;

    char ch_ = '\0';

    const char *ch_begin = nullptr;

    /// Pointer to the current char
    const char *ch_ptr_ = nullptr;

    /// Pointer to the string end
    const char *ch_end = nullptr;

private:
    /// Parses a single integral number and \returns it
    /// If the parsed number is outside \p min_value and \p max_value, reports error and \returns nullopt
    std::optional<ParseIntegerValue> parse_integer_impl(ParseIntegerValue min_value, ParseIntegerValue max_value);
};
