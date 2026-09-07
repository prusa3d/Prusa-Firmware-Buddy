#pragma once

#include <cstdint>
#include <utility>

#include <utils/enum_array.hpp>
#include <i18n.h>

#include <option/has_gcode_compatibility.h>

/// !!! DO NOT CHANGE, STORED IN EEPROM
enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2,
    _last = Abort
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

extern const EnumArray<HWCheckType, const char *, hw_check_type_count> hw_check_type_names;
