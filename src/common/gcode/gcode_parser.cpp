#include "gcode_parser.hpp"

#include <Marlin/src/gcode/parser.h>

GCodeParser2::GCodeParser2(FromMarlinParser) {
    // This is currently just a wrapper, no need to do anything.
}

template <>
GCodeParser2::OptionResult<bool> GCodeParser2::option<bool>(char key) const {
    // !!! Seen is intentional here, just flag without the value is considered true
    if (!parser.seen(key)) {
        return std::unexpected(OptionError::not_present);
    }

    return parser.value_bool();
}

template <>
GCodeParser2::OptionResult<float> GCodeParser2::option<float>(char key) const {
    if (!parser.seenval(key)) {
        return std::unexpected(OptionError::not_present);
    }

    return parser.value_float();
}

template <>
GCodeParser2::OptionResult<uint32_t> GCodeParser2::option<uint32_t>(char key) const {
    if (!parser.seenval(key)) {
        return std::unexpected(OptionError::not_present);
    }

    return parser.value_long();
}

template <>
GCodeParser2::OptionResult<uint16_t> GCodeParser2::option<uint16_t>(char key) const {
    // TODO: Overflow checks
    return option<uint32_t>(key).transform([](auto val) { return static_cast<uint16_t>(val); });
}

template <>
GCodeParser2::OptionResult<uint8_t> GCodeParser2::option<uint8_t>(char key) const {
    // TODO: Overflow checks
    return option<uint32_t>(key).transform([](auto val) { return static_cast<uint8_t>(val); });
}
