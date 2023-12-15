#include "window_dlg_warning.hpp"

#include <common/fsm_base_types.hpp>

DialogWarning::DialogWarning(fsm::BaseData data)
    : AddSuperWindow<IDialogMarlin>(GuiDefaults::RectScreenBody)
    , icon(this, GuiDefaults::MessageIconRect, &img::warning_48x48)
    , text(this, GuiDefaults::MessageTextRect, is_multiline::yes, is_closed_on_click_t::yes, _(icon_title_text[get_type(data)].text))
    , button(this, GuiDefaults::GetButtonRect_AvoidFooter(GuiDefaults::RectScreenBody), PhasesWarning::Warning) {
    CaptureNormalWindow(button);
}

DialogWarning::types DialogWarning::get_type(fsm::BaseData data) {
    WarningType wtype = static_cast<WarningType>(*data.GetData().data());
    switch (wtype) {
    case WarningType::HotendFanError:
        return HotendFan;
        break;
    case WarningType::PrintFanError:
        return PrintFan;
        break;
    case WarningType::HotendTempDiscrepancy:
        return HotendTempDiscrepancy;
        break;
    case WarningType::HeatersTimeout:
    case WarningType::NozzleTimeout:
        return HeatersTimeout;
        break;
#if _DEBUG
    case WarningType::SteppersTimeout:
        return SteppersTimeout;
        break;
#endif
    case WarningType::USBFlashDiskError:
        return USBFlashDisk;
        break;
    case WarningType::HeatBreakThermistorFail:
        return HBThermistorFail;
        break;
#if ENABLED(POWER_PANIC)
    case WarningType::HeatbedColdAfterPP:
        return HeatbedColdAfterPP;
        break;
#endif
    case WarningType::NozzleDoesNotHaveRoundSection:
        return NozzleDoesNotHaveRoundSection;
        break;
    case WarningType::NotDownloaded:
        return NotDownloaded;
        break;
    case WarningType::BuddyMCUMaxTemp:
        return BuddyMCUMaxTemp;
        break;
#if HAS_DWARF()
    case WarningType::DwarfMCUMaxTemp:
        return DwarfMCUMaxTemp;
        break;
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
    case WarningType::ModBedMCUMaxTemp:
        return ModBedMCUMaxTemp;
        break;
#endif /* HAS_MODULARBED() */
    default:
        assert(false);
    }
}
