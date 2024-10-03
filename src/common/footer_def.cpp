#include "footer_def.hpp"

#include <option/has_chamber_api.h>

#include <enum_array.hpp>

const char *footer::to_string(Item item) {
    static constexpr EnumArray<Item, const char *, Item::_count> texts {
        { Item::none, nullptr },
        { Item::nozzle, N_("Nozzle") },
        { Item::bed, N_("Bed") },
        { Item::filament, N_("Filament") },
        { Item::f_s_value, N_("FS Value") },
        { Item::f_sensor, N_("FSensor") },
        { Item::speed, N_("Speed") },
        { Item::axis_x, N_("X") },
        { Item::axis_y, N_("Y") },
        { Item::axis_z, N_("Z") },
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
        { Item::finda, HAS_MMU2() ? (const char *)N_("Finda") : nullptr },
        { Item::current_tool,
#if defined(FOOTER_HAS_TOOL_NR)
            N_("Current tool")
#else
            nullptr
#endif
        },
        { Item::all_nozzles,
#if defined(FOOTER_HAS_TOOL_NR)
            N_("All nozzles")
#else
            nullptr
#endif
        },
        { Item::f_sensor_side, HAS_SIDE_FSENSOR() ? (const char *)N_("FSensor side") : nullptr },
        { Item::nozzle_diameter, N_("Nozzle diameter") },
        { Item::nozzle_pwm, N_("Nozzle PWM") },
        { Item::chamber_temp, HAS_CHAMBER_API() ? (const char *)N_("Chamber temperature") : nullptr },
    };

    return texts.get_fallback(item, Item::none);
}
