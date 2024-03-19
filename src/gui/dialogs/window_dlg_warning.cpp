#include "window_dlg_warning.hpp"

#include <common/fsm_base_types.hpp>

static constexpr int16_t iconSize = 48;

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
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

DialogWarning::DialogWarning(fsm::BaseData data)
    : AddSuperWindow<IDialogMarlin>(GuiDefaults::RectScreenBody)
    , icon(this, iconRect, &img::warning_48x48)
    , text(this, textRect, is_multiline::yes, is_closed_on_click_t::yes, _(find_error(icon_code[get_type(data)].code).err_text))
    , button(this, GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody), GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase())) {
    CaptureNormalWindow(button);

    if (icon_code[get_type(data)].icon) {
        icon.SetRes(icon_code[get_type(data)].icon);
    }
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    // Lack of space on ST7789 -> long text warnings does not have icon
    else {
        icon.Hide();
        text.SetRect(textRectNoIcon);
    }
#endif
}

DialogWarning::types DialogWarning::get_type(fsm::BaseData data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    switch (wtype) {
    case WarningType::HotendFanError:
        return HotendFan;
    case WarningType::PrintFanError:
        return PrintFan;
    case WarningType::HotendTempDiscrepancy:
        return HotendTempDiscrepancy;
    case WarningType::HeatersTimeout:
    case WarningType::NozzleTimeout:
        return HeatersTimeout;
#if _DEBUG
    case WarningType::SteppersTimeout:
        return SteppersTimeout;
#endif
    case WarningType::USBFlashDiskError:
        return USBFlashDisk;
    case WarningType::HeatBreakThermistorFail:
        return HBThermistorFail;
#if ENABLED(POWER_PANIC)
    case WarningType::HeatbedColdAfterPP:
        return HeatbedColdAfterPP;
#endif
#if ENABLED(CALIBRATION_GCODE)
    case WarningType::NozzleDoesNotHaveRoundSection:
        return NozzleDoesNotHaveRoundSection;
#endif
    case WarningType::NotDownloaded:
        return NotDownloaded;
    case WarningType::BuddyMCUMaxTemp:
        return BuddyMCUMaxTemp;
#if HAS_DWARF()
    case WarningType::DwarfMCUMaxTemp:
        return DwarfMCUMaxTemp;
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
    case WarningType::ModBedMCUMaxTemp:
        return ModBedMCUMaxTemp;
#endif /* HAS_MODULARBED() */
#if XL_ENCLOSURE_SUPPORT()
    case WarningType::EnclosureFilterExpirWarning:
        return EnclosureFilterExpirWarning;
    case WarningType::EnclosureFilterExpiration:
        return EnclosureFilterExpiration;
    case WarningType::EnclosureFanError:
        return EnclosureFanError;
#endif /* XL_ENCLOSURE_SUPPORT */
#if HAS_BED_PROBE
    case WarningType::ProbingFailed:
        return ProbingFailed;
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    case WarningType::NozzleCleaningFailed:
        return NozzleCleaningFailed;
#endif
    }
    assert(false);
    return count_;
}
