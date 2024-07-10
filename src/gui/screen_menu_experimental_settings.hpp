/**
 * @file screen_menu_experimental_settings.hpp
 */

#pragma once
#include "config.h"

#ifndef _DEBUG
    #include "screen_menu_experimental_settings_release.hpp"
#else // _DEBUG
    #include "screen_menu_experimental_settings_debug.hpp"
#endif // _DEBUG

class ScreenMenuExperimentalSettings : public ScreenMenuExperimentalSettings__ {
    static constexpr const char *const save_and_reboot = N_("Do you want to save changes and reboot the printer?");
    constexpr static const char *label = N_("EXPERIMENTAL SETTINGS");

    ExperimentalSettingsValues initial;

    void clicked_return();

public:
    ScreenMenuExperimentalSettings();

    virtual void windowEvent(window_t *sender, GUI_event_t ev, void *param) override;
};
