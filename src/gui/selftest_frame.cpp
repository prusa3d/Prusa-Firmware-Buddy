/**
 * @file selftest_frame.cpp
 * @author Radek Vana
 * @date 2021-11-30
 */

#include "selftest_frame.hpp"
#include "marlin_client.hpp"
#include "wizard_config.hpp"

/*****************************************************************************/
//SelftestFrame
SelftestFrame::SelftestFrame(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<window_frame_t>(parent, WizardDefaults::RectSelftestFrame)
    , phase_current(PhasesSelftest::_none)
    , phase_previous(PhasesSelftest::_none) {
    Change(ph, data);
}

void SelftestFrame::Change(PhasesSelftest ph, fsm::PhaseData data) {
    data_previous = data_current;
    data_current = data;
    phase_previous = phase_current;
    phase_current = ph;
    if (phase_current == phase_previous && data_current == data_previous)
        return;
    pre_change();
    change();
}

/*****************************************************************************/
//SelftestFrameWithRadio
SelftestFrameWithRadio::SelftestFrameWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , radio(this, GuiDefaults::GuiDefaults::GetButtonRect(GetRect()), ClientResponses::GetResponses(ph)) {
    Enable();
}

//TODO make radio button events behave like normal button
void SelftestFrameWithRadio::windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(phase_current, response);
        break;
    }
    case GUI_event_t::ENC_UP:
        ++radio;
        break;
    case GUI_event_t::ENC_DN:
        --radio;
        break;
    default:
        break;
    }
}

void SelftestFrameWithRadio::pre_change() {
    if (phase_current == phase_previous)
        return;
    radio.Change(ClientResponses::GetResponses(phase_current));
}

/*****************************************************************************/
//SelftestFrameNamed
SelftestFrameNamed::SelftestFrameNamed(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , test_name(this, WizardDefaults::RectSelftestName, is_multiline::no, is_closed_on_click_t::no, name) {
}

/*****************************************************************************/
//SelftestFrameNamedWithRadio
SelftestFrameNamedWithRadio::SelftestFrameNamedWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name)
    : AddSuperWindow<SelftestFrameNamed>(parent, ph, data, name)
    , radio(this, WizardDefaults::RectRadioButton, ClientResponses::GetResponses(ph)) {
    Enable();
}

//TODO make radio button events behave like normal button
void SelftestFrameNamedWithRadio::windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(phase_current, response);
        break;
    }
    case GUI_event_t::ENC_UP:
        ++radio;
        break;
    case GUI_event_t::ENC_DN:
        --radio;
        break;
    default:
        break;
    }
}

void SelftestFrameNamedWithRadio::pre_change() {
    if (phase_current == phase_previous)
        return;
    radio.Change(ClientResponses::GetResponses(phase_current));
}
