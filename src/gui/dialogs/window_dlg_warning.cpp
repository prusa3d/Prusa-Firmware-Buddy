#include "window_dlg_warning.hpp"

#include <common/fsm_base_types.hpp>

static constexpr int16_t iconSize = 48;

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
static constexpr Rect16 layoutRect = { 16, 40, GuiDefaults::ScreenWidth - 16 * 2, 170 };
static constexpr Rect16 textRect = Rect16(layoutRect.Left() + iconSize + 8, layoutRect.Top(), layoutRect.Width() - iconSize - 8, layoutRect.Height());

#else
static constexpr Rect16 layoutRect = { 70, 90, 363, GuiDefaults::ScreenHeight - 90 - GuiDefaults::ButtonHeight };
static constexpr Rect16 textRect = Rect16(layoutRect.Left() + iconSize + 15, layoutRect.Top(), 300, layoutRect.Height());

#endif

static constexpr Rect16 iconRect = Rect16(layoutRect.Left(), layoutRect.Top(), iconSize, iconSize);

DialogWarning::DialogWarning(fsm::BaseData data)
    : AddSuperWindow<IDialogMarlin>(GuiDefaults::RectScreenBody)
    , icon(this, iconRect, &img::warning_48x48)
    , text(this, textRect, is_multiline::yes, is_closed_on_click_t::yes, _(icon_title_text[get_type(data)].text))
    , button(this, GuiDefaults::GetButtonRect_AvoidFooter(GuiDefaults::RectScreenBody), PhasesWarning::Warning) {
    CaptureNormalWindow(button);
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
    case WarningType::NozzleDoesNotHaveRoundSection:
        return NozzleDoesNotHaveRoundSection;
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
    default:
        assert(false);
        return count_;
    }
}
