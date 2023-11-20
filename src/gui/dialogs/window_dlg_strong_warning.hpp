/**
 * @file window_dlg_strong_warning.hpp
 * @author Radek Vana
 * @brief Dialog to handle warnings from Marlin thread
 * @date 2020-11-06
 */

#pragma once

#include "IDialog.hpp"
#include "radio_button.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "i18n.h"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "img_resources.hpp"
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <bitset>

// Singleton dialog for messages
class window_dlg_strong_warning_t : public AddSuperWindow<IDialog> {
protected: // inherited by unit tests, must be protected
    static constexpr const char *Title = N_("INFO");
    static constexpr const char *HotendFanErrorMsg = N_("Hotend fan not spinning. Check it for possible debris, then inspect the wiring.");
    static constexpr const char *PrintFanErrorMsg = N_("Print fan not spinning. Check it for possible debris, then inspect the wiring.");
    static constexpr const char *TitleNozzle = N_("TEMP NOT MATCHING");
    static constexpr const char *HotendTempDiscrepancyMsg = N_("Measured temperature is not matching expected value. Check the thermistor is in contact with hotend. In case of damage, replace it.");
    static constexpr const char *HeaterTimeoutMsg = N_("Heating disabled due to 30 minutes of inactivity.");
#if _DEBUG
    static constexpr const char *SteppersTimeoutMsg = N_("Steppers disabled due to inactivity.");
#endif
    static constexpr const char *USBFlashDiskError = N_("USB drive or file error, the print is now paused. Reconnect the drive.");
#if ENABLED(POWER_PANIC)
    static constexpr const char *HeatbedColdAfterPPMsg = N_("The heatbed cooled down during the power outage, printed object might have detached. Inspect it before continuing.");
#endif
    static constexpr const char *HeatBreakThermistorFail = N_("Heatbreak thermistor is disconnected. Inspect the wiring.");
    static constexpr const char *NozzleDoesNotHaveRoundSectionMsg = N_("Nozzle doesn't seem to have round cross section. Make sure it is clean and perpendicular to the bed.");
    static constexpr const char *NotDownloadedMsg = N_("G-Code transfer running too slow. Check your network for issues or use different USB drive. Press Continue to resume printing.");
    static constexpr const char *BuddyMCUMaxTempMsg = N_("MCU in Buddy is overheated. Any higher will result in fatal error.");
#if HAS_DWARF()
    static constexpr const char *DwarfMCUMaxTempMsg = N_("MCU in Dwarf is overheated. Any higher will result in fatal error.");
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
    static constexpr const char *ModBedMCUMaxTempMsg = N_("MCU in Modular Bed is overheated. Any higher will result in fatal error.");
#endif /* HAS_MODULARBED() */

    struct icon_title_text_t {
        const img::Resource *icon;
        const char *title;
        const char *text;
    };

public:
    enum types {
        HotendFan,
        PrintFan,
        HotendTempDiscrepancy,
        HeatersTimeout,
#if _DEBUG
        SteppersTimeout,
#endif
        USBFlashDisk,
        HBThermistorFail,
#if ENABLED(POWER_PANIC)
        HeatbedColdAfterPP,
#endif
        NozzleDoesNotHaveRoundSection,
        NotDownloaded,
        BuddyMCUMaxTemp,
#if HAS_DWARF()
        DwarfMCUMaxTemp,
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
        ModBedMCUMaxTemp,
#endif /* HAS_MODULARBED() */
        count_
    };

protected:
    // order must match to enum types
    static constexpr icon_title_text_t icon_title_text[] = {
        { &img::fan_error_48x48, Title, HotendFanErrorMsg },
        { &img::fan_error_48x48, Title, PrintFanErrorMsg },
        { nullptr, TitleNozzle, HotendTempDiscrepancyMsg },
        { &img::exposure_times_48x48, Title, HeaterTimeoutMsg },
#if _DEBUG
        { &img::exposure_times_48x48, Title, SteppersTimeoutMsg },
#endif
        { &img::usb_error_48x48, Title, USBFlashDiskError },
        { nullptr, Title, HeatBreakThermistorFail }, // TODO need icon for heatbreak thermistor disconnect
#if ENABLED(POWER_PANIC)
        { nullptr, Title, HeatbedColdAfterPPMsg },
#endif
        { &img::nozzle_34x32, Title, NozzleDoesNotHaveRoundSectionMsg },
        { &img::no_stream_48x48, Title, NotDownloadedMsg }, // NotDownloaded
        { &img::warning_48x48, Title, BuddyMCUMaxTempMsg }, // BuddyMCUMaxTemp
#if HAS_DWARF()
        { &img::warning_48x48, Title, DwarfMCUMaxTempMsg }, // DwarfMCUMaxTemp
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
        { &img::warning_48x48, Title, ModBedMCUMaxTempMsg }, // ModBedMCUMaxTemp
#endif /* HAS_MODULARBED() */
    };
    static_assert(std::size(icon_title_text) == types::count_);

    static std::bitset<types::count_> shown; // mask of all "active" dialogs
    static types on_top; // one of shown dialogs - on top == this one is visible

    static window_dlg_strong_warning_t &Instance() {
        static window_dlg_strong_warning_t ret;
        return ret;
    }

    window_header_t header;
    StatusFooter footer;

    window_icon_t icon;
    window_text_t text;
    RadioButton button;

    window_dlg_strong_warning_t();
    window_dlg_strong_warning_t(const window_dlg_strong_warning_t &) = delete;

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    void show(types type);
    void screenJump();
    void setIcon(const img::Resource *icon);
    void adjustLayout();
    void setWarningText(types type);

public:
    static void ScreenJumpCheck();

    /**
     * @brief Show warning dialog screen.
     * @param type which dialog to show
     */
    static void ShowType(types type) {
        Instance().show(type);
    }
};
