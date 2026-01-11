#pragma once

#include <cstdint>

#include <utils/enum_array.hpp>
#include <i18n.h>

#include <inc/MarlinConfig.h>
#include <option/has_gcode_compatibility.h>

enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2
};

enum class HWCheckType : uint8_t {
    nozzle,
    model,
    firmware,
#if HAS_GCODE_COMPATIBILITY()
    gcode_compatibility,
#endif
    gcode_level,
    input_shaper,
    _last = input_shaper,
};

static constexpr size_t hw_check_type_count = static_cast<size_t>(HWCheckType::_last) + 1;

static constexpr EnumArray<HWCheckType, const char *, hw_check_type_count> hw_check_type_names {
    { HWCheckType::nozzle, N_("Nozzle") },
        { HWCheckType::model, N_("Printer Model") },
        { HWCheckType::firmware, N_("Firmware Version") },
#if HAS_GCODE_COMPATIBILITY()
        { HWCheckType::gcode_compatibility, N_("G-code Compatibility") },
#endif
        { HWCheckType::gcode_level, N_("G-code Level") },
        { HWCheckType::input_shaper, N_("Input Shaper") },
};
