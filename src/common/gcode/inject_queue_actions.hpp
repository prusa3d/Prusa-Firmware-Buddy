#pragma once
#include <variant>
#include <enum_array.hpp>
#include <async_job/async_job_execution_control.hpp>

typedef void (*GCodePresetMacroCallback)(AsyncJobExecutionControl &);

struct GCodePresetMacro {
    GCodePresetMacroCallback callback;
};

struct GCodeFilename {
    const char *name;
};

struct GCodeLiteral {
    const char *gcode;
};

struct GCodeMacroButton {
    uint8_t button;
};

using InjectQueueRecord = std::variant<GCodePresetMacro, GCodeFilename, GCodeMacroButton, GCodeLiteral>;
