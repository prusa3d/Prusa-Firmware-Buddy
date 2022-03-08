/**
 * @file gui_media_events.cpp
 * @author Radek Vana
 * @date 2020-11-11
 */

#include "gui_media_events.hpp"
#include "marlin_client.h"
#include "marlin_events.h"
#include "gui_time.hpp" //gui::GetTick()

GuiMediaEventsHandler &GuiMediaEventsHandler::Instance() {
    static GuiMediaEventsHandler ret;
    return ret;
}

GuiMediaEventsHandler::GuiMediaEventsHandler()
    : start_time(gui::GetTick())
    , is_starting(true)
    , one_click_printing(false)
    , state_sent(true)
    , media_state(MediaState_t::unknown) {
}

void GuiMediaEventsHandler::Tick() {
    Instance().tick();
}

void GuiMediaEventsHandler::tick() {
    if (is_starting) {
        if ((gui::GetTick() - start_time) >= startup_finished_delay) {
            marlin_event_clr(MARLIN_EVT_MediaRemoved);
            marlin_event_clr(MARLIN_EVT_MediaInserted);
            marlin_event_clr(MARLIN_EVT_MediaError);
            is_starting = false;
            media_state = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS))->media_inserted ? MediaState_t::inserted : MediaState_t::removed;
            state_sent = false;
        }
        return;
    }

    // normal run
    MediaState_t actual_state = MediaState_t::unknown;

    if (marlin_event_clr(MARLIN_EVT_MediaInserted))
        actual_state = MediaState_t::inserted;
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved))
        actual_state = MediaState_t::removed;
    if (marlin_event_clr(MARLIN_EVT_MediaError))
        actual_state = MediaState_t::error;

    if (media_state == MediaState_t::error)
        return; //error must be cleared manually

    switch (actual_state) {
    case MediaState_t::inserted:
        one_click_printing = true;
        state_sent = false;
        break; // update after break
    case MediaState_t::removed:
    case MediaState_t::error:
        one_click_printing = false;
        state_sent = false;
        break; // update after break
    default:
        return; //nothing happened, nothing to do .. just return
    }

    media_state = actual_state; // update
}

bool GuiMediaEventsHandler::ConsumeOneClickPrinting() {
    bool ret = Instance().one_click_printing;
    Instance().one_click_printing = false;
    return ret;
}

bool GuiMediaEventsHandler::IsStarting() {
    return Instance().is_starting;
}

void GuiMediaEventsHandler::ClrMediaError() {
    //clear
    if (Instance().media_state == MediaState_t::error)
        Instance().clr();
    //update
    Tick();
    //clear again
    if (Instance().media_state == MediaState_t::error)
        Instance().clr();
}

bool GuiMediaEventsHandler::ConsumeSent(MediaState_t &ret) {
    Tick();                       //first update
    ret = Instance().media_state; //remember
    bool sent = Instance().state_sent;
    if (ret != MediaState_t::error)
        Instance().state_sent = true; //set sent
    return !sent;
}

MediaState_t GuiMediaEventsHandler::Get() {
    return Instance().media_state;
}
