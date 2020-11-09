/**
 * @file WindowMenuItems.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-09
 *
 * @copyright Copyright (c) 2020
 *
 */

#pragma once

#include "IWindowMenuItem.hpp"
#include "WindowMenuLabel.hpp"
#include "WindowMenuSpin.hpp"
#include "WindowMenuSwitch.hpp"

// most common version of WI_SWITCH with on/off options
// also very nice how-to-use example
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, _(str_Off), _(str_On)) {}
};

class WI_ICON_SWITCH_OFF_ON_t : public WI_ICON_SWITCH_t<2> {
    constexpr static const uint16_t iid_off = IDR_PNG_switch_off_36px;
    constexpr static const uint16_t iid_on = IDR_PNG_switch_on_36px;

public:
    WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_ICON_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, iid_off, iid_on) {}
};

class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Return");

public:
    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

class MI_TEST_DISABLED_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Disabled RETURN button";

public:
    MI_TEST_DISABLED_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};
