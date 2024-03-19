#pragma once

#include "IDialogMarlin.hpp"
#include "client_response.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "radio_button_fsm.hpp"
#include "img_resources.hpp"
#include <find_error.hpp>

#include <option/has_dwarf.h>
#include <option/has_modularbed.h>

static_assert(sizeof(fsm::PhaseData) == sizeof(WarningType), "If this does not hold, we need to revise how we send the type through teh fsm machinery.");
class DialogWarning : public AddSuperWindow<IDialogMarlin> {
    window_icon_t icon;
    window_text_t text;
    RadioButtonFsm<PhasesWarning> button;

    // TODO unify this with WarningType, so we don't have to do the conversion??
    enum types : uint8_t {
        HotendFan = 0,
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
#if ENABLED(CALIBRATION_GCODE)
        NozzleDoesNotHaveRoundSection,
#endif
        NotDownloaded,
        BuddyMCUMaxTemp,
#if HAS_BED_PROBE
        ProbingFailed,
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        NozzleCleaningFailed,
#endif
#if HAS_DWARF()
        DwarfMCUMaxTemp,
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
        ModBedMCUMaxTemp,
#endif /* HAS_MODULARBED() */
#if XL_ENCLOSURE_SUPPORT()
        EnclosureFilterExpirWarning,
        EnclosureFilterExpiration,
        EnclosureFanError,
#endif /* XLBUDDY BOARD */
        count_
    };

    types get_type(fsm::BaseData data);

    struct icon_code {
        const img::Resource *icon;
        ErrCode code;
    };
    static constexpr icon_code icon_code[] = {
        { &img::fan_error_48x48, ErrCode::CONNECT_HOTEND_FAN_ERROR },
        { &img::fan_error_48x48, ErrCode::CONNECT_PRINT_FAN_ERROR },
        { nullptr, ErrCode::CONNECT_HOTEND_TEMP_DISCREPANCY },
        { &img::exposure_times_48x48, ErrCode::CONNECT_HEATERS_TIMEOUT },
#if _DEBUG
        { &img::exposure_times_48x48, ErrCode::CONNECT_STEPPERS_TIMEOUT },
#endif
        { &img::usb_error_48x48, ErrCode::CONNECT_USB_FLASH_DISK_ERROR },
        { nullptr, ErrCode::CONNECT_HEATBREAK_THERMISTOR_FAIL }, // TODO need icon for heatbreak thermistor disconnect
#if ENABLED(POWER_PANIC)
        { nullptr, ErrCode::CONNECT_POWER_PANIC_COLD_BED },
#endif
#if ENABLED(CALIBRATION_GCODE)
        { &img::nozzle_34x32, ErrCode::CONNECT_NOZZLE_DOES_NOT_HAVE_ROUND_SECTION },
#endif
#if defined(USE_ILI9488)
        { &img::no_stream_48x48, ErrCode::CONNECT_NOT_DOWNLOADED }, // NotDownloaded
#else
        { nullptr, ErrCode::CONNECT_NOT_DOWNLOADED }, // NotDownloaded -- Text is too long - ST7789 has to have no icon
#endif
        { &img::warning_48x48, ErrCode::CONNECT_BUDDY_MCU_MAX_TEMP }, // BuddyMCUMaxTemp
#if HAS_BED_PROBE
        { nullptr, ErrCode::CONNECT_PROBING_FAILED },
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
        { nullptr, ErrCode::CONNECT_NOZZLE_CLEANING_FAILED },
#endif
#if HAS_DWARF()
        { &img::warning_48x48, ErrCode::CONNECT_DWARF_MCU_MAX_TEMP }, // DwarfMCUMaxTemp
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
        { &img::warning_48x48, ErrCode::CONNECT_MOD_BED_MCU_MAX_TEMP }, // ModBedMCUMaxTemp
#endif /* HAS_MODULARBED() */
#if XL_ENCLOSURE_SUPPORT()
        { &img::warning_48x48, ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION_WARNING },
        { &img::warning_48x48, ErrCode::CONNECT_ENCLOSURE_FILTER_EXPIRATION },
        { &img::warning_48x48, ErrCode::CONNECT_ENCLOSURE_FAN_ERROR },
#endif /* XLBUDDY BOARD */
    };
    static_assert(std::size(icon_code) == types::count_);

public:
    DialogWarning(fsm::BaseData);
};
