#include "window_dlg_warning.hpp"

#include "img_resources.hpp"
#include <common/enum_array.hpp>
#include <common/find_error.hpp>
#include <common/fsm_base_types.hpp>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_uneven_bed_prompt.h>
#include <state/printer_state.hpp>

static constexpr int16_t icon_size = 48;
static constexpr int16_t qr_size = GuiDefaults::QRSize;
static constexpr const img::Resource *phone_resource = &img::hand_qr_59x72;

static constexpr Rect16 screen_rect = GuiDefaults::RectScreen;
static constexpr uint16_t padding = HAS_MINI_DISPLAY() ? 6 : 26;
static constexpr int16_t top_row_height = std::max({ icon_size, (int16_t)phone_resource->h, qr_size });
static constexpr int16_t top_row_width_spare_space = screen_rect.Width() - (icon_size + phone_resource->w + qr_size + 2 * padding);
static_assert(top_row_width_spare_space > 0);

static constexpr int16_t icon_left = top_row_width_spare_space / 2;
static constexpr Rect16 icon_rect = {
    icon_left,
    padding + (top_row_height - icon_size) / 2,
    icon_size,
    icon_size,
};
static constexpr int16_t phone_left = icon_left + icon_size + padding;
static constexpr Rect16 phone_rect = {
    phone_left,
    padding + (top_row_height - phone_resource->h) / 2,
    phone_resource->w,
    phone_resource->h,
};
static constexpr Rect16 qr_rect = {
    phone_left + phone_resource->w + padding,
    padding + (top_row_height - qr_size) / 2,
    qr_size,
    qr_size,
};

static constexpr Rect16 text_rect = Rect16::fromLTRB(
    padding,
    padding + top_row_height + padding,
    screen_rect.Width() - padding,
    GuiDefaults::GetButtonRect(screen_rect).Top());

const img::Resource *warning_dialog_icon(WarningType warning_type) {
    switch (warning_type) {

    default:
        // Warnings have the warning icon by default, but you can change it if you need it
        return &img::warning_48x48;

    case WarningType::HotendFanError:
    case WarningType::PrintFanError:
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

#if ENABLED(CALIBRATION_GCODE)
    case WarningType::NozzleDoesNotHaveRoundSection:
        return &img::nozzle_34x32;
#endif

    case WarningType::NotDownloaded:
        return &img::no_stream_48x48;
    }
}

DialogWarning::DialogWarning(fsm::BaseData data)
    : IDialogMarlin(screen_rect)
    , icon(this, icon_rect, nullptr)
    , phone(this, phone_rect, phone_resource)
    , qr(this, qr_rect, ErrCode::ERR_UNDEF)
    , text(this, text_rect, is_multiline::yes, is_closed_on_click_t::yes, {})
    , button(this, GuiDefaults::GetButtonRect(screen_rect), PhasesWarning::_last) {
    CaptureNormalWindow(button);
    Change(data);
}

void DialogWarning::Change(fsm::BaseData data) {
    const auto phase = GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase());
    const auto warning_type = static_cast<WarningType>(*data.GetData().data());
    const auto err_desc = find_error(printer_state::warning_type_to_error_code(warning_type));

    icon.SetRes(warning_dialog_icon(warning_type));
    qr.set_error_code(err_desc.err_code);
    text.SetText(_(err_desc.err_text));
    button.set_fsm_and_phase(phase);

    // Reset selection to default on phase change
    button.SetBtnIndex(0);
}
