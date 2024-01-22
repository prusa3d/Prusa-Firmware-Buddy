/**
 * @file selftest_frame.cpp
 * @author Radek Vana
 * @date 2021-11-30
 */

#include "selftest_frame.hpp"
#include "marlin_client.hpp"
#include <guiconfig/wizard_config.hpp>

/*****************************************************************************/
// SelftestFrame
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
    if (phase_current == phase_previous && data_current == data_previous) {
        return;
    }
    pre_change();
    change();
}

/*****************************************************************************/
// SelftestFrameWithRadio
SelftestFrameWithRadio::SelftestFrameWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, size_t lines_of_footer)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , radio(this, WizardDefaults::RectRadioButton(lines_of_footer), ph) {
    Enable();
    CaptureNormalWindow(radio);
}

void SelftestFrameWithRadio::pre_change() {
    if (phase_current == phase_previous) {
        return;
    }
    radio.Change(phase_current);
}

/*****************************************************************************/
// SelftestFrameNamed
SelftestFrameNamed::SelftestFrameNamed(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , test_name(this, WizardDefaults::RectSelftestName, is_multiline::no, is_closed_on_click_t::no, name) {
}
void SelftestFrameNamed::SetName(string_view_utf8 txt) {
    test_name.SetText(txt);
    test_name.Invalidate(); // force invalidate because we could be using the same buffer
}

/*****************************************************************************/
// SelftestFrameNamedWithRadio
SelftestFrameNamedWithRadio::SelftestFrameNamedWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name, size_t lines_of_footer)
    : AddSuperWindow<SelftestFrameNamed>(parent, ph, data, name)
    , radio(this, WizardDefaults::RectRadioButton(lines_of_footer), ph) {
    Enable();
    CaptureNormalWindow(radio);
}

void SelftestFrameNamedWithRadio::pre_change() {
    if (phase_current == phase_previous) {
        return;
    }
    radio.Change(phase_current);
}
