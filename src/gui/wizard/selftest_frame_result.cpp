/**
 * @file selftest_frame_result.cpp
 * @author Radek Vana
 * @brief part of selftest showing result
 * @date 2022-01-24
 */

#include "selftest_frame_result.hpp"
#include "ScreenHandler.hpp"
#include "wizard_config.hpp"
#include "selftest_result_type.hpp"
#include "selftest_eeprom.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"
#include "i18n.h"

static constexpr size_t view_msg_gap = 10;
static constexpr size_t msg_bottom_gap = 6;
static Rect16::Height_t msg_height() { return 2 * GuiDefaults::Font->h; }
static Rect16::Height_t view_height() { return WizardDefaults::Y_space - msg_height() - view_msg_gap - msg_bottom_gap; }

SelftestFrameResult::SelftestFrameResult(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , view(this, { WizardDefaults::col_0, WizardDefaults::row_0, WizardDefaults::X_space, view_height() })
    , msg(this, this->GenerateRect(msg_height(), view_msg_gap), is_multiline::yes)
    , bar(this)

    , fans(SelftestResult_t(data).heatBreakFan, SelftestResult_t(data).printFan)
    , axis(SelftestResult_t(data).xaxis, SelftestResult_t(data).yaxis, SelftestResult_t(data).zaxis)
    , heaters(SelftestResult_t(data).nozzle, SelftestResult_t(data).bed)
    , eth(SelftestResult_t(data).eth)
    , wifi(SelftestResult_t(data).wifi) {

    if (SelftestResult_t(data).Passed()) {
        msg.SetText(_("Selftest passed.\nCheck summary for more information."));
    } else if (SelftestResult_t(data).Failed()) {
        msg.SetText(_("Selftest failed.\nPlease check summary for failed checks."));
    } else
        msg.SetText(_("Some tests did not run or were skipped. Check summary for more information."));

    //TODO automatic
    view.Add(fans);
    view.Add(axis);
    view.Add(heaters);
    view.Add(eth);
    view.Add(wifi);

    height_draw_offset = view.GetDrawOffset();
    virtual_view_height = view.GetTotalHeight();
    bar.SetHeightToScroll(virtual_view_height);
    bar.SetScrollOffset(height_draw_offset);
}

void SelftestFrameResult::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        marlin_FSM_response(phase_current, ClientResponses::GetResponses(phase_current)[0]);
        break;
    case GUI_event_t::ENC_DN:
        height_draw_offset = std::max(height_draw_offset - pixels_per_knob_move, 0);
        view.SetDrawOffset(height_draw_offset);
        bar.SetScrollOffset(height_draw_offset);
        break;
    case GUI_event_t::ENC_UP:
        height_draw_offset = std::min(height_draw_offset + pixels_per_knob_move, virtual_view_height - 1);
        view.SetDrawOffset(height_draw_offset);
        bar.SetScrollOffset(height_draw_offset);
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}
