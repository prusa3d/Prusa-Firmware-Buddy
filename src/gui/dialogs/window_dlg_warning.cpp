#include "window_dlg_warning.hpp"

#include "img_resources.hpp"
#include <common/enum_array.hpp>
#include <common/find_error.hpp>
#include <common/fsm_base_types.hpp>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
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

static constexpr Rect16 text_rect = {
    padding,
    padding + top_row_height + padding,
    screen_rect.Width() - padding * 2,
    screen_rect.Height() - (padding + top_row_height + padding),
};

const img::Resource *warning_dialog_icon(WarningType warning_type) {
    static constexpr EnumArray<WarningType, const img::Resource *, static_cast<int>(WarningType::_last) + 1> data {
        { WarningType::HotendFanError, &img::fan_error_48x48 },
            { WarningType::PrintFanError, &img::fan_error_48x48 },
            { WarningType::HeatersTimeout, &img::exposure_times_48x48 },
            { WarningType::HotendTempDiscrepancy, &img::warning_48x48 },
            { WarningType::NozzleTimeout, &img::exposure_times_48x48 },
#if _DEBUG
            { WarningType::SteppersTimeout, &img::exposure_times_48x48 },
#endif
            { WarningType::USBFlashDiskError, &img::usb_error_48x48 },
#if ENABLED(POWER_PANIC)
            { WarningType::HeatbedColdAfterPP, &img::warning_48x48 },
#endif
            { WarningType::HeatBreakThermistorFail, &img::warning_48x48 },
#if ENABLED(CALIBRATION_GCODE)
            { WarningType::NozzleDoesNotHaveRoundSection, &img::nozzle_34x32 },
#endif
            { WarningType::BuddyMCUMaxTemp, &img::warning_48x48 },
#if HAS_DWARF()
            { WarningType::DwarfMCUMaxTemp, &img::warning_48x48 },
#endif
#if HAS_MODULARBED()
            { WarningType::ModBedMCUMaxTemp, &img::warning_48x48 },
#endif
#if HAS_BED_PROBE
            { WarningType::ProbingFailed, &img::warning_48x48 },
#endif
#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
            { WarningType::NozzleCleaningFailed, &img::warning_48x48 },
#endif
#if XL_ENCLOSURE_SUPPORT()
            { WarningType::EnclosureFilterExpirWarning, &img::warning_48x48 },
            { WarningType::EnclosureFilterExpiration, &img::warning_48x48 },
            { WarningType::EnclosureFanError, &img::warning_48x48 },
#endif
            { WarningType::NotDownloaded, &img::no_stream_48x48 },
            { WarningType::GcodeCorruption, &img::warning_48x48 },
            { WarningType::GcodeCropped, &img::warning_48x48 },
            { WarningType::MetricsConfigChangePrompt, &img::warning_48x48 },
    };
    return data[warning_type];
}

static ErrDesc warning_type_to_error_desc(WarningType warning_type) {
    return find_error(printer_state::warning_type_to_error_code(warning_type));
}

DialogWarning::DialogWarning(fsm::BaseData data)
    : DialogWarning {
        GetEnumFromPhaseIndex<PhasesWarning>(data.GetPhase()),
        static_cast<WarningType>(*data.GetData().data())
    } {
}

DialogWarning::DialogWarning(PhasesWarning phase, WarningType warning_type)
    : DialogWarning { phase, warning_type, warning_type_to_error_desc(warning_type) } {
}

DialogWarning::DialogWarning(PhasesWarning data, WarningType warning_type, const ErrDesc &error_desc)
    : IDialogMarlin(screen_rect)
    , icon(this, icon_rect, warning_dialog_icon(warning_type))
    , phone(this, phone_rect, phone_resource)
    , qr(this, qr_rect, error_desc.err_code)
    , text(this, text_rect, is_multiline::yes, is_closed_on_click_t::yes, _(error_desc.err_text))
    , button(this, GuiDefaults::GetButtonRect(screen_rect), data) {
    CaptureNormalWindow(button);
}
