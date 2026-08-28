/// @file
#pragma once

#include <screen.hpp>
#include <window_header.hpp>
#include <utils/timing/timer.hpp>

namespace buddy::openprinttag {

/// Base class for ScreenOPTDetected and ScreenOPTLost
class ScreenOPTPrompt : public screen_t {

public:
    /// Whether the GUI is in a good state to show the popups
    /// Returning false will also close any open prompt
    [[nodiscard]] static bool can_popup();

protected:
    ScreenOPTPrompt(const char *title);

    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    window_header_t header_;

private:
    utils::Timer<uint32_t> close_timer_ { 30000 };
};

} // namespace buddy::openprinttag
