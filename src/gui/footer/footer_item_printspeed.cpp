/**
 * @file footer_item_printspeed.cpp
 * @author Radek Vana
 * @date 2021-04-16
 */

#include "footer_item_printspeed.hpp"
#include "display_helper.h" // font_meas_text
#include "resource.h"       // IDR_PNG_speed_16px
#include "marlin_client.h"
#include <algorithm>

FooterItemSpeed::FooterItemSpeed(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, IDR_PNG_speed_16px, static_makeView, static_readValue) {
}

int FooterItemSpeed::static_readValue() {
    return marlin_vars()->print_speed;
}

string_view_utf8 FooterItemSpeed::static_makeView(int value) {
    static char buff[5]; //max 999%
    int value_to_print = std::clamp(value, 1, 999);
    snprintf(buff, sizeof(buff), "%d%%", value_to_print);
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
