#include <feature/door_sensor_calibration/screen_door_sensor_calibration.hpp>

#include <i18n.h>
#include <frame_calibration_common.hpp>
#include <gui/qr.hpp>
#include <window_text.hpp>
#include <window_icon.hpp>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <option/has_door_sensor_calibration.h>

static_assert(HAS_DOOR_SENSOR_CALIBRATION(), "Doesn't support door sensor calibration");

namespace {
class FrameConfirmAbort final : FrameText {
public:
    FrameConfirmAbort(window_t *parent)
        : FrameText {
            parent,
            _("Are you sure you want to skip the door sensor calibration for now?"),
            WizardDefaults::row_1,
        } {}
};

class FrameRepeat final : FrameText {
public:
    FrameRepeat(window_t *parent)
        : FrameText {
            parent,
            _("Adjusting the tensioning screw of the door sensor was not successful. Please repeat the last step."),
            WizardDefaults::row_1,
        } {}
};

class FrameSkipAsk final : FrameTextWithQR {
public:
    FrameSkipAsk(window_t *parent)
        : FrameTextWithQR {
            parent,
            _("Calibration of door sensor is only necessary for user-assembled printers or services door sensor. In all other cases you can skip this step."),
            WizardDefaults::row_1,
            "http://prusa.io/core-door-sensor-calibration",
        } {}
};

class FrameConfirmClosed final : FrameText {
public:
    FrameConfirmClosed(window_t *parent)
        : FrameText {
            parent,
            _("Is the chamber door closed? If not, please close it and confirm."),
            WizardDefaults::row_1,
        } {}
};

class FrameTightenScrewHalf final : FrameTextWithQR {
public:
    FrameTightenScrewHalf(window_t *parent)
        : FrameTextWithQR {
            parent,
            _("Open the door and tighten the tensioning screw by about a half turn. Then close the door and confirm."),
            WizardDefaults::row_1,
            "http://prusa.io/core-door-sensor-calibration",
        } {}
};

class FrameConfirmOpen final : FrameText {
public:
    FrameConfirmOpen(window_t *parent)
        : FrameText {
            parent,
            _("Now fully open the door and confirm that it is open."),
            WizardDefaults::row_1,
        } {}
};

class FrameLoosenScrewHalf final : FrameTextWithQR {
public:
    FrameLoosenScrewHalf(window_t *parent)
        : FrameTextWithQR {
            parent,
            _("Open the door and loosen the tensioning screw by about a half turn. Then leave the door open and confirm."),
            WizardDefaults::row_1,
            "http://prusa.io/core-door-sensor-calibration",
        } {}
};

class FrameFingerTest final : FrameTextWithImage {
public:
    FrameFingerTest(window_t *parent)
        : FrameTextWithImage {
            parent,
            _("Grab the door in the middle, place your fingers behind it, and close it as shown in the picture. Then confirm."),
            WizardDefaults::row_1,
            &img::door_sensor_calibration_170x170,
            170,
        } {}
};

class FrameTightenScrewQuarter final : FrameTextWithQR {
public:
    FrameTightenScrewQuarter(window_t *parent)
        : FrameTextWithQR {
            parent,
            _("Open the door and tighten the tensioning screw by about a quarter turn. Then close the door and confirm."),
            WizardDefaults::row_1,
            "http://prusa.io/core-door-sensor-calibration",
        } {}
};

class FrameDone final : FrameText {
public:
    FrameDone(window_t *parent)
        : FrameText {
            parent,
            _("The door sensor is successfully calibrated now."),
            WizardDefaults::row_1,
        } {}
};

PhaseDoorSensorCalibration get_phase(const fsm::BaseData &fsm_base_data) {
    return GetEnumFromPhaseIndex<PhaseDoorSensorCalibration>(fsm_base_data.GetPhase());
}

using Frames = FrameDefinitionList<ScreenDoorSensorCalibration::FrameStorage,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_abort, FrameConfirmAbort>,
    FrameDefinition<PhaseDoorSensorCalibration::repeat, FrameRepeat>,
    FrameDefinition<PhaseDoorSensorCalibration::skip_ask, FrameSkipAsk>,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_closed, FrameConfirmClosed>,
    FrameDefinition<PhaseDoorSensorCalibration::tighten_screw_half, FrameTightenScrewHalf>,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_open, FrameConfirmOpen>,
    FrameDefinition<PhaseDoorSensorCalibration::loosen_screw_half, FrameLoosenScrewHalf>,
    FrameDefinition<PhaseDoorSensorCalibration::finger_test, FrameFingerTest>,
    FrameDefinition<PhaseDoorSensorCalibration::tighten_screw_quarter, FrameTightenScrewQuarter>,
    FrameDefinition<PhaseDoorSensorCalibration::done, FrameDone>>;

} /* namespace */

ScreenDoorSensorCalibration::ScreenDoorSensorCalibration()
    : ScreenFSM { N_("DOOR SENSOR CALIBRATION"), rect_screen }
    , radio(this, rect_radio, PhaseDoorSensorCalibration::finish) {
    CaptureNormalWindow(radio);
    create_frame();
}

ScreenDoorSensorCalibration::~ScreenDoorSensorCalibration() {
    destroy_frame();
}

void ScreenDoorSensorCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(fsm_base_data), this);
    radio.set_fsm_and_phase(get_phase(fsm_base_data));
    radio.Invalidate();
}

void ScreenDoorSensorCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase(fsm_base_data));
}

void ScreenDoorSensorCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(fsm_base_data), fsm_base_data.GetData());
}
