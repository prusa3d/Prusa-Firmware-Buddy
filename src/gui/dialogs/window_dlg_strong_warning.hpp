/**
 * @file window_dlg_strong_warning.hpp
 * @author Radek Vana
 * @brief Dialog to handle warnings from Marlin thread
 * @date 2020-11-06
 */

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "i18n.h"

//Singleton dialog for messages
class window_dlg_strong_warning_t : public AddSuperWindow<IDialog> {
    window_text_t text;

    window_dlg_strong_warning_t();
    window_dlg_strong_warning_t(const window_dlg_strong_warning_t &) = delete;

    static constexpr const char *HotendFanErrorMsg = N_("Print fan not spinning");
    static constexpr const char *PrintFanErrorMsg = N_("Hotend fan not spinning");
    static constexpr const char *HeaterTimeoutMsg = N_("Heating disabled due to a heaters timeout.");
    static constexpr const char *USBFlashDiskError = N_("USB drive error. Print paused.");

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    void show(string_view_utf8 txt); // could use const char *, but with stringview I can pass both translated and not translated texts

public:
    static void ShowHotendFan();
    static void ShowPrintFan();
    static void ShowHeaterTimeout();
    static void ShowUSBFlashDisk();
};
