/// @file
#include "screen_opt_flashing.hpp"

#include <ScreenHandler.hpp>
#include <common/marlin_client.hpp>
#include <common/marlin_vars.hpp>
#include <puppies/cyphal_bridge_host.hpp>
#include <puppies/cyphal_node_table_host.hpp>
#include <timing.h>
#include <xbuddy_extension/shared_enums.hpp>

namespace buddy::openprinttag {

namespace {
    constexpr uint32_t stall_after_ms = 5000;
    constexpr uint32_t close_after_done_ms = 2000;
} // namespace

ScreenOPTFlashing::ScreenOPTFlashing()
    : header_ { this, _("OPENPRINTTAG READER") }
    , frame_ { this }
    , stall_timer_ { stall_after_ms }
    , done_timer_ { close_after_done_ms } {

    frame_.radio().set_callback([](Response) { Screens::Access()->close_screen(); });
    stall_timer_.restart(ticks_ms());
}

bool ScreenOPTFlashing::can_popup() {
    // Don't bother the user while printing or processing gcode (especially during loads/unloads)
    return marlin_client::is_idle() && !marlin_vars().is_processing;
}

void ScreenOPTFlashing::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update();
    }

    screen_t::windowEvent(sender, event, param);
}

void ScreenOPTFlashing::update() {
    using xbuddy_extension::NodeName;
    using xbuddy_extension::NodeState;

    const auto now = ticks_ms();

    if (finished_) {
        if (done_timer_.check(now)) {
            Screens::Access()->close_screen();
        }
        return;
    }

    // TODO: Tell apart several ANFC nodes (BFW-9360)
    switch (puppies::cyphal_node_table().get_node_state(NodeName::cz_prusa3d_honeybee_nfc)) {
    case NodeState::flash: {
        const auto &bridge = puppies::cyphal_bridge_host();
        if (bridge.get_flash_file_id() != xbuddy_extension::firmware_file_id(NodeName::cz_prusa3d_honeybee_nfc)) {
            // The bridge is flashing another node first
            stall_timer_.restart(now);
            set_stalled(false);
        } else if (const uint8_t percent = bridge.get_flash_progress_percent(); percent != last_percent_) {
            last_percent_ = percent;
            frame_.set_progress(last_percent_);
            stall_timer_.restart(now);
            set_stalled(false);
        } else if (stall_timer_.check(now)) {
            set_stalled(true);
        }
        break;
    }

    case NodeState::unknown:
        // Can be just a communication glitch, give the reader a chance to show up again
        if (stall_timer_.check(now)) {
            set_stalled(true);
        }
        break;

    case NodeState::verify:
    case NodeState::ready:
        // From the user's point of view the update is done
        finished_ = true;
        frame_.set_done();
        done_timer_.restart(now);
        break;
    }
}

void ScreenOPTFlashing::set_stalled(bool stalled) {
    if (stalled != stalled_) {
        stalled_ = stalled;
        frame_.set_stalled(stalled);
    }
}

} // namespace buddy::openprinttag
