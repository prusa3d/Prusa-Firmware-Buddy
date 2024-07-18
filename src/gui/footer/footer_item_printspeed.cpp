/**
 * @file footer_item_printspeed.cpp
 */

#include "footer_item_printspeed.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "i18n.h"
#include <algorithm>

FooterItemSpeed::FooterItemSpeed(window_t *parent)
    : FooterIconText_IntVal(parent, &img::speed_16x16, static_makeView, static_readValue) {
}

int FooterItemSpeed::static_readValue() {
    return marlin_vars().print_speed;
}

string_view_utf8 FooterItemSpeed::static_makeView(int value) {
    static char buff[5]; // max 999%
    int value_to_print = std::clamp(value, 1, 999);
    snprintf(buff, sizeof(buff), "%d%%", value_to_print);
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
