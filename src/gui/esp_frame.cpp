#include "esp_frame.hpp"

#include "marlin_client.hpp"
#include <guiconfig/wizard_config.hpp>

ESPFrame::ESPFrame(window_t *parent, PhasesESP ph, fsm::PhaseData data)
    : AddSuperWindow<window_frame_t>(parent, WizardDefaults::RectSelftestFrame)
    , phase_current(PhasesESP::_none)
    , phase_previous(PhasesESP::_none)
    , radio(this, WizardDefaults::RectRadioButton(0), ph) {
    Enable();
    CaptureNormalWindow(radio);
    Change(ph, data);
}

void ESPFrame::Change(PhasesESP ph, fsm::PhaseData data) {
    data_previous = data_current;
    data_current = data;
    phase_previous = phase_current;
    phase_current = ph;
    if (phase_current == phase_previous && data_current == data_previous) {
        return;
    }

    if (phase_current != phase_previous) {
        radio.Change(phase_current);
    }
    change();
}
