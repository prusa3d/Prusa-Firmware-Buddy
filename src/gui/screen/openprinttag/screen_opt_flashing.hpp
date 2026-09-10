/// @file
#pragma once

#include "frame_opt_flashing.hpp"

#include <screen.hpp>
#include <utils/timing/timer.hpp>
#include <window_header.hpp>

namespace buddy::openprinttag {

/// Pops up while the NFC reader firmware is being updated.
/// Closes itself shortly after the update ends, or on Ok.
class ScreenOPTFlashing final : public screen_t {

public:
    ScreenOPTFlashing();

    /// Whether the GUI is in a good state to show the popup
    [[nodiscard]] static bool can_popup();

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    void update();
    void set_stalled(bool stalled);

private:
    window_header_t header_;
    FrameOPTFlashing frame_;

private:
    utils::Timer<uint32_t> stall_timer_; ///< restarted whenever the percentage moves
    utils::Timer<uint32_t> done_timer_; ///< started once the update is over
    uint8_t last_percent_ = 0;
    bool stalled_ = false;
    bool finished_ = false;
};

} // namespace buddy::openprinttag
