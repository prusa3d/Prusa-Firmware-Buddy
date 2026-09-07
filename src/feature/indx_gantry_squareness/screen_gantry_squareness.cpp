/// @file
#include "screen_gantry_squareness.hpp"

#include "indx_gantry_squareness.hpp"
#include <common/fsm_base_types.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <i18n.h>
#include <img_resources.hpp>
#include <gui/auto_layout.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_qr_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <string_view_utf8.hpp>
#include <window_icon.hpp>

namespace {

constexpr auto txt_title = N_("Gantry Squareness Check");
constexpr auto txt_intro = N_("The printer will measure the gantry skew by touching the outermost docks with the empty printhead.");
constexpr auto txt_preparing = N_("Preparing the printer, please wait.");
constexpr auto txt_remove_nozzles = N_("Remove the nozzles from docks 1 and 8 (if you have them), then press continue.");

constexpr auto txt_reinsert_nozzles = N_("Put the nozzles back into docks 1 and 8 (if you have them).");

constexpr const img::Resource *img_remove_nozzles = &img::gantry_squareness_remove;
constexpr const img::Resource *img_reinsert_nozzles = &img::gantry_squareness_insert;

// FramePrompt's layout with the dock-row image squeezed between the text and the radio
constexpr std::array nozzle_image_layout {
    StackLayoutItem { .height = 32, .margin_side = 16, .margin_bottom = 4 }, // Title
    StackLayoutItem { .height = 2, .margin_side = 16 }, // Line below title
    StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 8 }, // Text
    StackLayoutItem { .height = 128, .margin_bottom = 8 }, // Dock-row image (80 % of the display width)
    standard_stack_layout::for_radio,
};

/// FramePrompt with the dock-row image below the text
class FrameNozzleImagePrompt : public FramePrompt {
public:
    FrameNozzleImagePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, const img::Resource *img_res)
        : FramePrompt(parent, fsm_phase, txt_title, txt_info)
        , icon(parent, Rect16(), img_res) {
        icon.SetAlignment(Align_t::Center());

        std::array<window_t *, nozzle_image_layout.size()> windows { &title, &title_line, &info, &icon, &radio };
        layout_vertical_stack(parent->GetRect(), windows, nozzle_image_layout);
    }

private:
    window_icon_t icon;
};
constexpr auto txt_measuring = N_("Measuring the gantry squareness, please wait.");
constexpr auto txt_result_ok = N_("Measured skew: %0.2f mm (limit %0.2f mm)\n\nThe gantry is square.");
constexpr auto txt_result_skewed = N_("Measured skew: %0.2f mm (limit %0.2f mm)\n\nThe gantry is not square. Align it following the guide, then measure again.");
constexpr auto txt_result_error = N_("The measurement failed. Check that the outermost docks are empty, then retry.");

class FrameResultOk : public FramePrompt {
public:
    FrameResultOk(window_frame_t *parent, FSMAndPhase fsm_phase)
        : FramePrompt(parent, fsm_phase, _(txt_title), string_view_utf8::MakeNULLSTR()) {}

    void update(fsm::PhaseData data) {
        const auto result = fsm::deserialize_data<SquarenessData>(data);
        info.SetText(_(txt_result_ok).formatted(params, static_cast<double>(result.dy_um / 1000.f), static_cast<double>(indx_gantry_squareness::dy_limit_mm)));
    }

private:
    StringViewUtf8Parameters<16> params;
};

class FrameResultFailed : public FrameQRPrompt {
public:
    FrameResultFailed(window_frame_t *parent, FSMAndPhase fsm_phase)
        : FrameQRPrompt(parent, fsm_phase, string_view_utf8::MakeNULLSTR(), "core-belt-calibration") {}

    void update(fsm::PhaseData data) {
        const auto result = fsm::deserialize_data<SquarenessData>(data);
        if (!result.valid) {
            set_info_text(_(txt_result_error));
            return;
        }
        set_info_text(_(txt_result_skewed).formatted(params, static_cast<double>(result.dy_um / 1000.f), static_cast<double>(indx_gantry_squareness::dy_limit_mm)));
    }

private:
    StringViewUtf8Parameters<16> params;
};

using Frames = FrameDefinitionList<ScreenGantrySquareness::FrameStorage,
    FrameDefinition<PhaseGantrySquareness::intro, FramePrompt, PhaseGantrySquareness::intro, txt_title, txt_intro>,
    FrameDefinition<PhaseGantrySquareness::preparing, FrameWait, txt_preparing>,
    FrameDefinition<PhaseGantrySquareness::remove_nozzles, FrameNozzleImagePrompt, PhaseGantrySquareness::remove_nozzles, txt_title, txt_remove_nozzles, img_remove_nozzles>,
    FrameDefinition<PhaseGantrySquareness::measuring, FrameWait, txt_measuring>,
    FrameDefinition<PhaseGantrySquareness::reinsert_nozzles, FrameNozzleImagePrompt, PhaseGantrySquareness::reinsert_nozzles, txt_title, txt_reinsert_nozzles, img_reinsert_nozzles>,
    FrameDefinition<PhaseGantrySquareness::result_ok, FrameResultOk, PhaseGantrySquareness::result_ok>,
    FrameDefinition<PhaseGantrySquareness::result_failed, FrameResultFailed, PhaseGantrySquareness::result_failed>>;

} // namespace

ScreenGantrySquareness::ScreenGantrySquareness()
    : ScreenFSM { N_("GANTRY SQUARENESS"), GuiDefaults::RectScreenNoHeader } {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenGantrySquareness::~ScreenGantrySquareness() {
    destroy_frame();
}

void ScreenGantrySquareness::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenGantrySquareness::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenGantrySquareness::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
