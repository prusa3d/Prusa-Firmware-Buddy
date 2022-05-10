/**
 * @file ScreenSelftest.cpp
 * @author Radek Vana
 * @date 2021-11-25
 */
#include "ScreenSelftest.hpp"
#include "resource.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "marlin_client.h"

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_invalid(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<ScreenSelftestInvalidState>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_esp(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameESP>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_esp_progress(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameESP_progress>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_esp_qr(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameESP_qr>(&rThs, phase, data);
}

ScreenSelftest::fnc ScreenSelftest::Get(SelftestParts part) {
    switch (part) {
    case SelftestParts::ESP:
        return creator_esp;
    case SelftestParts::ESP_progress:
        return creator_esp_progress;
    case SelftestParts::ESP_qr:
        return creator_esp_qr;
    case SelftestParts::_none:
        break;
    }

    return creator_invalid;
}

ScreenSelftest::ScreenSelftest()
    : AddSuperWindow<screen_t>()
    , header(this, _(en_esp))
    , part_current(SelftestParts::_none)
    , part_previous(SelftestParts::_none) {
    ScreenSelftest::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetIcon(IDR_PNG_selftest_16x16);
    ths = this;
}

ScreenSelftest::~ScreenSelftest() {
    ths = nullptr;
}

/******************************************************************************/
//static methods and member variables
ScreenSelftest *ScreenSelftest::ths = nullptr;

ScreenSelftest *ScreenSelftest::GetInstance() {
    return ScreenSelftest::ths;
}

void ScreenSelftest::Change(fsm::BaseData data) {
    const uint8_t phase_id = data.GetPhase();
    const PhasesSelftest phase = GetEnumFromPhaseIndex<PhasesSelftest>(phase_id);

    part_previous = part_current;
    part_current = SelftestGetPartFromPhase(phase);

    if (part_previous != part_current) {
        //update header
        header.SetIcon(getIconId(part_current));
        header.SetText(getCaption(part_current));

        ReleaseCaptureOfNormalWindow(); // release is not automatic !!!
        //delete old
        ptr = nullptr;
        //create new
        ptr = Get(part_current)(*this, phase, data.GetData());
        CaptureNormalWindow(*ptr);
        return;
    }
    if (!ptr)
        return; //should never happen

    ptr->Change(phase, data.GetData());
}

string_view_utf8 ScreenSelftest::getCaption(SelftestParts part) {
    switch (part) {
    case SelftestParts::ESP:
    case SelftestParts::ESP_progress:
    case SelftestParts::ESP_qr:
        return _(en_esp);
    case SelftestParts::_none:
        break;
    }
    return string_view_utf8::MakeCPUFLASH((uint8_t *)error);
}

uint16_t ScreenSelftest::getIconId(SelftestParts part) {
    switch (part) {
    case SelftestParts::ESP:
    case SelftestParts::ESP_progress:
    case SelftestParts::ESP_qr:
        return IDR_PNG_wifi_16px;
    case SelftestParts::_none:
        break;
    }
    return IDR_PNG_error_16px;
}

void ScreenSelftest::InitState(screen_init_variant var) {
}
