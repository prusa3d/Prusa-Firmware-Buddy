#include "footer_item_z_heigth.hpp"
#include "marlin_client.h"
#include "resource.h"
#include <algorithm>
#include <cmath>
#include "menu_vars.h"

string_view_utf8 FooterItemZHeigth::static_makeView(float value) {
    static std::array<char, 7> buff; // the buffer must be static, because string_view does not copy data inside itself
    float value_clamped = std::clamp((float)value, (float)MenuVars::GetMaximumZRange()[0], (float)MenuVars::GetMaximumZRange()[1]);
    int printed_chars = snprintf(buff.data(), buff.size(), "%.2f", (double)value_clamped);

    if (printed_chars < 1) {
        buff[0] = '\0';
    } else {
        while (((--printed_chars) > 2) && (buff[printed_chars] == '0') && (buff[printed_chars - 1] != '.')) {
            buff[printed_chars] = '\0';
        }
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}
FooterItemZHeigth::FooterItemZHeigth(window_t *parent)
    : AddSuperWindow<FooterIconText_FloatVal>(parent, IDR_PNG_z_axis_16px, static_makeView, static_readValue) {
}
float FooterItemZHeigth::static_readValue() {
    return marlin_vars()->curr_pos[2];
}
