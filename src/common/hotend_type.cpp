#include "hotend_type.hpp"

#include <bsod/bsod.h>

#include <option/has_nextruder.h>

const char *hotend_type_name(HotendType t) {
    switch (t) {

    case HotendType::stock:
        return N_("Stock");

#if !PRINTER_IS_PRUSA_MINI()
    case HotendType::stock_with_sock:
        return N_("With sock");
#endif

#if PRINTER_IS_PRUSA_MK3_5()
    case HotendType::e3d_revo:
        return "E3D Revo";
#endif

#if HAS_HT_HOTEND()
    case HotendType::high_temp:
        return N_("High-temp");
#endif
    }

    // This shouldn't happen, but if it does, let the firmware continue.
    // Might be due to mis-migration in config-store.
    debug_assert(0);
    return nullptr;
}

int8_t hotend_type_heater_selftest_offset(HotendType t) {
    switch (t) {

    case HotendType::stock:
        return 0;

#if PRINTER_IS_PRUSA_MK3_5()
    case HotendType::stock_with_sock:
        return -25;
#elif HAS_NEXTRUDER()
    case HotendType::stock_with_sock:
        return -20;
#endif

#if PRINTER_IS_PRUSA_MK3_5()
    case HotendType::e3d_revo:
        return 40;
#endif

#if HAS_HT_HOTEND()
    case HotendType::high_temp:
        // No offset: the HT hotend has no sock, and its selftest range is defined directly
        // by Config_HeaterNozzle_HighTemp rather than derived from the standard one.
        return 0;
#endif
    }

    // This shouldn't happen, but if it does, let the firmware continue.
    // Might be due to mis-migration in config-store.
    debug_assert(0);
    return 0;
}
