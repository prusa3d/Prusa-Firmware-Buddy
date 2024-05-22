/**
 * @file gui_media_events.hpp
 * @author Radek Vana
 * @brief consumes marlin media events and stores state, handles one click printing flag
 * TODO handle remaining events this should be only place where marlin_event_clr is called
 * @date 2020-11-11
 */

#pragma once
#include <cstdint>
#include "media_state.hpp"

class GuiMediaEventsHandler {
public:
    GuiMediaEventsHandler();
    GuiMediaEventsHandler(const GuiMediaEventsHandler &) = default;

    static GuiMediaEventsHandler &Instance();

private:
    bool one_click_printing;
    bool state_sent;
    MediaState_t media_state;

    void tick();
    void clr() {
        media_state = MediaState_t::unknown;
        state_sent = true;
    }

public:
    static void Tick();
    static bool ConsumeOneClickPrinting();
    static void ClrMediaError(); // clear - update - clear again
    static bool ConsumeSent(MediaState_t &ret); // update - remember - set sent - return
    static MediaState_t Get();
};
