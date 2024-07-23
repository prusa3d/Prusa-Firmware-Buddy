/**
 * @file selftest_frame_result.cpp
 * @author Radek Vana
 * @brief part of selftest showing result
 * @date 2022-01-24
 */

#include "selftest_frame_result.hpp"
#include "ScreenHandler.hpp"
#include <guiconfig/wizard_config.hpp>
#include "selftest_result_type.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"

static constexpr size_t view_msg_gap = 10;
static constexpr size_t msg_bottom_gap = 6;
static Rect16::Height_t msg_height() { return (GuiDefaults::ScreenWidth > 240 ? 2 : 3) * height(GuiDefaults::DefaultFont); } // cannot be constexpr, because of font
static Rect16::Height_t view_height() { return WizardDefaults::Y_space - msg_height() - view_msg_gap - msg_bottom_gap; }

SelftestFrameResult::SelftestFrameResult(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrame(parent, ph, data)
    , msg(this, { WizardDefaults::col_0, WizardDefaults::row_0, WizardDefaults::X_space, msg_height() }, is_multiline::yes)
    , view(this, this->GenerateRect(view_height(), view_msg_gap))
    , bar(this)
    , eth(false)
    , wifi(true) {

    SelftestResult eeres;

    FsmSelftestResult fsm_data(data);
    if (fsm_data.is_test_selftest()) {
        // Fake some results to test selftest result screen
        auto get_state = [fsm_data](int n) {
            return static_cast<TestResult>((fsm_data.test_selftest_code() >> ((n & 0x3) * 2)) & 0x03);
        };
        HOTEND_LOOP() {
            eeres.tools[e].printFan = get_state(e);
            eeres.tools[e].heatBreakFan = get_state(e + 1);

#if not PRINTER_IS_PRUSA_MINI()
            eeres.tools[e].fansSwitched = get_state(e + 2);
#endif
            eeres.tools[e].nozzle = get_state(e + 3);
            eeres.tools[e].fsensor = get_state(e + 4);
            eeres.tools[e].loadcell = get_state(e + 5);
            eeres.tools[e].sideFsensor = get_state(e + 6);
        }
        eeres.xaxis = get_state(0);
        eeres.yaxis = get_state(1);
        eeres.zaxis = get_state(2);
        eeres.bed = get_state(3);
        eeres.eth = static_cast<TestResultNet>(get_state(4));
        eeres.wifi = static_cast<TestResultNet>(get_state(5));
    } else {
        eeres = config_store().selftest_result.get(); // Read test result directly from EEPROM
    }

#if HAS_TOOLCHANGER()
    // XL should use snake instead of this
    msg.SetText(_("This screen shows only 1st tool!\nXL should use snake!"));
#else
    if (SelftestResult_Passed_All(eeres)) {
        msg.SetText(_("Selftest OK!\nDetails below, use knob to scroll"));
    } else if (SelftestResult_Failed(eeres)) {
        msg.SetText(_("Selftest failed!\nDetails below, use knob to scroll"));
    } else {
        msg.SetText(_("Selftest incomplete!\nDetails below, use knob to scroll"));
    }
#endif /*HAS_TOOLCHANGER()*/

    // Set results
    fans.SetState(eeres.tools[0].heatBreakFan, eeres.tools[0].printFan, eeres.tools[0].fansSwitched);
#if HAS_LOADCELL()
    loadcell.SetState(eeres.tools[0].loadcell);
#endif /*HAS_LOADCELL()*/
    heaters.SetState(eeres.tools[0].nozzle, eeres.bed);
#if FILAMENT_SENSOR_IS_ADC()
    fsensor.SetState(eeres.tools[0].fsensor);
#endif /*FILAMENT_SENSOR_IS_ADC()*/
    axis.SetState(eeres.xaxis, eeres.yaxis, eeres.zaxis);
    eth.SetState(eeres.eth);
    wifi.SetState(eeres.wifi);

    // Add all
    view.Add(fans);
    view.Add(axis);
#if HAS_LOADCELL()
    view.Add(loadcell);
#endif /*HAS_LOADCELL()*/
    view.Add(heaters);
#if FILAMENT_SENSOR_IS_ADC()
    view.Add(fsensor);
#endif /*FILAMENT_SENSOR_IS_ADC()*/
    view.Add(eth);
    view.Add(wifi);

    height_draw_offset = view.GetDrawOffset();
    virtual_view_height = view.GetTotalHeight();
    bar.SetHeightToScroll(virtual_view_height);
    bar.SetScrollOffset(height_draw_offset);
}

void SelftestFrameResult::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        marlin_client::FSM_response(phase_current, ClientResponses::GetResponses(phase_current)[0]);
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
        SelftestFrame::windowEvent(sender, event, param);
    }
}
