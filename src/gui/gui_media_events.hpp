/**
 * @file gui_media_events.hpp
 * @author Radek Vana
 * @brief tracks media state reported by marlin, handles one click printing flag
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

public:
    static void Tick();
    static bool ConsumeOneClickPrinting();
    static bool ConsumeSent(MediaState_t &ret); // update - remember - set sent - return
    static MediaState_t Get();
};
