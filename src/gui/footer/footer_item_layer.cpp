/**
 * @file footer_item_layer.cpp
 */

#include "footer_item_layer.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include "i18n.h"
#include <algorithm>
#include <cmath>
#include <gcode_info.hpp>
#include "menu_vars.h"

FooterItemCurrentLayer::FooterItemCurrentLayer(window_t *parent)
    : FooterIconText_IntVal(parent, &img::layers_dark_16x14, static_makeView, static_readValue) {
}

int FooterItemCurrentLayer::static_readValue() {
    const auto range = MenuVars::axis_range(2);
    const auto curr_axis_pos = std::clamp((float)marlin_vars().logical_curr_pos[2], (float)range.first, (float)range.second);
    const auto z_offset = std::clamp((float)marlin_vars().z_offset, (float)range.first, (float)range.second);
    const auto extruder_info = GCodeInfo::getInstance().get_extruder_info(marlin_vars().active_extruder.get());
    const auto layer_height = extruder_info.layer_height.value_or(0);
    const auto first_layer_height = extruder_info.first_layer_height.value_or(0);
    auto cur_z_pos = curr_axis_pos - z_offset;
    if (first_layer_height > 0) {
        if (cur_z_pos > first_layer_height) {
            return (int)((cur_z_pos - first_layer_height) / layer_height) + 1;
        } else {
            return 0 ? (cur_z_pos < first_layer_height) : (int)(cur_z_pos / first_layer_height);
        }
    }
    return 0 ? (layer_height == 0) : (int)(cur_z_pos / layer_height);
}

string_view_utf8 FooterItemCurrentLayer::static_makeView(int value) {
    static std::array<char, 7> buff;

    int printed_chars = snprintf(buff.data(), buff.size(), "%i", value);

    if (printed_chars < 1) {
        buff[0] = '\0';
    }

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}
