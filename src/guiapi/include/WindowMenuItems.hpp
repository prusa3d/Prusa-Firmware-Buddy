/**
 * @file WindowMenuItems.hpp
 * @author Radek Vana
 * @brief Some commonly used menu items (to be inherited)
 * @date 2020-11-09
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "WindowMenuLabel.hpp"
#include "WindowMenuSpin.hpp"
#include "WindowMenuSwitch.hpp"
#include "WindowMenuInfo.hpp"

// not translated
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

    WI_SWITCH_0_1_NA_t(state_t index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_0), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_1), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_NA)) {}
};

class WI_ICON_SWITCH_OFF_ON_t : public WI_ICON_SWITCH_t<2> {
public:
    WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden);
};

class MI_RETURN : public WI_LABEL_t {
public:
    static constexpr const char *label = N_("Return");

    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EXIT : public WI_LABEL_t {
public:
    static constexpr const char *label { N_("Exit") };
    MI_EXIT();

protected:
    void click(IWindowMenu &window_menu) override;
};

class MI_TEST_DISABLED_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Disabled RETURN Button";

public:
    MI_TEST_DISABLED_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
