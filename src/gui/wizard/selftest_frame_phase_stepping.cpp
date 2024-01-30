#include "selftest_frame_phase_stepping.hpp"
#include "i18n.h"
#include "str_utils.hpp"
#include "wizard_config.hpp"
#include "selftest_phase_stepping_result.hpp"
#include "img_resources.hpp"

namespace {
constexpr const char *txt_header { N_("Phase Stepping Calibration") };
constexpr const char *txt_picking_tool { N_("Picking Tool") };
constexpr const char *txt_calibrating_x { N_("Calibrating X") };
constexpr const char *txt_calibrating_y { N_("Calibrating Y") };
constexpr const char *txt_calibration_nok { N_("Calibration of axis %c failed.\nParameter 1: forward %3d%%, backward %3d%%\nParameter 2: forward %3d%%, backward %3d%%") };
constexpr const char *txt_calibration_error { N_("Calibration failed with error.") };
constexpr const char *txt_enabling { N_("Finishing") };
constexpr const char *txt_calibration_ok { N_("Axis X vibration A reduced by %2d%%\nAxis X vibration B reduced by %2d%%\nAxis Y vibration A reduced by %2d%%\nAxis Y vibration B reduced by %2d%%") };

string_view_utf8 fmt_calibration_nok(char axis, auto &text_buffer, const fsm::PhaseData &data) {
    std::array<char, 150> fmt;
    _(txt_calibration_nok).copyToRAM(fmt.data(), fmt.size());
    snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), axis, data[0], data[1], data[2], data[3]);
    return string_view_utf8::MakeRAM((const uint8_t *)text_buffer.data());
}

string_view_utf8 fmt_calibration_ok(auto &text_buffer, const fsm::PhaseData &data) {
    std::array<char, 150> fmt;
    _(txt_calibration_ok).copyToRAM(fmt.data(), fmt.size());
    snprintf(text_buffer.data(), text_buffer.size(), fmt.data(), data[0], data[1], data[2], data[3]);
    return string_view_utf8::MakeRAM((const uint8_t *)text_buffer.data());
}

} // namespace

SelftestFramePhaseStepping::SelftestFramePhaseStepping(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamedWithRadio>(parent, ph, data, _(txt_header), 1)
    , footer(this, 0, footer::Item::axis_x, footer::Item::axis_y)
    , text(this, { 0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, GuiDefaults::BodyHeight - GuiDefaults::FooterHeight * 3 }, is_multiline::yes) {
    text.SetAlignment(Align_t::Center()); // looks better with temporary text until this frame gets proper texts
    change();
}

void SelftestFramePhaseStepping::change() {
    switch (phase_current) {
    case PhasesSelftest::PhaseStepping_pick_tool:
        text.SetText(_(txt_picking_tool));
        break;
    case PhasesSelftest::PhaseStepping_calib_x:
        text.SetText(_(txt_calibrating_x));
        break;
    case PhasesSelftest::PhaseStepping_calib_y:
        text.SetText(_(txt_calibrating_y));
        break;
    case PhasesSelftest::PhaseStepping_calib_x_nok:
        text.SetText(fmt_calibration_nok('X', text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_calib_y_nok:
        text.SetText(fmt_calibration_nok('Y', text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_calib_error:
        text.SetText(_(txt_calibration_error));
        break;
    case PhasesSelftest::PhaseStepping_calib_ok:
        text.SetText(fmt_calibration_ok(text_buffer, data_current));
        break;
    case PhasesSelftest::PhaseStepping_enabling:
        text.SetText(_(txt_enabling));
        break;
    default:
        break;
    }
};
