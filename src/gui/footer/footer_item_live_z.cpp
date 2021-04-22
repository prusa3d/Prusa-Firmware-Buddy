/**
 * @file footer_item_live_z.cpp
 * @author Radek Vana
 * @date 2021-04-17
 */

#include "footer_item_live_z.hpp"
#include "marlin_client.h"
#include "resource.h"
#include <algorithm>
#include <cmath>

FooterItemLiveZ::FooterItemLiveZ(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, IDR_PNG_z_axis_16px, static_makeView, static_readValue) {
}

int FooterItemLiveZ::static_readValue() {
    return std::lroundf(1000.f * marlin_vars()->z_offset); //store as fix point
}

string_view_utf8 FooterItemLiveZ::static_makeView(int value) {
    static std::array<char, 7> buff; //"-2" - "0", longest "-0.001"
    int value_clamped = std::clamp(value, -2000, 0);
    bool sign = std::signbit(value_clamped);
    uint uval = std::abs(value_clamped);

    //float - "%.3g" if giveing me warnings about buffer size, so I will print it manually
    auto ptr = buff.begin();
    if (sign) {
        *ptr = '-';
        ++ptr;
    }
    *ptr = '0' + uval / 1000;
    ++ptr;

    if (uval % 1000) {
        *ptr = '.';
        ++ptr;

        for (int modulo = 1000; modulo >= 10; modulo /= 10) {
            uint i = uval % modulo;
            if (i == 0)
                break;

            *ptr = '0' + i / (modulo / 10);
            ++ptr;
        }

        *ptr = '\0';
    }

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}
