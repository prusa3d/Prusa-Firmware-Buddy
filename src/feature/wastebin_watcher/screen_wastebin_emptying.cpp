/// @file
#include "screen_wastebin_emptying.hpp"

#include <feature/wastebin_watcher/wastebin_watcher.hpp>
#include <fsm/wastebin_emptying_mapper.hpp>
#include <fsm/wastebin_emptying_phases.hpp>
#include <i18n.h>
#include <standard_frame/frame_qr_prompt.hpp>
#include <standard_frame/frame_wait.hpp>

using Phase = PhaseWastebinEmptying;

namespace {

/// Adds the number of remaining tool changes, which the static error text cannot carry.
class FrameWastebinFull : public FrameQRPrompt {
public:
    FrameWastebinFull(window_frame_t *parent)
        : FrameQRPrompt(parent, Phase::prompt_full, wastebin_emptying_phase_error_code_mapper) {
        // Without a toolchange count in the gcode there is nothing to add, keep the error text.
        if (const std::optional<uint32_t> remaining = WastebinWatcher::instance().expected_remaining_pellets()) {
            set_info_text(_("Nozzle cleaner is full.\nTool changes remaining: %u.").formatted(params_, static_cast<unsigned>(*remaining)));
        }
    }

private:
    StringViewUtf8Parameters<8> params_;
};

using Frames = FrameDefinitionList<ScreenWastebinEmptying::FrameStorage,
    FrameDefinition<Phase::parking, FrameWait, N_("Parking")>,
    FrameDefinition<Phase::prompt_full, FrameWastebinFull>,
    FrameDefinition<Phase::prompt_empty, FrameQRPrompt, Phase::prompt_empty, wastebin_emptying_phase_error_code_mapper>,
    FrameDefinition<Phase::raising_bed, FrameWait, N_("Raising the bed")>,
    FrameDefinition<Phase::heating, FrameWait, N_("Heating up the nozzle")>,
    FrameDefinition<Phase::purging, FrameWait, N_("Purging the nozzle")>,
    FrameDefinition<Phase::returning, FrameWait, N_("Returning to the print")>>;

} // namespace

ScreenWastebinEmptying::ScreenWastebinEmptying()
    : ScreenFSM(N_("NOZZLE CLEANER")) {
    create_frame();
}

ScreenWastebinEmptying::~ScreenWastebinEmptying() {
    destroy_frame();
}

void ScreenWastebinEmptying::create_frame() {
    Frames::create_frame(frame_storage, GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase()), &inner_frame);
}

void ScreenWastebinEmptying::destroy_frame() {
    Frames::destroy_frame(frame_storage, GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase()));
}

void ScreenWastebinEmptying::update_frame() {
    Frames::update_frame(frame_storage, GetEnumFromPhaseIndex<Phase>(fsm_base_data.GetPhase()), fsm_base_data.GetData());
}
