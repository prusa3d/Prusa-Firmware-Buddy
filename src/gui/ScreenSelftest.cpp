/**
 * @file ScreenSelftest.cpp
 * @author Radek Vana
 * @date 2021-11-25
 */
#include "ScreenSelftest.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include <option/has_selftest_snake.h>

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_prologue(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameWizardPrologue>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_axis(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrametAxis>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_fans(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameFans>(&rThs, phase, data);
}

#if FILAMENT_SENSOR_IS_ADC()
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_fsensor(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameFSensor>(&rThs, phase, data);
}
#endif

#if PRINTER_IS_PRUSA_MK4
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_gears_calib(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameGearsCalib>(&rThs, phase, data);
}
#endif

#if HAS_LOADCELL()
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_loadcell(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameLoadcell>(&rThs, phase, data);
}
#endif
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_temp(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<ScreenSelftestTemp>(&rThs, phase, data);
}

#if !PRINTER_IS_PRUSA_MINI
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_specify_hot_end(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameHotEndSock>(&rThs, phase, data);
}
#endif

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_calib_z(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameCalibZ>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_firstlayer(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameFirstLayer>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_firstlayer_questions(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameFirstLayerQuestions>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_result(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameResult>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_epilogue(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameWizardEpilogue>(&rThs, phase, data);
}

#if BOARD_IS_XLBUDDY
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_dock(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameDock>(&rThs, phase, data);
}

static_unique_ptr<SelftestFrame> ScreenSelftest::creator_tool_offsets(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<SelftestFrameToolOffsets>(&rThs, phase, data);
}
#endif
static_unique_ptr<SelftestFrame> ScreenSelftest::creator_invalid(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
    return rThs.makePtr<ScreenSelftestInvalidState>(&rThs, phase, data);
}

ScreenSelftest::fnc ScreenSelftest::Get(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return creator_prologue;
    case SelftestParts::Axis:
        return creator_axis;
    case SelftestParts::Fans:
        return creator_fans;
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return creator_loadcell;
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
        return creator_fsensor;
#endif
#if PRINTER_IS_PRUSA_MK4
    case SelftestParts::GearsCalib:
        return creator_gears_calib;
#endif
#if BOARD_IS_XLBUDDY
    case SelftestParts::Dock:
        return creator_dock;
    case SelftestParts::ToolOffsets:
        return creator_tool_offsets;
#endif
    case SelftestParts::Heaters:
        return creator_temp;
    case SelftestParts::SpecifyHotEnd:
#if PRINTER_IS_PRUSA_MINI
        break;
#else
        return creator_specify_hot_end;
#endif
    case SelftestParts::CalibZ:
        return creator_calib_z;
    case SelftestParts::FirstLayer:
        return creator_firstlayer;
    case SelftestParts::FirstLayerQuestions:
        return creator_firstlayer_questions;
    case SelftestParts::Result:
        return creator_result;
    case SelftestParts::WizardEpilogue_ok:
    case SelftestParts::WizardEpilogue_nok:
        return creator_epilogue;
    case SelftestParts::_none:
        break;
    }

    return creator_invalid;
}

ScreenSelftest::ScreenSelftest()
    : AddSuperWindow<screen_t>()
    , header(this, _(en_selftest))
    , part_current(SelftestParts::_none)
    , part_previous(SelftestParts::_none) {
    ScreenSelftest::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetIcon(&img::selftest_16x16);
    ths = this;
}

ScreenSelftest::~ScreenSelftest() {
    ths = nullptr;
}

/******************************************************************************/
// static methods and member variables
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
        // update header
        header.SetIcon(&img::home_shape_16x16);
        header.SetIcon(getIconId(part_current));
        header.SetText(getCaption(part_current));

        ReleaseCaptureOfNormalWindow(); // release is not automatic !!!
        // delete old
        ptr = nullptr;
        // create new
        ptr = Get(part_current)(*this, phase, data.GetData());
        CaptureNormalWindow(*ptr);
        return;
    }
    if (!ptr) {
        return; // should never happen
    }

    ptr->Change(phase, data.GetData());
}

string_view_utf8 ScreenSelftest::getCaption(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return _(en_wizard);
    case SelftestParts::Axis:
    case SelftestParts::Fans:
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
#endif
#if PRINTER_IS_PRUSA_MK4
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::SpecifyHotEnd:
    case SelftestParts::CalibZ:
    case SelftestParts::Result:
#if BOARD_IS_XLBUDDY
    case SelftestParts::Dock:
    case SelftestParts::ToolOffsets:
#endif
        return _(en_selftest);
    case SelftestParts::FirstLayer:
    case SelftestParts::FirstLayerQuestions:
        return _(en_firstlay);
    case SelftestParts::WizardEpilogue_ok:
        return _(en_wizard_ok);
    case SelftestParts::WizardEpilogue_nok:
        return _(en_wizard_nok);
    case SelftestParts::_none:
        break;
    }
    return string_view_utf8::MakeCPUFLASH((uint8_t *)error);
}

const img::Resource *ScreenSelftest::getIconId(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return &img::wizard_16x16;
    case SelftestParts::Axis:
    case SelftestParts::Fans:
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
#endif
#if PRINTER_IS_PRUSA_MK4
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::SpecifyHotEnd:
    case SelftestParts::CalibZ:
    case SelftestParts::FirstLayer:
    case SelftestParts::FirstLayerQuestions:
    case SelftestParts::Result:
#if BOARD_IS_XLBUDDY
    case SelftestParts::Dock:
    case SelftestParts::ToolOffsets:
#endif
        return &img::selftest_16x16;
    case SelftestParts::WizardEpilogue_ok:
    case SelftestParts::WizardEpilogue_nok:
        return &img::wizard_16x16;
    case SelftestParts::_none:
        break;
    }
    return &img::error_16x16;
}

void ScreenSelftest::InitState(screen_init_variant var) {
    auto val = var.GetSelftestMask();
    if (val) {
        marlin_client::test_start(*val);
#if !HAS_SELFTEST_SNAKE()
        // check mask if contains wizard prologue
        // it is simplified method, but should work correctly for meaningful use
        if ((*val) & stmWizardPrologue) {
            header.SetIcon(&img::wizard_16x16);
            header.SetText(_(en_wizard));
        }
        // no need for else, selftest is default
#endif
    }
}
