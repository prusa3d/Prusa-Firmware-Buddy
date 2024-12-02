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
#include "selftest_frame_fsensor.hpp"
#include "selftest_frame_gears_calib.hpp"
#include "selftest_frame_loadcell.hpp"
#include "selftest_frame_calib_z.hpp"
#include "selftest_frame_temp.hpp"
#include "selftest_frame_firstlayer.hpp"
#include "selftest_frame_firstlayer_questions.hpp"
#include "selftest_frame_dock.hpp"
#include "selftest_frame_tool_offsets.hpp"
#include "selftest_invalid_state.hpp"

ScreenSelftest::fnc ScreenSelftest::Get(SelftestParts part) {
    switch (part) {
    case SelftestParts::Axis:
        return creator<SelftestFrametAxis>;

#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
        return creator<SelftestFrameLoadcell>;
#endif

#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
        return creator<SelftestFrameFSensor>;
#endif

#if HAS_GEARS_CALIBRATION()
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
    case SelftestParts::Axis:
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
#endif
#if HAS_GEARS_CALIBRATION()
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::CalibZ:
    case SelftestParts::RevisePrinterSetup:
#if BOARD_IS_XLBUDDY()
    case SelftestParts::Dock:
    case SelftestParts::ToolOffsets:
#endif
        return _(en_selftest);
    case SelftestParts::FirstLayer:
    case SelftestParts::FirstLayerQuestions:
        return _(en_firstlay);
    case SelftestParts::_none:
        break;
    }
    return string_view_utf8::MakeCPUFLASH((uint8_t *)error);
}

const img::Resource *ScreenSelftest::getIconId(SelftestParts part) {
    switch (part) {
    case SelftestParts::Axis:
#if HAS_LOADCELL()
    case SelftestParts::Loadcell:
#endif
#if FILAMENT_SENSOR_IS_ADC()
    case SelftestParts::FSensor:
#endif
#if HAS_GEARS_CALIBRATION()
    case SelftestParts::GearsCalib:
#endif
    case SelftestParts::Heaters:
    case SelftestParts::CalibZ:
    case SelftestParts::FirstLayer:
    case SelftestParts::FirstLayerQuestions:
    case SelftestParts::RevisePrinterSetup:
#if BOARD_IS_XLBUDDY()
    case SelftestParts::Dock:
    case SelftestParts::ToolOffsets:
#endif
        return &img::selftest_16x16;
    case SelftestParts::_none:
        break;
    }
    return &img::error_16x16;
}

void ScreenSelftest::InitState(screen_init_variant var) {
    auto val = var.GetSelftestMask();
    if (val) {
        marlin_client::test_start(*val);
    }
}
