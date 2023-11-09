/**
 * @file footer_item_input_shaper.cpp
 */

#include "footer_item_input_shaper.hpp"
#include "img_resources.hpp"

static constexpr const char *const str_disabled = N_("Disabled");

static const char *to_short_string(input_shaper::Type type) {
    switch (type) {
    case input_shaper::Type::zv:
        return "ZV";
    case input_shaper::Type::zvd:
        return "ZVD";
    case input_shaper::Type::mzv:
        return "MZV";
    case input_shaper::Type::ei:
        return "EI";
    case input_shaper::Type::ei_2hump:
        return "EI2";
    case input_shaper::Type::ei_3hump:
        return "EI3";
    default:
        break;
    }
    return "Unknown";
}

FooterItemInputShaperX::FooterItemInputShaperX(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &img::x_axis_16x16, static_makeViewIntoBuff, static_readValue) {}

typename FooterItemInputShaperX::buffer_t FooterItemInputShaperX::buff;

int FooterItemInputShaperX::static_readValue() {
    const auto &axis_config = input_shaper::current_config().axis[X_AXIS];
    if (axis_config) {
        return static_cast<int>(axis_config->frequency);
    } else {
        return 0;
    }
}

string_view_utf8 FooterItemInputShaperX::static_makeViewIntoBuff(int value) {
    const auto &axis_config = input_shaper::current_config().axis[X_AXIS];
    if (axis_config) {
        // FORMAT: [ZVM 100 Hz] == 11 chars, null-terminator included
        int printed_chars = snprintf(buff.data(), buff.size(), "%s %d Hz", to_short_string(axis_config->type), value);
        if (printed_chars < 1) {
            buff[0] = '\0';
        }
        return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
    } else {
        return _(str_disabled);
    }
}

FooterItemInputShaperY::FooterItemInputShaperY(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &img::y_axis_16x16, static_makeViewIntoBuff, static_readValue) {}

typename FooterItemInputShaperY::buffer_t FooterItemInputShaperY::buff;

int FooterItemInputShaperY::static_readValue() {
    const auto &axis_config = input_shaper::current_config().axis[Y_AXIS];
    if (axis_config) {
        return static_cast<int>(axis_config->frequency);
    } else {
        return 0;
    }
}

string_view_utf8 FooterItemInputShaperY::static_makeViewIntoBuff(int value) {
    const auto &axis_config = input_shaper::current_config().axis[Y_AXIS];
    if (axis_config) {
        // FORMAT: [ZVM 100 Hz] == 11 chars, null-terminator included
        int printed_chars = snprintf(buff.data(), buff.size(), "%s %d Hz", to_short_string(axis_config->type), value);
        if (printed_chars < 1) {
            buff[0] = '\0';
        }
        return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
    } else {
        return _(str_disabled);
    }
}
