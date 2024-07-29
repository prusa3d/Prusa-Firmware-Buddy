#pragma once

#include <cstdint>

#include <enum_array.hpp>
#include <i18n.h>

#include <inc/MarlinConfig.h>

enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2
};

enum class HWCheckType : uint8_t {
    nozzle,
    model,
    firmware,
#if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
    fan_compatibility,
#endif
#if ENABLED(GCODE_COMPATIBILITY_MK3)
    mk3_compatibility,
#endif
    gcode,

    _last = gcode
};

static constexpr size_t hw_check_type_count = static_cast<size_t>(HWCheckType::_last) + 1;

static constexpr EnumArray<HWCheckType, const char *, hw_check_type_count> hw_check_type_names {
    { HWCheckType::nozzle, N_("Nozzle") },
        { HWCheckType::model, N_("Printer Model") },
        { HWCheckType::firmware, N_("Firmware Version") },
#if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
        { HWCheckType::fan_compatibility, N_("Fan Compabibility") },
#endif
#if ENABLED(GCODE_COMPATIBILITY_MK3)
        { HWCheckType::mk3_compatibility, N_("MK3 Compatibility") },
#endif
        { HWCheckType::gcode, N_("G-Code Level") },
};
