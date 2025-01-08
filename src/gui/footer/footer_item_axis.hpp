/**
 * @file footer_item_axis.hpp
 * @author Radek Vana
 * @brief axis position related footer items
 * @date 2021-12-02
 */
#pragma once
#include "ifooter_item.hpp"
#include "menu_vars.h"
#include "marlin_client.hpp"

// XYZE position
template <size_t AXIS>
class FooterItemAxisPos : public FooterIconText_FloatVal {
    using buffer_t = std::array<char, 7>;
    static float static_readValue();
    static buffer_t buff;
    static string_view_utf8 static_makeViewIntoBuff(float value);

public:
    FooterItemAxisPos(window_t *parent, const img::Resource *icon)
        : FooterIconText_FloatVal(parent, icon, static_makeViewIntoBuff, static_readValue) {}
};

template <size_t AXIS>
typename FooterItemAxisPos<AXIS>::buffer_t FooterItemAxisPos<AXIS>::buff;

template <size_t AXIS>
string_view_utf8 FooterItemAxisPos<AXIS>::static_makeViewIntoBuff(float value) {
    int printed_chars = snprintf(buff.data(), buff.size(), "%.2f", (double)value);

    if (printed_chars < 1) {
        buff[0] = '\0';
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

template <size_t AXIS>
float FooterItemAxisPos<AXIS>::static_readValue() {
    const auto range = MenuVars::axis_range(AXIS);
    return std::clamp((float)marlin_vars().logical_pos[AXIS], (float)range.first, (float)range.second);
}

// Position according to gcode
template <size_t AXIS>
class FooterItemAxisCurrPos : public FooterIconText_FloatVal {
    using buffer_t = std::array<char, 7>;
    static float static_readValue();
    static buffer_t buff;
    static string_view_utf8 static_makeViewIntoBuff(float value);

public:
    FooterItemAxisCurrPos(window_t *parent, const img::Resource *icon)
        : FooterIconText_FloatVal(parent, icon, static_makeViewIntoBuff, static_readValue) {}
};

template <size_t AXIS>
typename FooterItemAxisCurrPos<AXIS>::buffer_t FooterItemAxisCurrPos<AXIS>::buff;

template <size_t AXIS>
string_view_utf8 FooterItemAxisCurrPos<AXIS>::static_makeViewIntoBuff(float value) {
    int printed_chars = snprintf(buff.data(), buff.size(), "%.2f", (double)value);

    if (printed_chars < 1) {
        buff[0] = '\0';
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}
template <size_t AXIS>
float FooterItemAxisCurrPos<AXIS>::static_readValue() {
    const auto range = MenuVars::axis_range(AXIS);
    return std::clamp((float)marlin_vars().logical_curr_pos[AXIS], (float)range.first, (float)range.second);
}

class FooterItemAxisX final : public FooterItemAxisPos<0> {
public:
    FooterItemAxisX(window_t *parent);
};
class FooterItemAxisY final : public FooterItemAxisPos<1> {
public:
    FooterItemAxisY(window_t *parent);
};

class FooterItemAxisZ final : public FooterItemAxisPos<2> {
public:
    FooterItemAxisZ(window_t *parent);
};

class FooterItemZHeight final : public FooterItemAxisCurrPos<2> {
public:
    FooterItemZHeight(window_t *parent);
};
