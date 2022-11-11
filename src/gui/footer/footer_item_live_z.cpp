/**
 * @file footer_item_live_z.cpp
 */

#include "footer_item_live_z.hpp"
#include "marlin_client.h"
#include "png_resources.hpp"
#include "i18n.h"
#include <algorithm>
#include <cmath>

FooterItemLiveZ::FooterItemLiveZ(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::z_axis_16x16, static_makeView, static_readValue) {
}

int FooterItemLiveZ::static_readValue() {
    return std::lroundf(1000.f * marlin_vars()->z_offset); //store as fix point
}

string_view_utf8 FooterItemLiveZ::static_makeView(int value) {
    static std::array<char, 7> buff; //"-2" - "0", longest "-0.001"
    int value_clamped = std::clamp(value, -2000, 0);

    int printed_chars = snprintf(buff.data(), buff.size(), "%i.%.3u", value_clamped / 1000, std::abs(value_clamped) % 1000);

    if (printed_chars < 1) {
        buff[0] = '\0';
    } else {
        //dont want it to erase last in 0.0, -1.0, -2.0
        while ((--printed_chars) > 2 && buff[printed_chars] == '0' && buff[printed_chars - 1] != '.') {
            buff[printed_chars] = '\0';
        }
    }

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

string_view_utf8 FooterItemLiveZ::GetName() { return _("Live Z"); }
