#pragma once

#include <variant>
#include <limits>
#include <str_utils.hpp>

class StringBuilder;

struct GCodeFile {
    const char *filename { nullptr };
    ConstexprString directory { nullptr }; // An optional subdirectory under /usb/macros/
    ConstexprString default_gcode { nullptr };
};

/*
 * The user is responsible for ensuring the G-code string has the correct format
 * specifier (e.g., "%f" or "%d") if a parameter is provided. For example:
 *   GCodeLiteral("M600 T%d P", 3.0f);
 *
 * When no parameter is necessary, omit it or pass std::nullopt:
 *   GCodeLiteral("M17");
 *
 * It is up to the caller to perform the actual string formatting if needed.
 * NaN is representing empty parameter.
 */
struct GCodeLiteral {
    ConstexprString gcode = nullptr;
    float parameter = std::numeric_limits<float>::quiet_NaN();

    inline bool is_empty() const {
        return gcode == nullptr;
    }

    /// Transforms the literal into a null-terminated string
    void to_string(StringBuilder &sb) const;
};

struct GCodeMacroButton {
    uint8_t button;
};

using InjectQueueRecord = std::variant<GCodeFile, GCodeMacroButton, GCodeLiteral>;
