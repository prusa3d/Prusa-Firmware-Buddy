#include "screen_nozzle_cleaner_calibration.hpp"

#include "indx_nozzle_cleaner_calibration.hpp"
#include <i18n.h>
#include <guiconfig/wizard_config.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_title_text_image_prompt.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <standard_frame/frame_wait_temp.hpp>
#include <standard_frame/frame_extensions/with_footer.hpp>
#include <footer_line.hpp>
#include <common/fsm_base_types.hpp>
#include <img_resources.hpp>
#include <string_view_utf8.hpp>
#include <gui/auto_layout.hpp>
#include <array>
#include <cstdio>

namespace {

constexpr auto txt_title_intro = N_("Nozzle Cleaner Calibration");
constexpr auto txt_intro = N_("The printer will guide you through calibrating the nozzle cleaner. You will need to adjust the height screw.");
constexpr auto txt_wait_for_nozzle_cooldown = N_("Wait!\n\nThe nozzle is still cooling down.");
constexpr auto txt_picking_tool = N_("Picking up tool");
constexpr auto txt_homing = N_("Homing XY axes");
constexpr auto txt_moving_away = N_("Lowering bed for clearance");
// %c is the axis letter ('X', 'Y' or 'Z')
constexpr auto txt_title_axis_alignment = N_("Nozzle %c-Axis Alignment");
// Identical wording to dock_calibration's txt_lock_position; kept verbatim so both share one POT entry.
constexpr auto txt_lock_position = N_("Motors are now locked.\n\nEnsure your hands are outside the printer enclosure.\n\nVerify the head is in the correct position, then press Continue to start measuring.");
constexpr auto txt_move_to_z_point = N_("Position the nozzle in the center of the silicone V-groove as shown.\n\nTurn the adjustment screw counterclockwise until it touches the silicone.");
constexpr auto txt_ask_position_x = N_("Now we will measure the X offset of the nozzle cleaner.\n\nMove the head to the center of the silicone V-groove so the nozzle lightly touches the silicone, as shown in the picture.");
constexpr auto txt_ask_position_y = N_("Move the nozzle precisely to the Y-axis calibration indent on the side of the nozzle cleaner bin.\n\nThen press Continue.");
// %c is the axis letter ('X' or 'Y')
constexpr auto txt_measuring = N_("Measuring %c position\n\nDo not touch the printer.");
constexpr auto txt_success = N_("Nozzle cleaner calibrated.");
constexpr auto txt_clean_nozzle = N_("The nozzle diameter was measured wider than expected.\n\nClean the nozzle, then press Retry.");

// %s is the measured offset, formatted as "<value> mm" or "N/A" when the nozzle never made contact.
constexpr auto txt_evaluating_failed = N_("Calibration failed.\n\nNominal: %.1f mm (+/- %hu mm)\n\nMeasured offset: %s\n\nCalibrate manually?");
constexpr uint8_t max_offset_mm = 3;

/// FrameWait variant where a single %c in the text is replaced by an axis letter.
class FrameWaitWithAxis : public FrameWait {
public:
    FrameWaitWithAxis(window_frame_t *parent, const char *txt, char axis)
        : FrameWait(parent, string_view_utf8::MakeNULLSTR()) {
        set_text(_(txt).formatted(params_, axis));
    }

private:
    StringViewUtf8Parameters<2> params_;
};

/// FrameTitleTextImagePrompt variant where a single %c in the title is replaced by an axis letter.
class FrameTitleTextImagePromptWithAxis : public FrameTitleTextImagePrompt {
public:
    FrameTitleTextImagePromptWithAxis(window_frame_t *parent, FSMAndPhase fsm_phase, const char *txt_title, const string_view_utf8 &txt_info, const img::Resource *img_res, char axis)
        : FrameTitleTextImagePrompt(parent, fsm_phase, _(txt_title), txt_info, img_res) {
        title.SetText(_(txt_title).formatted(title_params_, axis));
    }

private:
    StringViewUtf8Parameters<2> title_params_;
};

/// Frame for evaluating phases - shows measured offset, nominal value and valid range on failure
class FrameEvaluating {
public:
    FrameEvaluating(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &)
        : info(parent, {}, is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeNULLSTR())
        , radio(parent, {}, fsm_phase) {

        info.SetAlignment(Align_t::LeftCenter());

        parent->CaptureNormalWindow(radio);

        static constexpr std::array layout {
            StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 16 },
            standard_stack_layout::for_radio,
        };
        std::array<window_t *, layout.size()> windows { &info, &radio };
        layout_vertical_stack(parent->GetRect(), windows, layout);
    }

    void update(fsm::PhaseData data) {
        const auto eval_data = fsm::deserialize_data<indx_nozzle_cleaner_calibration::EvaluatingData>(data);

        // The offset is absent when the nozzle never touched (probe did not trigger); show "N/A" instead
        // of a misleading 0.00 mm. formatted() copies the parameter, so the local buffer may be temporary.
        const auto offset = eval_data.offset();
        std::array<char, 16> offset_str;
        if (offset.has_value()) {
            snprintf(offset_str.data(), offset_str.size(), "%.2f mm", static_cast<double>(*offset));
        }
        const char *offset_text = offset.has_value() ? offset_str.data() : "N/A";

        info.SetText(_(txt_evaluating_failed).formatted(params, static_cast<double>(eval_data.nominal()), max_offset_mm, offset_text));
    }

private:
    window_text_t info;
    RadioButtonFSM radio;
    StringViewUtf8Parameters<60> params;
};

static constexpr const img::Resource *img_ask_position_x = &img::cleaner_calibration_x;
static constexpr const img::Resource *img_ask_position_y = &img::cleaner_calibration_y;
static constexpr const img::Resource *img_move_to_z_point = &img::cleaner_calibration_z;

/// Footer items shown on the nozzle cooldown phase
constexpr FooterLine::IdArray cooldown_footer_items = { footer::Item::nozzle };

using Frames = FrameDefinitionList<ScreenNozzleCleanerCalibration::FrameStorage,
    FrameDefinition<PhaseNozzleCleanerCalibration::intro, FramePrompt, PhaseNozzleCleanerCalibration::intro, txt_title_intro, txt_intro>,
    FrameDefinition<PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown, WithFooter<FrameWaitTemp, cooldown_footer_items>, PhaseNozzleCleanerCalibration::wait_for_nozzle_cooldown, txt_wait_for_nozzle_cooldown>,
    FrameDefinition<PhaseNozzleCleanerCalibration::picking_tool, FrameWait, txt_picking_tool>,
    FrameDefinition<PhaseNozzleCleanerCalibration::homing, FrameWait, txt_homing>,
    FrameDefinition<PhaseNozzleCleanerCalibration::moving_away, FrameWait, txt_moving_away>,
    FrameDefinition<PhaseNozzleCleanerCalibration::move_to_z_point, FrameTitleTextImagePromptWithAxis, PhaseNozzleCleanerCalibration::move_to_z_point, txt_title_axis_alignment, txt_move_to_z_point, img_move_to_z_point, 'Z'>,
    FrameDefinition<PhaseNozzleCleanerCalibration::ask_position_x, FrameTitleTextImagePromptWithAxis, PhaseNozzleCleanerCalibration::ask_position_x, txt_title_axis_alignment, txt_ask_position_x, img_ask_position_x, 'X'>,
    FrameDefinition<PhaseNozzleCleanerCalibration::lock_position_x, FrameTextPrompt, PhaseNozzleCleanerCalibration::lock_position_x, txt_lock_position>,
    FrameDefinition<PhaseNozzleCleanerCalibration::measuring_x, FrameWaitWithAxis, txt_measuring, 'X'>,
    FrameDefinition<PhaseNozzleCleanerCalibration::evaluating_x, FrameEvaluating, PhaseNozzleCleanerCalibration::evaluating_x, txt_evaluating_failed>,
    FrameDefinition<PhaseNozzleCleanerCalibration::clean_nozzle, FrameTextPrompt, PhaseNozzleCleanerCalibration::clean_nozzle, txt_clean_nozzle>,
    FrameDefinition<PhaseNozzleCleanerCalibration::ask_position_y, FrameTitleTextImagePromptWithAxis, PhaseNozzleCleanerCalibration::ask_position_y, txt_title_axis_alignment, txt_ask_position_y, img_ask_position_y, 'Y'>,
    FrameDefinition<PhaseNozzleCleanerCalibration::lock_position_y, FrameTextPrompt, PhaseNozzleCleanerCalibration::lock_position_y, txt_lock_position>,
    FrameDefinition<PhaseNozzleCleanerCalibration::measuring_y, FrameWaitWithAxis, txt_measuring, 'Y'>,
    FrameDefinition<PhaseNozzleCleanerCalibration::evaluating_y, FrameEvaluating, PhaseNozzleCleanerCalibration::evaluating_y, txt_evaluating_failed>,
    FrameDefinition<PhaseNozzleCleanerCalibration::calibration_success, FrameTextPrompt, PhaseNozzleCleanerCalibration::calibration_success, txt_success>>;

} // namespace

ScreenNozzleCleanerCalibration::ScreenNozzleCleanerCalibration()
    : ScreenFSM { N_("NOZZLE CLEANER CALIBRATION"), GuiDefaults::RectScreenNoHeader } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenNozzleCleanerCalibration::~ScreenNozzleCleanerCalibration() {
    destroy_frame();
}

void ScreenNozzleCleanerCalibration::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenNozzleCleanerCalibration::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenNozzleCleanerCalibration::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
