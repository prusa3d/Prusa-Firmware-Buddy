#pragma once
#include <variant>
#include <enum_array.hpp>
#include <async_job/async_job_execution_control.hpp>

namespace {
typedef void (*InjectPresetMacroCallback)(AsyncJobExecutionControl &);
}

struct InjectPresetMacro {
    InjectPresetMacroCallback callback;
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

using InjectQueueRecord = std::variant<InjectPresetMacro, GCodeFilename, GCodeMacroButton, GCodeLiteral>;
