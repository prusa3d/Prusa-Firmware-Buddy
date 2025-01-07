#pragma once

#include <variant>
#include <str_utils.hpp>

struct GCodeFilename {
    ConstexprString name;
    ConstexprString fallback { nullptr };
};

struct GCodeLiteral {
    ConstexprString gcode;
};

struct GCodeMacroButton {
    uint8_t button;
};

using InjectQueueRecord = std::variant<GCodeFilename, GCodeMacroButton, GCodeLiteral>;
