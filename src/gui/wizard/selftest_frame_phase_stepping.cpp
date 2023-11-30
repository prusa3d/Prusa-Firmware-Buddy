#include "selftest_frame_phase_stepping.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_phase_stepping_result.hpp"
#include "img_resources.hpp"

namespace {
constexpr const char *txt_header { N_("Phase Stepping Calibration") };
constexpr const char *txt_picking_tool { N_("Picking Tool") };
constexpr const char *txt_calibrating_x { N_("Calibrating X") };
constexpr const char *txt_calibrating_y { N_("Calibrating Y") };
constexpr const char *txt_calibrating_failed { N_("Calibration failed.") };
constexpr const char *txt_enabling { N_("Finishing") };
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
    case PhasesSelftest::PhaseStepping_calib_failed:
        text.SetText(_(txt_calibrating_failed));
        break;
    case PhasesSelftest::PhaseStepping_enabling:
        text.SetText(_(txt_enabling));
        break;

    default:
        break;
    }
};
