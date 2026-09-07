/// @file
#pragma once

#include <option/has_extra_experimental_settings.h>

#include <screen_menu.hpp>
#include <MItem_menus.hpp>
#include <MItem_experimental_tools.hpp>
#include <config.h>

#if HAS_LOADCELL()
    #include <MItem_loadcell.hpp>
#endif

using ScreenMenuExperimentalSettings__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_SAVE_AND_RETURN,
#if PRINTER_IS_PRUSA_MK3_5()
    MI_ALT_FAN,
#endif
    MI_Z_AXIS_LEN,
    MI_RESET_Z_AXIS_LEN,
#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    MI_STEPS_PER_UNIT_X,
    MI_STEPS_PER_UNIT_Y,
    MI_STEPS_PER_UNIT_Z,
#endif
    MI_STEPS_PER_UNIT_E,
    MI_RESET_STEPS_PER_UNIT,
#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    MI_DIRECTION_X,
    MI_DIRECTION_Y,
    MI_DIRECTION_Z,
#endif
    MI_DIRECTION_E,
    MI_RESET_DIRECTION,
#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
    MI_CURRENT_X,
    MI_CURRENT_Y,
    MI_CURRENT_Z,
    MI_CURRENT_E,
    MI_RESET_CURRENTS,
#endif
    MI_SERIAL_PRINTING_SCREEN_ENABLE
#if HAS_ILI9488_DISPLAY()
    ,
    MI_FAST_DRAW_ENABLE
#endif
#if HAS_LOADCELL()
    ,
    MI_LOADCELL_SCALE
#endif
    >;

class ScreenMenuExperimentalSettings : public ScreenMenuExperimentalSettings__ {
    static constexpr const char *const save_and_reboot = N_("Do you want to save changes and reboot the printer?");
    constexpr static const char *label = "EXPERIMENTAL SETTINGS";

    void clicked_return();

public:
    ScreenMenuExperimentalSettings();

    virtual void windowEvent(window_t *sender, GUI_event_t ev, void *param) override;
};
