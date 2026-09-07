#include <common/hw_check.hpp>

constinit const EnumArray<HWCheckType, const char *, hw_check_type_count> hw_check_type_names {
    { HWCheckType::nozzle, N_("Nozzle") },
        { HWCheckType::model, N_("Printer Model") },
        { HWCheckType::firmware, N_("Firmware Version") },
#if HAS_GCODE_COMPATIBILITY()
        { HWCheckType::gcode_compatibility, N_("G-Code Compatibility") },
#endif
        { HWCheckType::gcode_level, N_("G-Code Level") },
        { HWCheckType::input_shaper, N_("Input Shaper") },
};
