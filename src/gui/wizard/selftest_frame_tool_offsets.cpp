#include "selftest_frame_tool_offsets.hpp"
#include "i18n.h"
#include "png_resources.hpp"
#include "wizard_config.hpp"
#include "selftest_tool_offsets_type.hpp"
#include "marlin_client.hpp"
#include <array>

static constexpr size_t col_texts = WizardDefaults::MarginLeft;
static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;

SelftestFrameToolOffsets::SelftestFrameToolOffsets(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _("Tool Offsets Calibration"), 1)
    , footer(this, 0, footer::items::ItemAllNozzles, footer::items::ItemBed) // ItemAxisZ to show Z coord while moving up
    , progress(this, WizardDefaults::row_1)
    , text_phase(this, Rect16(col_texts, row_2, WizardDefaults::X_space, txt_h * 5), is_multiline::yes) {
    change();
}

void SelftestFrameToolOffsets::change() {
    switch (phase_current) {
    case PhasesSelftest::ToolOffsets_wait_user_confirm_start:
        text_phase.SetText(_("We will need your help with this calibration. You will be asked to:\n- Screw in special calibration pin"));
        break;

    case PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold:
    case PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot:
        text_phase.SetText(_("Nozzle have to be perfectly clean for good calibration results.\n- Clean all nozzles\n- Clean parking plate\n- Press continue when done"));
        break;

    case PhasesSelftest::ToolOffsets_wait_user_install_sheet:
        text_phase.SetText(_("Install sheet on heatbed."));
        break;

    case PhasesSelftest::ToolOffsets_pin_install_prepare:
        text_phase.SetText(_("Preparing for calibration pin installation."));
        break;

    case PhasesSelftest::ToolOffsets_wait_user_install_pin:
        text_phase.SetText(_("Remove heatbed sheet, install calibration pin."));
        break;

    case PhasesSelftest::ToolOffsets_wait_calibrate:
        text_phase.SetText(_("Calibrating tool offsets."));
        break;

    case PhasesSelftest::ToolOffsets_wait_final_park:
        text_phase.SetText(_("Moving away."));
        break;

    case PhasesSelftest::ToolOffsets_wait_user_remove_pin:
        text_phase.SetText(_("Remove calibration pin, install sheet on heatbed."));
        break;

    default:
        text_phase.Hide();
    }
};
