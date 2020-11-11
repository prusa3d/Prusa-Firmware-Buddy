/**
 * @file gui_media_events.hpp
 * @author Radek Vana
 * @brief consumes marlin media events and stores state, handles one click printing flag
 * TODO handle remaining events this should be only place where marlin_event_clr is called
 * @date 2020-11-11
 */

#pragma once
#include <cstdint>

class GuiMediaEventsHandler {
public:
    enum class state_t {
        unknown,
        inserted,
        removed,
        error
    };

    GuiMediaEventsHandler();
    GuiMediaEventsHandler(const GuiMediaEventsHandler &) = default;

    static GuiMediaEventsHandler &Instance();

    static constexpr uint32_t startup_finished_dellay = 1000;
    uint32_t start_time;
    bool is_starting;
    bool one_click_printing;
    bool state_sent;
    state_t media_state;

    void tick();
    void clr() {
        media_state = state_t::unknown;
        state_sent = true;
    }

public:
    static void Tick();
    static bool ConsumeOneClickPrinting();
    static bool IsStarting();
    static void ClrMediaError();        //clear - update - clear again
    static state_t ConsumeMediaState(); //update - remember - clear sent - return
};
