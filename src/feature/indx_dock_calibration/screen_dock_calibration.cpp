#include "screen_dock_calibration.hpp"
#include "dialog_dock_select.hpp"
#include "dock_calibration_data.hpp"

#include <i18n.h>
#include <guiconfig/wizard_config.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_title_text_image_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <common/fsm_base_types.hpp>
#include <common/encoded_fsm_response.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>
#include <marlin_vars.hpp>
#include <module/prusa/toolchanger.h>
#include <string_view_utf8.hpp>

namespace {

constexpr auto txt_title_intro = N_("Dock Calibration");
constexpr auto txt_intro = N_("The printer will guide you through calibrating the docks. You will need to manually position the head at each dock starting from the left.");
constexpr auto txt_remove_tool = N_("A tool is currently detected on the head.\n\nPlease remove the tool manually and try again.");
constexpr auto txt_homing = N_("Homing XY axes");
constexpr auto txt_moving_away = N_("Lowering bed for clearance");
constexpr auto txt_parking_tool = N_("Parking tool");
constexpr auto txt_lock_position = N_("Motors are now locked.\n\nEnsure your hands are outside the printer enclosure.\n\nVerify the head is in the correct position, then press Continue to start measuring.");
constexpr auto txt_measuring = N_("Measuring dock position\n\nDo not touch the printer.");
constexpr auto txt_success = N_("Dock positions have been successfully calibrated and saved.");
constexpr auto txt_failed = N_("Dock %d calibration failed.\n\nMeasured: X=%.1f Y=%.1f\n\nExpected: X=%.1f Y=%.1f\nTolerance: X=+-%.1f Y=+-%.1f");

constexpr auto txt_select_dock_count = N_("How many docks does your printer have?");
constexpr auto txt_title_ask_position = N_("Calibrating Dock %d");
constexpr auto txt_ask_position = N_("Please manually move the head precisely to dock %d. Make sure it is all the way to the front.\n\nWhile holding it there, press Continue. Don't worry, the printer will lock motors and ask before doing any moves.");

constexpr auto txt_title_tighten_silver_screws = N_("Tighten Dock Screws");
constexpr auto txt_tighten_silver_screws = N_("Tighten the silver screws on each selected dock for calibration. When finished, press Continue.");

constexpr auto txt_title_loosen_each_screw = N_("Loosen Dock Bolts");
constexpr auto txt_loosen_each_screw = N_("Loosen each silver screw by exactly one turn.\n\nWhen done, press Continue.");

/// Parameters for the select_docks dialog, extracted from FSM PhaseData
struct DockSelectParams {
    uint8_t dock_count;
    bool preselect_all;
};

/// Read dock_count and preselect_all from the current FSM PhaseData
/// (set by the server when entering select_docks: data[0]=dock_count, data[1]=preselect_all)
static DockSelectParams get_dock_select_params_from_fsm() {
    return marlin_vars().peek_fsm_states([](const auto &states) -> DockSelectParams {
        const auto &state = states[ClientFSM::DockCalibration];
        if (state.has_value()) {
            const auto data = state->GetData();
            if (data[0] > 0) {
                return { .dock_count = data[0], .preselect_all = data[1] != 0 };
            }
        }
        return { .dock_count = PhysicalToolIndex::count, .preselect_all = false };
    });
}

/// Frame that opens a dock selection dialog and sends the result as FSMResponseVariant
class FrameSelectDocks final {
public:
    FrameSelectDocks(window_frame_t) {
        const auto params = get_dock_select_params_from_fsm();
        const auto result = select_docks_dialog(params.dock_count, params.preselect_all);
        if (result.has_value()) {
            marlin_client::FSM_response_variant(PhaseDockCalibration::select_docks, FSMResponseVariant::make<DockSelection>(*result));
        } else {
            marlin_client::FSM_response_variant(PhaseDockCalibration::select_docks, FSMResponseVariant::make<Response>(Response::Abort));
        }
    }
};

/// Frame that displays dock-specific text with the dock number
class FrameDockPosition : public FramePrompt {
public:
    FrameDockPosition(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt)
        : FramePrompt(parent, fsm_phase, string_view_utf8::MakeNULLSTR(), txt) {}

    void update(fsm::PhaseData data) {
        const uint8_t dock_index = data[0];
        title.SetText(_(txt_title_ask_position).formatted(title_params, dock_index + 1));
        info.SetText(_(txt_ask_position).formatted(params, dock_index + 1));
        title_line.Show();
    }

private:
    StringViewUtf8Parameters<4> title_params;
    StringViewUtf8Parameters<4> params;
};

/// Frame that displays calibration failure details with measured vs expected values
class FrameCalibrationFailed : public FrameTextPrompt {
public:
    FrameCalibrationFailed(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt)
        : FrameTextPrompt(parent, fsm_phase, txt) {}

    void update(fsm::PhaseData data) {
        const auto d = DockCalibrationFailedData::deserialize(data);
        const float expected_x = PrusaToolChanger::DOCK_DEFAULT_X_MM[d.dock_index];
        const float expected_y = PrusaToolChanger::DOCK_DEFAULT_Y_MM;

        info.SetText(_(txt_failed).formatted(params, d.dock_index.to_raw() + 1, static_cast<double>(d.measured_x), static_cast<double>(d.measured_y), static_cast<double>(expected_x), static_cast<double>(expected_y), static_cast<double>(PrusaToolChanger::DOCK_INVALID_OFFSET_X_MM), static_cast<double>(PrusaToolChanger::DOCK_INVALID_OFFSET_Y_MM)));
    }

private:
    StringViewUtf8Parameters<48> params;
};

static constexpr const img::Resource *img_tighten_silver_screws = &img::indx_dock_calibration_tightening;
static constexpr const img::Resource *img_loosen_each_screw = &img::indx_dock_calibration_loosening;

class FrameScrewInstructions : public FrameTitleTextImagePrompt {
public:
    FrameScrewInstructions(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, const img::Resource *img_res)
        : FrameTitleTextImagePrompt(parent, fsm_phase, txt_title, txt_info, img_res) {
        icon.SetAlignment(Align_t::CenterTop());
    }
};

using Frames = FrameDefinitionList<ScreenDockCalibration::FrameStorage,
    FrameDefinition<PhaseDockCalibration::intro, FramePrompt, PhaseDockCalibration::intro, txt_title_intro, txt_intro>,
    FrameDefinition<PhaseDockCalibration::remove_tool, FrameTextPrompt, PhaseDockCalibration::remove_tool, txt_remove_tool>,
    FrameDefinition<PhaseDockCalibration::select_dock_count, FrameTextPrompt, PhaseDockCalibration::select_dock_count, txt_select_dock_count>,
    FrameDefinition<PhaseDockCalibration::select_docks, FrameSelectDocks>,
    FrameDefinition<PhaseDockCalibration::homing, FrameWait, txt_homing>,
    FrameDefinition<PhaseDockCalibration::moving_away, FrameWait, txt_moving_away>,
    FrameDefinition<PhaseDockCalibration::parking_tool, FrameWait, txt_parking_tool>,
    FrameDefinition<PhaseDockCalibration::tighten_silver_screws, FrameScrewInstructions, PhaseDockCalibration::tighten_silver_screws, txt_title_tighten_silver_screws, txt_tighten_silver_screws, img_tighten_silver_screws>,
    FrameDefinition<PhaseDockCalibration::ask_position_dock, FrameDockPosition, PhaseDockCalibration::ask_position_dock, txt_intro /* unused, overridden by update */>,
    FrameDefinition<PhaseDockCalibration::lock_position, FrameTextPrompt, PhaseDockCalibration::lock_position, txt_lock_position>,
    FrameDefinition<PhaseDockCalibration::measuring, FrameWait, txt_measuring>,
    FrameDefinition<PhaseDockCalibration::loosen_each_bolt, FrameScrewInstructions, PhaseDockCalibration::loosen_each_bolt, txt_title_loosen_each_screw, txt_loosen_each_screw, img_loosen_each_screw>,
    FrameDefinition<PhaseDockCalibration::calibration_success, FrameTextPrompt, PhaseDockCalibration::calibration_success, txt_success>,
    FrameDefinition<PhaseDockCalibration::calibration_failed, FrameCalibrationFailed, PhaseDockCalibration::calibration_failed, txt_failed>>;

} // namespace

ScreenDockCalibration::ScreenDockCalibration()
    : ScreenFSM { N_("DOCK CALIBRATION"), GuiDefaults::RectScreenNoHeader } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenDockCalibration::~ScreenDockCalibration() {
    destroy_frame();
}

void ScreenDockCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenDockCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenDockCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
