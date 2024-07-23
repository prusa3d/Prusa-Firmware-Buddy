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
#include <option/has_selftest.h>

#include "selftest_frame_revise_printer_setup.hpp"
#include "selftest_frame_axis.hpp"
#include "selftest_frame_fans.hpp"
#include "selftest_frame_fsensor.hpp"
#include "selftest_frame_gears_calib.hpp"
#include "selftest_frame_loadcell.hpp"
#include "selftest_frame_calib_z.hpp"
#include "selftest_frame_temp.hpp"
#include "selftest_frame_firstlayer.hpp"
#include "selftest_frame_firstlayer_questions.hpp"
#include "selftest_frame_result.hpp"
#include "selftest_frame_wizard_prologue.hpp"
#include "selftest_frame_wizard_epilogue.hpp"
#include "selftest_frame_dock.hpp"
#include "selftest_frame_tool_offsets.hpp"
#include "selftest_invalid_state.hpp"

ScreenSelftest::fnc ScreenSelftest::Get(SelftestParts part) {
    switch (part) {
    case SelftestParts::WizardPrologue:
        return creator<SelftestFrameWizardPrologue>;

    case SelftestParts::Axis:
        return creator<SelftestFrametAxis>;

    case SelftestParts::Fans:
        return creator<SelftestFrameFans>;

#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return creator<SelftestFrameLoadcell>;
#endif

#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
        return creator<SelftestFrameFSensor>;
#endif

#if PRINTER_IS_PRUSA_MK4()
    case SelftestParts::GearsCalib:
        return creator<SelftestFrameGearsCalib>;
#endif

#if BOARD_IS_XLBUDDY()
    case SelftestParts::Dock:
        return creator<SelftestFrameDock>;

    case SelftestParts::ToolOffsets:
        return creator<SelftestFrameToolOffsets>;
#endif

    case SelftestParts::Heaters:
        return creator<ScreenSelftestTemp>;

    case SelftestParts::CalibZ:
        return creator<SelftestFrameCalibZ>;

    case SelftestParts::FirstLayer:
        return creator<SelftestFrameFirstLayer>;

    case SelftestParts::FirstLayerQuestions:
        return creator<SelftestFrameFirstLayerQuestions>;

    case SelftestParts::Result:
        return creator<SelftestFrameResult>;

    case SelftestParts::WizardEpilogue_ok:
    case SelftestParts::WizardEpilogue_nok:
        return creator<SelftestFrameWizardEpilogue>;

    case SelftestParts::RevisePrinterSetup:
        return creator<SelftestFrameRevisePrinterSetup>;

    case SelftestParts::_none:
        break;
    }

    return creator<ScreenSelftestInvalidState>;
}

ScreenSelftest::ScreenSelftest()
    : screen_t()
    , header(this, _(en_selftest))
    , part_current(SelftestParts::_none)
    , part_previous(SelftestParts::_none) {
    ScreenSelftest::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetIcon(&img::selftest_16x16);
}

/******************************************************************************/
// static methods and member variables

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
#if PRINTER_IS_PRUSA_MK4()
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::CalibZ:
    case SelftestParts::Result:
    case SelftestParts::RevisePrinterSetup:
#if BOARD_IS_XLBUDDY()
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
#if PRINTER_IS_PRUSA_MK4()
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::CalibZ:
    case SelftestParts::FirstLayer:
    case SelftestParts::FirstLayerQuestions:
    case SelftestParts::Result:
    case SelftestParts::RevisePrinterSetup:
#if BOARD_IS_XLBUDDY()
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
#if !HAS_SELFTEST()
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
