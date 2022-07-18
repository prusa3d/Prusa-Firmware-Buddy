/**
* @file footer_item_axis.hpp
* @author Radek Vana
* @brief axis position related footer items
* @date 2021-12-02
*/
#pragma once
#include "ifooter_item.hpp"
#include "menu_vars.h"
#include "marlin_client.h"
#include "resource.h"

/**
* @brief parent for X, Y, Z footer items
*/
class IFooterItemAxis : public AddSuperWindow<FooterIconText_FloatVal> {
    using buffer_t = std::array<char, 7>;

protected:
    static buffer_t buff;
    static string_view_utf8 static_makeViewIntoBuff(float value);

public:
    IFooterItemAxis(window_t *parent, ResourceId icon_id, reader_cb value_reader);
};

// XYZE position
template <size_t AXIS>
class FooterItemAxisPos : public IFooterItemAxis {
    static float static_readValue();

public:
    FooterItemAxisPos(window_t *parent, ResourceId icon_id)
        : IFooterItemAxis(parent, icon_id, static_readValue) {}
};

template <size_t AXIS>
float FooterItemAxisPos<AXIS>::static_readValue() {
    return std::clamp((float)marlin_vars()->pos[AXIS], (float)MenuVars::GetAxisRanges()[AXIS][0], (float)MenuVars::GetAxisRanges()[AXIS][1]);
}

// Position according to gcode
template <size_t AXIS>
class FooterItemAxisCurrPos : public IFooterItemAxis {
    static float static_readValue();

public:
    FooterItemAxisCurrPos(window_t *parent, ResourceId icon_id)
        : IFooterItemAxis(parent, icon_id, static_readValue) {}
};

template <size_t AXIS>
float FooterItemAxisCurrPos<AXIS>::static_readValue() {
    return std::clamp((float)marlin_vars()->curr_pos[AXIS], (float)MenuVars::GetAxisRanges()[AXIS][0], (float)MenuVars::GetAxisRanges()[AXIS][1]);
}

class FooterItemAxisX : FooterItemAxisPos<0> {
public:
    static string_view_utf8 GetName() { return _("X Axis"); }
    FooterItemAxisX(window_t *parent)
        : FooterItemAxisPos<0>(parent, IDR_PNG_x_axis_16x16) {}
};
class FooterItemAxisY : FooterItemAxisPos<1> {
public:
    static string_view_utf8 GetName() { return _("Y Axis"); }
    FooterItemAxisY(window_t *parent)
        : FooterItemAxisPos<1>(parent, IDR_PNG_y_axis_16x16) {}
};

class FooterItemAxisZ : FooterItemAxisPos<2> {
public:
    static string_view_utf8 GetName() { return _("Z Axis"); }

    FooterItemAxisZ(window_t *parent)
        : FooterItemAxisPos<2>(parent, IDR_PNG_z_axis_16x16) {}
};

class FooterItemZHeight : FooterItemAxisCurrPos<2> {
public:
    static string_view_utf8 GetName() { return _("Z Heigth"); }

    FooterItemZHeight(window_t *parent)
        : FooterItemAxisCurrPos<2>(parent, IDR_PNG_z_axis_16x16) {}
};
