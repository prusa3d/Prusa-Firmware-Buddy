#include "screen_indx_nozzle_cleaning.hpp"

#include "indx_nozzle_cleaning.hpp"
#include <i18n.h>
#include <guiconfig/GuiDefaults.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <standard_frame/frame_wait_temp.hpp>
#include <common/fsm_base_types.hpp>
#include <string_view_utf8.hpp>

using Phase = PhaseNozzleCleaning;

namespace {

// Identical wording to other wizards; kept verbatim so they share one POT entry.
constexpr auto txt_picking_tool = N_("Picking up tool");
constexpr auto txt_cleaning = N_("Cleaning nozzle");
constexpr auto txt_parking_tool = N_("Parking tool");
constexpr auto txt_preparing = N_("Preparing");
constexpr auto txt_heating = N_("Heating up the nozzle");
constexpr auto txt_cooling = N_("Cooling the nozzle");
// %d is the tool number
constexpr auto txt_title_tool = N_("TOOL %d");
constexpr auto txt_finished = N_("Done");

constexpr auto txt_manual_clean_hot = N_("The nozzle is hot, be careful!\n\nClean the nozzle manually, then press Continue. The nozzle will cool down for the final cleaning.");
constexpr auto txt_manual_clean_cold = N_("Finish cleaning the nozzle manually, then press Continue. The nozzle cleaner will clean it afterwards.");

uint8_t tool_display_index(const fsm::PhaseData &data) {
    return fsm::deserialize_data<indx_nozzle_cleaning::PhaseDataLayout>(data).tool_display_index;
}

/// FramePrompt with the tool number formatted into the title
class FrameToolPrompt : public FramePrompt {
public:
    FrameToolPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const char *txt_info)
        : FramePrompt(parent, fsm_phase, string_view_utf8::MakeNULLSTR(), _(txt_info)) {}

    void update(fsm::PhaseData data) {
        const uint8_t tool = tool_display_index(data);
        if (tool != shown_tool_) {
            shown_tool_ = tool;
            title.SetText(_(txt_title_tool).formatted(params_, tool));
        }
    }

private:
    uint8_t shown_tool_ = 0;
    StringViewUtf8Parameters<4> params_;
};

using Frames = FrameDefinitionList<ScreenIndxNozzleCleaning::FrameStorage,
    FrameDefinition<Phase::preparing, FrameWait, txt_preparing>,
    FrameDefinition<Phase::picking_tool, FrameWait, txt_picking_tool>,
    FrameDefinition<Phase::heating, FrameWaitTemp, Phase::heating, txt_heating>,
    FrameDefinition<Phase::cleaning, FrameWait, txt_cleaning>,
    FrameDefinition<Phase::manual_clean_hot, FrameToolPrompt, Phase::manual_clean_hot, txt_manual_clean_hot>,
    FrameDefinition<Phase::cooling, FrameWaitTemp, Phase::cooling, txt_cooling>,
    FrameDefinition<Phase::manual_clean_cold, FrameToolPrompt, Phase::manual_clean_cold, txt_manual_clean_cold>,
    FrameDefinition<Phase::parking_tool, FrameWait, txt_parking_tool>,
    FrameDefinition<Phase::finished, FrameTextPrompt, Phase::finished, txt_finished>>;

} // namespace

ScreenIndxNozzleCleaning::ScreenIndxNozzleCleaning()
    : ScreenFSM { N_("NOZZLE CLEANING"), GuiDefaults::RectScreenNoHeader } {
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenIndxNozzleCleaning::~ScreenIndxNozzleCleaning() {
    destroy_frame();
}

void ScreenIndxNozzleCleaning::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenIndxNozzleCleaning::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenIndxNozzleCleaning::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
