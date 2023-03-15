/**
 * @file footer_item_heatbreak.cpp
 */

#include "footer_item_heatbreak.hpp"
#include "png_resources.hpp"
#include "marlin_client.hpp"
#include "i18n.h"
#include <algorithm>

FooterItemHeatBreak::FooterItemHeatBreak(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::heatbreak_dark_16x16, static_makeView, static_readValue) {
}

/**
 * @brief Encode heatbreak temperature to tenths of °C
 *
 * @return int - tenths of °C
 */
int FooterItemHeatBreak::static_readValue() {
    return int(marlin_vars()->active_hotend().temp_heatbreak * 10.F);
}

/**
 * @brief Create stringview from int value
 *
 * @param value tenths of °C
 * @return string_view_utf8
 */
string_view_utf8 FooterItemHeatBreak::static_makeView(int value) {
    static constexpr bool print_tenths = false; //change if needed better precision
    static constexpr const char *str_with_tenths = "%u.%u\177C";
    static constexpr const char *str = "%u\177C";
    static char buff[print_tenths ? 8 : 6]; //max "999.9°C" / "999°C"
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

string_view_utf8 FooterItemHeatBreak::GetName() { return _("Heatbreak"); }
