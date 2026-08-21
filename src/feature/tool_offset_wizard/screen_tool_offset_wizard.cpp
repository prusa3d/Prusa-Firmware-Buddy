#include "screen_tool_offset_wizard.hpp"

#include "tool_offset_wizard.hpp"
#include <common/fsm_base_types.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <guiconfig/wizard_config.hpp>
#include <i18n.h>
#include <img_resources.hpp>
#include <standard_frame/frame_extensions/with_footer.hpp>
#include <standard_frame/frame_progress_prompt.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <string_view_utf8.hpp>

namespace {

constexpr auto txt_title = N_("Tool Offsets Calibration");
constexpr auto txt_intro = N_("The printer will calibrate the XY/Z offsets of all tools using the tool offset sensor. This may take several minutes.");
constexpr auto txt_ensure_nozzles_clean = N_("Make sure all the nozzles are clean, then press Continue.");
constexpr auto txt_moving_away = N_("Lowering bed for clearance");
constexpr auto txt_picking_tool = N_("Picking up tool");
constexpr auto txt_homing = N_("Homing");
// %u placeholders are filled with the 1-based current tool and the total tool count.
constexpr auto txt_calibrating_progress = N_("Tool %u of %u");
constexpr auto txt_success = N_("Tool offsets have been successfully calibrated and saved.");
constexpr auto txt_failed = N_("Tool offsets calibration failed.");

/// FrameProgressPrompt variant for the in-progress phase: shows progress, Abort radio, and the
/// currently selected nozzle's temperature in the footer.
class FrameCalibratingProgress : public WithFooter<FrameProgressPrompt, { footer::Item::nozzle }> {
public:
    FrameCalibratingProgress(window_frame_t *parent, FSMAndPhase fsm_phase, const char *t_title, const char *t_info_fmt)
        : WithFooter(parent, fsm_phase, _(t_title), string_view_utf8::MakeNULLSTR())
        , fmt_(t_info_fmt) {}

    void update(fsm::PhaseData data) {
        const auto progress = fsm::deserialize_data<tool_offset_wizard::ProgressData>(data);
        if (progress.total_steps == 0) {
            progress_bar.set_progress_percent(0);
            return;
        }
        info.SetText(_(fmt_).formatted(params_, static_cast<unsigned>(progress.step), static_cast<unsigned>(progress.total_steps)));
        // window_text_t::SetText short-circuits when the new string_view points to the same
        // backing buffer (which it does — `params_` is a member). Force a redraw so the new
        // contents of the buffer actually paint.
        info.Invalidate();
        const float percent = (100.0f * static_cast<float>(progress.step)) / static_cast<float>(progress.total_steps);
        progress_bar.set_progress_percent(percent);
    }

private:
    const char *fmt_;
    StringViewUtf8Parameters<20> params_;
};

using Frames = FrameDefinitionList<ScreenToolOffsetWizard::FrameStorage,
    FrameDefinition<PhaseToolOffsetsCalibration::intro, FramePrompt, PhaseToolOffsetsCalibration::intro, txt_title, txt_intro>,
    FrameDefinition<PhaseToolOffsetsCalibration::ensure_nozzles_clean, FramePrompt, PhaseToolOffsetsCalibration::ensure_nozzles_clean, txt_title, txt_ensure_nozzles_clean>,
    FrameDefinition<PhaseToolOffsetsCalibration::moving_away, FrameWait, txt_moving_away>,
    FrameDefinition<PhaseToolOffsetsCalibration::picking_tool, FrameWait, txt_picking_tool>,
    FrameDefinition<PhaseToolOffsetsCalibration::homing, FrameWait, txt_homing>,
    FrameDefinition<PhaseToolOffsetsCalibration::calibrating, FrameCalibratingProgress, PhaseToolOffsetsCalibration::calibrating, txt_title, txt_calibrating_progress>,
    FrameDefinition<PhaseToolOffsetsCalibration::calibration_success, FrameTextPrompt, PhaseToolOffsetsCalibration::calibration_success, txt_success>,
    FrameDefinition<PhaseToolOffsetsCalibration::calibration_failed, FrameTextPrompt, PhaseToolOffsetsCalibration::calibration_failed, txt_failed>>;

} // namespace

ScreenToolOffsetWizard::ScreenToolOffsetWizard()
    : ScreenFSM { N_("TOOL OFFSETS CALIBRATION"), GuiDefaults::RectScreenNoHeader } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenToolOffsetWizard::~ScreenToolOffsetWizard() {
    destroy_frame();
}

void ScreenToolOffsetWizard::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenToolOffsetWizard::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenToolOffsetWizard::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
