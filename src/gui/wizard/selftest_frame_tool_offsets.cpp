#include "selftest_frame_tool_offsets.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include "wizard_config.hpp"
#include "selftest_tool_offsets_type.hpp"
#include "marlin_client.hpp"
#include <array>
#include <module/prusa/toolchanger.h>

static constexpr size_t col_texts = WizardDefaults::MarginLeft;
static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;

SelftestFrameToolOffsets::SelftestFrameToolOffsets(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _("Tool Offsets Calibration"), 1)
    , footer(this, 0, footer::Item::all_nozzles, footer::Item::bed)
    , progress(this, WizardDefaults::row_1)
    , text_phase(this, Rect16(col_texts, row_2, WizardDefaults::X_space, txt_h * 5), is_multiline::yes)
    , text_detail(this, Rect16(col_texts, row_2 + 5 * txt_h, WizardDefaults::X_space, txt_h * 2), is_multiline::yes) {
    change();
}

void SelftestFrameToolOffsets::change() {
    switch (phase_current) {
    case PhasesSelftest::ToolOffsets_wait_user_confirm_start:
        text_phase.SetText(_("We will need your help with this calibration. You will be asked to screw in a calibration pin."));

        // Quick user can
        //  1:15 - Home and install pin
        //  2:15 - Calibrate tool
        //  0:15 - Remove pin and install sheet
        // For 2 tool printer - at least 6 minutes
        // For 5 tool printer - at least 12:45
        if (prusa_toolchanger.get_num_enabled_tools() <= 2) {
            text_detail.SetText(_("The calibration should take a little over 7 minutes."));
        } else {
            text_detail.SetText(_("The calibration should take a little over 14 minutes."));
        }
        text_detail.Show();
        break;

    case PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold:
    case PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot:
        text_phase.SetText(_("Nozzle have to be perfectly clean for good calibration results.\n- Clean all nozzles\n- Clean parking plate\n- Press continue when done"));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_wait_user_install_sheet:
        text_phase.SetText(_("Install sheet on heatbed."));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_pin_install_prepare:
        text_phase.SetText(_("Preparing for calibration pin installation."));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_wait_user_install_pin:
        text_phase.SetText(_("- Remove heatbed sheet.\n- Install calibration pin."));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_wait_stable_temp:
        text_phase.SetText(_("Waiting for hotends to stabilize at calibration temperature:"));
        snprintf(target_temp_text, std::size(target_temp_text), "%d\xC2\xB0\x43", SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE); // \xB0\x43 == degree Celsius
        text_detail.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(target_temp_text)));
        text_detail.Show();
        break;

    case PhasesSelftest::ToolOffsets_wait_calibrate:
        text_phase.SetText(_("Calibrating tool offsets."));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_wait_move_away:
        text_phase.SetText(_("Moving away."));
        text_detail.Hide();
        break;

    case PhasesSelftest::ToolOffsets_wait_user_remove_pin:
        text_phase.SetText(_("- Remove calibration pin.\n- Install sheet on heatbed."));
        text_detail.Hide();
        break;

    default:
        text_phase.Hide();
        text_detail.Hide();
    }
};
