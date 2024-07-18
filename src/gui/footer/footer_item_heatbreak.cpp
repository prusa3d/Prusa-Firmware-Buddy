/**
 * @file footer_item_heatbreak.cpp
 */

#include "footer_item_heatbreak.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "i18n.h"
#include <algorithm>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

FooterItemHeatBreak::FooterItemHeatBreak(window_t *parent)
    : FooterIconText_IntVal(parent, &img::heatbreak_dark_16x16, static_makeView, static_readValue) {
}

/**
 * @brief Encode heatbreak temperature to tenths of °C
 *
 * @return int - tenths of °C, or negative min if invalid
 */
int FooterItemHeatBreak::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars().active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/

    return int(marlin_vars().active_hotend().temp_heatbreak * 10.F);
}

/**
 * @brief Create stringview from int value
 *
 * @param value tenths of °C, or negative min if invalid
 * @return string_view_utf8
 */
string_view_utf8 FooterItemHeatBreak::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    static constexpr bool print_tenths = false; // change if needed better precision
    static constexpr const char *str_with_tenths = "%u.%u\xC2\xB0\x43";
    static constexpr const char *str = "%u\xC2\xB0\x43";
    static char buff[9]; // max "999.9°C" / "999°C"
    uint value_to_print = std::clamp(value, 0, 9999);
    uint tenths = value_to_print % 10;
    value_to_print = (value_to_print / 10) % 1000;
    if (print_tenths) {
        snprintf(buff, sizeof(buff), str_with_tenths, value_to_print, tenths);
    } else {
        snprintf(buff, sizeof(buff), str, value_to_print);
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
