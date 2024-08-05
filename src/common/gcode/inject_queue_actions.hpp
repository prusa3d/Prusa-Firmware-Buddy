#pragma once
#include <variant>
#include <enum_array.hpp>

enum class GCodePresetMacro : uint8_t {
    nozzle_cleaning,
    _cnt,
};

static constexpr EnumArray<GCodePresetMacro, const char *, GCodePresetMacro::_cnt> gcode_macro_preset_filanames {
    { GCodePresetMacro::nozzle_cleaning, "nozzle_cleaning" },
};

struct GCodeLiteral {
    const char *gcode;
};

struct GCodeMacroButton {
    uint8_t button;
};

using InjectQueueRecord = std::variant<GCodePresetMacro, GCodeMacroButton, GCodeLiteral>;
