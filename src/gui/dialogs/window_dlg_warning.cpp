#include "window_dlg_warning.hpp"

#include <common/fsm_base_types.hpp>
#include <state/printer_state.hpp>

static constexpr int16_t iconSize = 48;

#if HAS_MINI_DISPLAY() || HAS_MOCK_DISPLAY()
static constexpr uint16_t side_padding = 16;
static constexpr uint16_t top_padding = 40;
static constexpr Rect16 textRect = { side_padding, top_padding + iconSize + 8, GuiDefaults::ScreenWidth - side_padding * 2, 170 - iconSize - 8 };
static constexpr Rect16 textRectNoIcon = { side_padding, top_padding, GuiDefaults::ScreenWidth - side_padding * 2, 170 };
static constexpr Rect16 iconRect = Rect16(GuiDefaults::ScreenWidth / 2 - iconSize / 2, top_padding, iconSize, iconSize);
#else
static constexpr uint16_t side_padding = 70;
static constexpr uint16_t top_padding = 90;
static constexpr Rect16 textRect = Rect16(side_padding + iconSize + 15, top_padding, 300, GuiDefaults::ScreenHeight - top_padding - GuiDefaults::ButtonHeight);
static constexpr Rect16 iconRect = Rect16(side_padding, top_padding, iconSize, iconSize);
#endif

const img::Resource *warning_dialog_icon(WarningType warning_type) {
    static constexpr EnumArray<WarningType, const img::Resource *, static_cast<int>(WarningType::_last) + 1> data {
        { WarningType::HotendFanError, &img::fan_error_48x48 },
            { WarningType::PrintFanError, &img::fan_error_48x48 },
            { WarningType::HeatersTimeout, &img::exposure_times_48x48 },
            { WarningType::HotendTempDiscrepancy, nullptr },
            { WarningType::NozzleTimeout, &img::exposure_times_48x48 },
#if _DEBUG
            { WarningType::SteppersTimeout, &img::exposure_times_48x48 },
#endif
            { WarningType::USBFlashDiskError, &img::usb_error_48x48 },
#if ENABLED(POWER_PANIC)
            { WarningType::HeatbedColdAfterPP, nullptr },
#endif
            { WarningType::HeatBreakThermistorFail, nullptr }, // TODO need icon for heatbreak thermistor disconnect
#if ENABLED(CALIBRATION_GCODE)
            { WarningType::NozzleDoesNotHaveRoundSection, &img::nozzle_34x32 },
#endif
            { WarningType::BuddyMCUMaxTemp, &img::warning_48x48 },
#if HAS_DWARF()
            { WarningType::DwarfMCUMaxTemp, &img::warning_48x48 },
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
            { WarningType::ModBedMCUMaxTemp, &img::warning_48x48 },
#endif /* HAS_MODULARBED() */
#if HAS_BED_PROBE
            { WarningType::ProbingFailed, nullptr },
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
            { WarningType::NozzleCleaningFailed, nullptr },
#endif
#if XL_ENCLOSURE_SUPPORT()
            { WarningType::EnclosureFilterExpirWarning, &img::warning_48x48 },
            { WarningType::EnclosureFilterExpiration, &img::warning_48x48 },
            { WarningType::EnclosureFanError, &img::warning_48x48 },
#endif /* XLBUDDY BOARD */
            { WarningType::NotDownloaded, HAS_LARGE_DISPLAY() ? &img::no_stream_48x48 : nullptr }, // Long text, does not fit on the mini display with the icon
            { WarningType::GcodeCorruption, &img::warning_48x48 },
            { WarningType::GcodeCropped, &img::warning_48x48 },
            { WarningType::MetricsConfigChangePrompt, &img::warning_48x48 },
    };
    return data[warning_type];
}

DialogWarning::DialogWarning(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreenBody)
    , icon(this, iconRect, &img::warning_48x48)
    , text(this, textRect, is_multiline::yes, is_closed_on_click_t::yes, {})
    , button(this, GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody), GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase())) {
    CaptureNormalWindow(button);

    const auto warning_type = static_cast<WarningType>(*data.GetData().data());
    const auto error_code = printer_state::warning_type_to_error_code(warning_type);
    const ErrDesc &error_desc = find_error(error_code);
    text.SetText(_(error_desc.err_text));

    if (auto icon_res = warning_dialog_icon(warning_type)) {
        icon.SetRes(icon_res);
    }
#if HAS_MINI_DISPLAY() || HAS_MOCK_DISPLAY()
    // Lack of space on ST7789 -> long text warnings does not have icon
    else {
        icon.Hide();
        text.SetRect(textRectNoIcon);
    }
#endif
}
