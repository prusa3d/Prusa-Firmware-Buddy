#include "window_dlg_warning.hpp"

#include <common/fsm_base_types.hpp>
#include <state/printer_state.hpp>
#include <img_resources.hpp>
#include <option/has_tool_offset_pin_calibration.h>
#include <option/has_indx.h>

namespace {
const img::Resource *warning_dialog_icon(WarningType warning_type) {
    switch (warning_type) {

    default:
        // Warnings have the warning icon by default, but you can change it if you need it
        return &img::warning_48x48;

    case WarningType::HotendFanError:
    case WarningType::PrintFanError:
#if HAS_INDX()
    case WarningType::DockFanError:
#endif
        return &img::fan_error_48x48;

    case WarningType::HeatersTimeout:
    case WarningType::NozzleTimeout:
#if _DEBUG
    case WarningType::SteppersTimeout:
#endif
        return &img::exposure_times_48x48;

    case WarningType::USBFlashDiskError:
    case WarningType::USBDriveUnsupportedFileSystem:
        return &img::usb_error_48x48;

#if HAS_TOOL_OFFSET_PIN_CALIBRATION()
    case WarningType::NozzleDoesNotHaveRoundSection:
        return &img::nozzle_34x32;
#endif

    case WarningType::NotDownloaded:
        return &img::no_stream_48x48;
    }
}
} // namespace

DialogWarning::DialogWarning(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreenNoHeader) {
    Change(data);
}

void DialogWarning::Change(fsm::BaseData data) {
    const auto phase = GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase());
    const auto warning_type = static_cast<WarningType>(*data.GetData().data());
    const auto err_code = printer_state::warning_type_to_error_code(warning_type);

    // Construct-once frame; recreated on change because the warning type
    // (and thus the error code and icon) is delivered through the FSM data.
    frame_.emplace(this, FSMAndPhase { phase }, err_code, warning_dialog_icon(warning_type));
}
