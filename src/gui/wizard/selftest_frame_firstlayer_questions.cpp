/**
 * @file selftest_frame_firstlayer_questions.cpp
 */

#include "selftest_frame_firstlayer_questions.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "selftest_firstlayer_type.hpp"
#include "marlin_client.hpp"

namespace {
constexpr size_t col_0 = WizardDefaults::MarginLeft;
constexpr size_t top_of_changeable_area = WizardDefaults::row_1;
constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(2).Top() - top_of_changeable_area;
constexpr Rect16 ChangeableRect = { col_0, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

constexpr const char *text_question_use_val = N_("Do you want to use the current value?\nCurrent: %0.3f.\nDefault: %0.3f.\nClick NO to use the default value (recommended)");
} // namespace

SelftestFrameFirstLayerQuestions::SelftestFrameFirstLayerQuestions(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data, 2)
    , footer(this
#if defined(FOOTER_HAS_LIVE_Z)
          ,
          footer::Item::live_z
#endif
#if defined(FOOTER_HAS_SHEETS)
          ,
          footer::Item::sheets
#endif
          ,
          footer::Item::filament)
    , text(this, ChangeableRect, is_multiline::yes) {

    change();
}

void SelftestFrameFirstLayerQuestions::change() {
    SelftestFirstLayer_t dt(data_current);

    const char *txt = nullptr;

    // texts
    switch (phase_current) {
    case PhasesSelftest::FirstLayer_filament_known_and_not_unsensed:
        txt = N_("To calibrate with currently loaded filament, press NEXT. To change filament, press UNLOAD.");
        radio.SetBtn(dt.preselect_response); // preselected button is depended on filament state
        break;
    case PhasesSelftest::FirstLayer_filament_not_known_or_unsensed:
        txt = N_("To calibrate with currently loaded filament, press NEXT. To load filament, press LOAD. To change filament, press UNLOAD.");
        radio.SetBtn(dt.preselect_response); // preselected button is depended on filament state
        break;
    case PhasesSelftest::FirstLayer_calib:
        txt = N_("Now, let's calibrate the distance between the tip of the nozzle and the print sheet.");
        break;
    case PhasesSelftest::FirstLayer_use_val: {
        std::array<char, 256> buffer;
        [[maybe_unused]] const auto copied = _(text_question_use_val).copyToRAM(buffer.data(), buffer.size());
        snprintf(txt_buff.begin(), txt_buff.size(),
            buffer.data(),
            (double)dt.current_offset, (double)z_offset_def);
        txt = txt_buff.begin();
        break;
    }
    case PhasesSelftest::FirstLayer_start_print:
        txt = N_("In the next step, use the knob to adjust the nozzle height. Check the pictures in the handbook for reference.");
        break;
    case PhasesSelftest::FirstLayer_reprint:
        txt = N_("Do you want to repeat the last step and readjust the distance between the nozzle and heatbed?");
        radio.SetBtn(dt.preselect_response); // want to preselect NO which is second button
        break;
    case PhasesSelftest::FirstLayer_clean_sheet:
        txt = N_("Clean steel sheet.");
        break;
    case PhasesSelftest::FirstLayer_failed:
        txt = N_("The first layer calibration failed to finish. Double-check the printer's wiring, nozzle and axes, then restart the calibration.");
        break;
    default:
        break;
    }

    if (txt) {
        text.Show();
        text.SetText(_(txt));
    } else {
        text.Hide();
    }
};
