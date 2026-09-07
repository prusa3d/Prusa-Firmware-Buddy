/// @file
#pragma once

#include <screen_menu.hpp>
#include <WindowMenuItems.hpp>
#include <WindowMenuSpin.hpp>
#include <WindowMenuSwitch.hpp>

class MI_LEFT_ALIGN_TEMP final : public MenuItemSwitch {
public:
    MI_LEFT_ALIGN_TEMP();

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;
};

class MI_SHOW_ZERO_TEMP_TARGET final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_SHOW_ZERO_TEMP_TARGET();
    virtual void OnChange(size_t) override;
};

class MI_FOOTER_CENTER_N final : public WiSpin {
public:
    MI_FOOTER_CENTER_N();
    virtual void OnClick() override;
};

using ScreenMenuAdvancedFooterSettingsBase = ScreenMenu<EFooter::On,
    MI_RETURN,
    MI_FOOTER_CENTER_N,
    MI_LEFT_ALIGN_TEMP,
    MI_SHOW_ZERO_TEMP_TARGET>;

class ScreenMenuAdvancedFooterSettings final : public ScreenMenuAdvancedFooterSettingsBase {
public:
    ScreenMenuAdvancedFooterSettings();
};
