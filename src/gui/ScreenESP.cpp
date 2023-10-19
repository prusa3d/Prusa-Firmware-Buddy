#include "ScreenESP.hpp"

#include "i18n.h"
#include "img_resources.hpp"

ScreenESP::ScreenESP()
    : AddSuperWindow<screen_t>()
    , header(this, _(en_esp))
    , part_current(ESPParts::_none)
    , part_previous(ESPParts::_none) {
    ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetIcon(&img::wifi_16x16);
    ths = this;
}

ScreenESP::~ScreenESP() {
    ths = nullptr;
}

ScreenESP *ScreenESP::ths = nullptr;

ScreenESP *ScreenESP::GetInstance() {
    return ScreenESP::ths;
}

void ScreenESP::Change(fsm::BaseData data) {
    const uint8_t phase_id = data.GetPhase();
    const PhasesESP phase = GetEnumFromPhaseIndex<PhasesESP>(phase_id);

    part_previous = part_current;
    part_current = ESPGetPartFromPhase(phase);

    if (part_previous != part_current) {
        ReleaseCaptureOfNormalWindow(); // release is not automatic !!!
        ptr = nullptr; // Must explicitly assign nullptr, otherwise destructor is not called
        switch (part_current) {
        case ESPParts::ESP:
            ptr = makePtr<ESPFrameText>(this, phase, data.GetData());
            break;
        case ESPParts::ESP_progress:
            ptr = makePtr<ESPFrameProgress>(this, phase, data.GetData());
            break;
        case ESPParts::ESP_qr:
            ptr = makePtr<ESPFrameQR>(this, phase, data.GetData());
            break;
        default:
            break;
        }
        if (ptr) {
            CaptureNormalWindow(*ptr);
        }
    } else if (ptr) {
        ptr->Change(phase, data.GetData());
    }
}
