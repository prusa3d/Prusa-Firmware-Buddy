/**
 * @file DialogSelftestResult.hpp
 * @author Radek Vana
 * @brief Dialog to read selftest result from eeprom and and show it
 * @date 2020-12-05
 */

#pragma once

#include "IDialog.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "eeprom.h" // SelftestResultEEprom_t

class DialogSelftestResult : public AddSuperWindow<IDialog> {
    window_text_t text_fan_test;

    window_text_t text_hotend_fan;
    WindowIcon_OkNg icon_hotend_fan;
    window_text_t text_print_fan;
    WindowIcon_OkNg icon_print_fan;

    window_text_t text_checking_axis;

    window_text_t text_x_axis;
    WindowIcon_OkNg icon_x_axis;
    window_text_t text_y_axis;
    WindowIcon_OkNg icon_y_axis;
    window_text_t text_z_axis;
    WindowIcon_OkNg icon_z_axis;

    window_text_t text_checking_temp;

    window_text_t text_noz;
    WindowIcon_OkNg icon_noz;
    window_text_t text_bed;
    WindowIcon_OkNg icon_bed;

    DialogSelftestResult(SelftestResultEEprom_t result); // created by static Show method
protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show(SelftestResultEEprom_t result);
};
