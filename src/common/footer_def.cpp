#include "footer_def.hpp"

#include <option/has_chamber_api.h>
#include <option/has_mmu2.h>
#include <option/has_wastebin_fill_tracking.h>

#include <utils/enum_array.hpp>

const char *footer::to_string(Item item) {
    static constexpr EnumArray<Item, const char *, Item::_count> texts {
        { Item::none, N_("None") },
        { Item::nozzle, N_("Nozzle") },
        { Item::bed, N_("Bed") },
        { Item::filament, N_("Filament") },
        { Item::f_s_value, FILAMENT_SENSOR_IS_NO() ? nullptr : (const char *)N_("FS Value") },
        { Item::f_sensor, FILAMENT_SENSOR_IS_NO() ? nullptr : (const char *)N_("FSensor") },
        { Item::speed, N_("Speed") },
        { Item::axis_x, "X" },
        { Item::axis_y, "Y" },
        { Item::axis_z, "Z" },
        { Item::z_height, N_("Z height") },
        { Item::print_fan, N_("Print fan") },
        { Item::heatbreak_fan, PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI() ? N_("Hotend Fan") : N_("Heatbreak Fan") },
        { Item::input_shaper_x, N_("Input Shaper X") },
        { Item::input_shaper_y, N_("Input Shaper Y") },
        { Item::live_z,
#if defined(FOOTER_HAS_LIVE_Z)
            N_("Live Z")
#else
            nullptr
#endif
        },
        { Item::heatbreak_temp, N_("Heatbreak") },
        { Item::sheets, HAS_SHEET_PROFILES() ? (const char *)N_("Sheets") : nullptr },
        { Item::finda, HAS_MMU2() ? (const char *)"Finda" : nullptr },
        { Item::current_tool,
#if FOOTER_HAS_TOOL_NR()
            N_("Current tool")
#else
            nullptr
#endif
        },
        { Item::all_nozzles,
#if HAS_PER_TOOL_TEMPERATURES()
            N_("All nozzles")
#else
            nullptr
#endif
        },
        { Item::f_sensor_side, HAS_SIDE_FSENSOR() ? (const char *)N_("FSensor side") : nullptr },
        { Item::nozzle_diameter, N_("Nozzle diameter") },
        { Item::nozzle_pwm, N_("Nozzle PWM") },
        {
            Item::chamber_temp,
            PRINTER_IS_PRUSA_XL() ? (const char *)N_("Enclosure temperature") : //
                HAS_CHAMBER_API() ? (const char *)N_("Chamber temperature")
                                  : nullptr //
        },
        { Item::f_s_value_side,
#if HAS_SIDE_FSENSOR()
            N_("FS Value Side")
#else
            nullptr
#endif
        },
        { Item::wastebin_pellets,
#if HAS_WASTEBIN_FILL_TRACKING()
            N_("Nozzle Cleaner")
#else
            nullptr
#endif
        },
        { Item::nozzle_power, HAS_INDX() ? (const char *)N_("Nozzle Power") : nullptr },
    };

    return texts.get_fallback(item, Item::none);
}
