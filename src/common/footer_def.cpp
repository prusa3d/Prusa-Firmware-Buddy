#include "footer_def.hpp"

#include <option/has_chamber_api.h>

const char *footer::to_string(Item item) {
    switch (item) {
    case Item::nozzle:
        return N_("Nozzle");
    case Item::nozzle_diameter:
        return N_("Nozzle diameter");
    case Item::nozzle_pwm:
        return N_("Nozzle PWM");
    case Item::bed:
        return N_("Bed");
    case Item::filament:
        return N_("Filament");
    case Item::f_sensor:
        return N_("FSensor");
    case Item::f_s_value:
        return N_("FS Value");
    case Item::speed:
        return N_("Speed");
    case Item::axis_x:
        return N_("X");
    case Item::axis_y:
        return N_("Y");
    case Item::axis_z:
        return N_("Z");
    case Item::z_height:
        return N_("Z height");
    case Item::print_fan:
        return N_("Print fan");
    case Item::heatbreak_fan:
#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
        return N_("Hotend Fan");
#else
        return N_("Heatbreak Fan");
#endif
    case Item::input_shaper_x:
        return N_("Input Shaper X");
    case Item::input_shaper_y:
        return N_("Input Shaper Y");
    case Item::live_z:
#if defined(FOOTER_HAS_LIVE_Z)
        return N_("Live Z");
#else
        break;
#endif
    case Item::heatbreak_temp:
        return N_("Heatbreak");
    case Item::sheets:
#if HAS_SHEET_PROFILES()
        return N_("Sheets");
#else
        break;
#endif
    case Item::finda:
#if HAS_MMU2()
        return N_("Finda");
#else
        break;
#endif
    case Item::current_tool:
#if defined(FOOTER_HAS_TOOL_NR)
        return N_("Current tool");
#else
        break;
#endif
    case Item::all_nozzles:
#if defined(FOOTER_HAS_TOOL_NR)
        return N_("All nozzles");
#else
        break;
#endif
    case Item::f_sensor_side:
#if HAS_SIDE_FSENSOR()
        return N_("FSensor side");
#else
        break;
#endif /*HAS_SIDE_FSENSOR()*/
    case Item::chamber_temp:
#if HAS_CHAMBER_API()
        return N_("Chamber temperature");
#else
        break;
#endif
    case Item::none:
        return N_("None");
    case Item::_count:
        break;
    }
    bsod("Nonexistent footer item");
    return "";
}
