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

    // until these texts become final, do not mark them for translation
    static constexpr const char *HotendFanErrorMsg = ("I am dummy HotendFanErrorMsg, I need to be replaced with something else ... ");
    static constexpr const char *PrintFanErrorMsg = ("I am dummy PrintFanError, I need to be replaced with something else ... ");
    static constexpr const char *HeaterTimeoutMsg = ("I am dummy HeaterTimeout, I need to be replaced with something else ... ");
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
