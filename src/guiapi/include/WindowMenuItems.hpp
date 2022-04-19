/**
 * @file WindowMenuItems.hpp
 * @author Radek Vana
 * @brief Some commonly used menu items (to be inherited)
 * @date 2020-11-09
 */

#pragma once

#include "IWindowMenuItem.hpp"
#include "WindowMenuLabel.hpp"
#include "WindowMenuSpin.hpp"
#include "WindowMenuSwitch.hpp"
#include "WindowMenuInfo.hpp"
#include "resource.h"

// most common version of WI_SWITCH with on/off options
// also very nice how-to-use example
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, _(str_Off), _(str_On)) {}
};

//not translated
class WI_SWITCH_0_1_NA_t : public WI_SWITCH_t<3> {
    constexpr static const char *str_0 = "0";
    constexpr static const char *str_1 = "1";
    constexpr static const char *str_NA = "N/A";

public:
    enum class state_t {
        low,
        high,
        unknown
    };

    WI_SWITCH_0_1_NA_t(state_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_0), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_1), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_NA)) {}
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
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_DISABLED_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Disabled RETURN button";

public:
    MI_TEST_DISABLED_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
