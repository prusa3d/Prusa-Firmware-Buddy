/**
 * @file window_dlg_strong_warning.hpp
 * @author Radek Vana
 * @brief Dialog to handle warnings from Marlin thread
 * @date 2020-11-06
 */

#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "i18n.h"
#include "window_header.hpp"
#include "status_footer.hpp"

//Singleton dialog for messages
class window_dlg_strong_warning_t : public AddSuperWindow<IDialog> {
protected: // inherited by unit tests, must be protected
    window_header_t header;
    StatusFooter footer;

    window_icon_t icon;
    window_text_t text;
    RadioButton button;

    window_dlg_strong_warning_t();
    window_dlg_strong_warning_t(const window_dlg_strong_warning_t &) = delete;

    static constexpr const char *Title = N_("INFO");
    static constexpr const char *HotendFanErrorMsg = N_("Hotend fan not spinning. Check it for possible debris, then inspect the wiring.");
    static constexpr const char *PrintFanErrorMsg = N_("Print fan not spinning. Check it for possible debris, then inspect the wiring.");
    static constexpr const char *TitleNozzle = N_("TEMP NOT MATCHING");
    static constexpr const char *HotendTempDiscrepancyMsg = N_("Measured temperature is not matching expected value. Check the thermistor is in contact with hotend. In case of damage, replace it.");
    static constexpr const char *HeaterTimeoutMsg = N_("Heating disabled due to 30 minutes of inactivity.");
    static constexpr const char *USBFlashDiskError = N_("USB drive error, the print is now paused. Reconnect the drive.");

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    void show(string_view_utf8 txt); // could use const char *, but with stringview I can pass both translated and not translated texts
    void setIcon(int16_t resId);

public:
    static void ShowHotendFan();
    static void ShowPrintFan();
    static void ShowHotendTempDiscrepancy();
    static void ShowHeatersTimeout();
    static void ShowUSBFlashDisk();
};
