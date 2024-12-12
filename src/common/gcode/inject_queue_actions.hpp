#pragma once
#include <variant>

struct GCodeFilename {
    const char *name;
    const char *fallback { nullptr };
};

struct GCodeLiteral {
    const char *gcode;
};

struct GCodeMacroButton {
    uint8_t button;
};

using InjectQueueRecord = std::variant<GCodeFilename, GCodeMacroButton, GCodeLiteral>;
