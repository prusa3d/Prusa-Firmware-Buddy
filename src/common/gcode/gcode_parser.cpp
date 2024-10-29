#include "gcode_parser.hpp"
#include "gcode_parser_helper.hpp"

#include <cassert>
#include <cstring>
#include <cstdarg>
#include <cinttypes>
#include <str_utils.hpp>

GCodeParser2::StoreOptionResult GCodeParser2::store_option(char key, std::string_view &target, std::span<char> buffer) const {
    if (key < first_option_letter || key > last_option_letter) {
        assert(0); // You shouldn't be feeding invalid keys to the function
        return std::unexpected(OptionError::not_present);
    }

    const auto pos = option_positions[key - first_option_letter];
    if (pos == 0) {
        return std::unexpected(OptionError::not_present);
    }

    GCodeParserHelper p(*this, gcode().substr(pos));
    auto result = parse_option_value(p, buffer);
    if (!result) {
        return std::unexpected(OptionError::parse_error);
    }

    target = *result;
    return {};
}

GCodeParser2::StoreOptionResult GCodeParser2::store_option(char key, bool &target) const {
    std::array<char, 2> buffer;
    const auto str = option<std::string_view>(key, buffer);
    if (!str) {
        return std::unexpected(str.error());
    }

    if (str->empty() || str == "1") {
        target = true;
        return {};

    } else if (str == "0") {
        target = false;
        return {};

    } else {
        report_option_error(key, "Expected 0, 1 or no value");
        return std::unexpected(OptionError::parse_error);
    }
}

GCodeParser2::StoreOptionResult GCodeParser2::store_option(char key, LargestInteger &target, LargestInteger min_value, LargestInteger max_value) const {
    std::array<char, 32> buffer;
    const auto str = option<std::string_view>(key, buffer);
    if (!str) {
        return std::unexpected(str.error());
    }

    LargestInteger val;
    const auto err = from_chars_light(str->begin(), str->end(), val, 10);
    if (err.ec != std::errc {}) {
        report_option_error(key, "Integer parsing failed");
        return std::unexpected(OptionError::parse_error);

    } else if (val < min_value || val > max_value) {
        static_assert(std::is_same_v<LargestInteger, int32_t>);
        report_option_error(key, "Numeric value %" PRIi32 " out of bounds <%" PRIi32 ", %" PRIi32 ">", val, min_value, max_value);
        return std::unexpected(OptionError::parse_error);
    }

    target = val;
    return {};
}

GCodeParser2::StoreOptionResult GCodeParser2::store_option(char key, float &target, float min_value, float max_value) const {
    std::array<char, 32> buffer;
    const auto str = option<std::string_view>(key, buffer);
    if (!str) {
        return std::unexpected(str.error());
    }

    float val;
    const auto err = from_chars_light(str->begin(), str->end(), val);
    if (err.ec != std::errc {}) {
        report_option_error(key, "Float parsing failed");
        return std::unexpected(OptionError::parse_error);

    } else if (val < min_value || val > max_value) {
        report_option_error(key, "Numeric value %f out of bounds <%f, %f>", static_cast<double>(val), static_cast<double>(min_value), static_cast<double>(max_value));
        return std::unexpected(OptionError::parse_error);
    }

    target = val;
    return {};
}

bool GCodeParser2::parse(const std::string_view &gcode) {
    // Reset state
    option_positions = {};

    if (!GCodeBasicParser::parse(gcode)) {
        return false;
    }

    if (body().empty()) {
        return true;
    }

    GCodeParserHelper p(*this, body());

    // Parse parameters in a loop
    while (true) {
        p.skip_whitespaces();

        if (p.ch() == '\0') {
            // Hit end of the body -> everything is parsed
            break;

        } else if (p.ch() < first_option_letter || p.ch() > last_option_letter) {
            p.report_error("Expected uppercase parameter letter");
            return false;
        }

        const char option_letter = p.ch();
        p.advance();
        p.skip_whitespaces();

        assert(p.pos() < 256);
        auto &opt_pos = option_positions[option_letter - first_option_letter];
        if (opt_pos != 0) {
            p.report_error("Option '%c' already defined", option_letter);
            return false;
        }
        opt_pos = static_cast<uint8_t>(p.pos());

        if (!parse_option_value(p)) {
            return false;
        }
    }

    return true;
}
std::optional<std::string_view> GCodeParser2::parse_option_value(GCodeParserHelper &p, std::span<char> accumulator) const {
    auto accum_it = accumulator.begin();

    const auto accumulate = [&](char ch) {
        assert(ch != '\0');

        if (accumulator.empty()) {
            return true;
        }

        if (accum_it == accumulator.end()) {
            p.report_error("Value too long");
            return false;
        }

        *accum_it = ch;
        accum_it++;
        return true;
    };

    if (p.ch() == '"') {
        /// If set, next read character will be escaped (if possible)
        bool escape_char = false;

        // Quoted value -> read till ending "
        while (true) {
            p.advance();

            if (p.ch() == '\0') {
                p.report_error("Unterminated quotes");
                return std::nullopt;
            }

            if (escape_char) {
                escape_char = false;
                if (!accumulate(p.ch())) {
                    return std::nullopt;
                }
                continue;
            }

            switch (p.ch()) {

            case '\\':
                escape_char = true;
                break;

            case '"':
                p.advance();
                return std::string_view(accumulator.begin(), accum_it);

            default:
                if (!accumulate(p.ch())) {
                    return std::nullopt;
                }
                break;
            }
        }
    } else {
        // Unquoted value -> basically we are expecting a number.
        // Accept anything that might look like a number, let from_chars_light figure out the rest
        while (p.ch() && strchr("0123456789+-.,", p.ch())) {
            if (!accumulate(p.ch())) {
                return std::nullopt;
            }
            p.advance();
        }

        return std::string_view(accumulator.begin(), accum_it);
    }
}

void GCodeParser2::report_option_error(char key, const char *msg, ...) const {
    if (!error_callback_) {
        return;
    }

    va_list args;
    va_start(args, msg);
    error_callback_({
                        .parser = *this,
                        .position = option_positions[key - first_option_letter],
                        .message = msg,
                    },
        args);
    va_end(args);
}
