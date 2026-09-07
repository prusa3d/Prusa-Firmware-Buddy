#include "standard_frame/frame_extensions/with_footer.hpp"
#include <feature/door_sensor_calibration/screen_door_sensor_calibration.hpp>

#include <i18n.h>
#include <gui/qr.hpp>
#include <window_text.hpp>
#include <window_icon.hpp>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <option/has_door_sensor_calibration.h>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_qr_prompt.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_calibration_text_with_image.hpp>

static_assert(HAS_DOOR_SENSOR_CALIBRATION(), "Doesn't support door sensor calibration");

namespace {

constexpr auto txt_confirm_abort = N_("Are you sure you want to skip the door sensor calibration for now?");
constexpr auto txt_repeat = N_("Adjusting the tensioning screw of the door sensor was not successful.\n\nPlease repeat the last step.");
constexpr auto txt_skip_ask = N_("Calibration of door sensor is only necessary for user-assembled printers or services door sensor. In all other cases you can skip this step.");
constexpr auto txt_confirm_closed = N_("Is the chamber door closed?\n\nIf not, please close it and confirm.");
constexpr auto txt_tighten_screw_half = N_("Open the door and tighten the tensioning screw by about a half turn. Then close the door and confirm.");
constexpr auto txt_confirm_open = N_("Now fully open the door and confirm that it is open.");
constexpr auto txt_loosen_screw_half = N_("Open the door and loosen the tensioning screw by about a half turn. Then leave the door open and confirm.");
constexpr auto txt_loosen_screw_quarter = N_("Open the door and loosen the tensioning screw by about a quarter turn. Then close the door and confirm.");
constexpr auto txt_ask_enable = N_("Do you want to enable door sensor safety features?");
constexpr auto txt_caution = N_("Caution!");
constexpr auto txt_warn_door_sensor = N_("Disabling the door sensor may lead to injury or printer damage.\nProceeding means you accept full responsibility.\nWe are not liable for any harm or damages.");
constexpr auto txt_done = N_("The door sensor is successfully calibrated now.");

constexpr auto qr_suffix = "core-door-sensor-cal"_tstr;

class FrameFingerTest final : FrameCalibrationTextWithImage {
public:
    FrameFingerTest(window_frame_t *parent)
        : FrameCalibrationTextWithImage {
            parent,
            PhaseDoorSensorCalibration::finger_test,
            _("Grab the door in the middle, place your fingers behind it, and close it as shown in the picture. Then confirm."),
            WizardDefaults::row_1,
            &img::door_sensor_calibration_170x170,
            170
        } {
    }
};

using Frames = FrameDefinitionList<ScreenDoorSensorCalibration::FrameStorage,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_abort, FrameTextPrompt, PhaseDoorSensorCalibration::confirm_abort, txt_confirm_abort>,
    FrameDefinition<PhaseDoorSensorCalibration::repeat, FrameTextPrompt, PhaseDoorSensorCalibration::repeat, txt_repeat>,
    FrameDefinition<PhaseDoorSensorCalibration::skip_ask, FrameQRPrompt, PhaseDoorSensorCalibration::skip_ask, txt_skip_ask, qr_suffix>,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_closed, FrameTextPrompt, PhaseDoorSensorCalibration::confirm_closed, txt_confirm_closed>,
    FrameDefinition<PhaseDoorSensorCalibration::tighten_screw_half, FrameQRPrompt, PhaseDoorSensorCalibration::tighten_screw_half, txt_tighten_screw_half, qr_suffix>,
    FrameDefinition<PhaseDoorSensorCalibration::confirm_open, FrameTextPrompt, PhaseDoorSensorCalibration::confirm_open, txt_confirm_open>,
    FrameDefinition<PhaseDoorSensorCalibration::loosen_screw_half, FrameQRPrompt, PhaseDoorSensorCalibration::loosen_screw_half, txt_loosen_screw_half, qr_suffix>,
    FrameDefinition<PhaseDoorSensorCalibration::finger_test, FrameFingerTest>,
    FrameDefinition<PhaseDoorSensorCalibration::loosen_screw_quarter, FrameQRPrompt, PhaseDoorSensorCalibration::loosen_screw_quarter, txt_loosen_screw_quarter, qr_suffix>,
    FrameDefinition<PhaseDoorSensorCalibration::ask_enable_safety_features, FrameQRPrompt, PhaseDoorSensorCalibration::ask_enable_safety_features, txt_ask_enable, qr_suffix>,
    FrameDefinition<PhaseDoorSensorCalibration::warn_disabled_sensor, FramePrompt, PhaseDoorSensorCalibration::warn_disabled_sensor, txt_caution, txt_warn_door_sensor>,
    FrameDefinition<PhaseDoorSensorCalibration::done, FrameTextPrompt, PhaseDoorSensorCalibration::done, txt_done>>;
} /* namespace */

ScreenDoorSensorCalibration::ScreenDoorSensorCalibration()
    : ScreenFSM { N_("DOOR SENSOR CALIBRATION"), GuiDefaults::RectScreenNoHeader } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenDoorSensorCalibration::~ScreenDoorSensorCalibration() {
    destroy_frame();
}

void ScreenDoorSensorCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenDoorSensorCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenDoorSensorCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
