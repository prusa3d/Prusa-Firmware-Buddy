/**
 * @file gui_media_events.cpp
 * @author Radek Vana
 * @date 2020-11-11
 */

#include "gui_media_events.hpp"
#include "marlin_client.h"
#include "marlin_events.h"
#include "stm32f4xx_hal.h" //HAL_GetTick()

GuiMediaEventsHandler &GuiMediaEventsHandler::Instance() {
    static GuiMediaEventsHandler ret;
    return ret;
}

GuiMediaEventsHandler::GuiMediaEventsHandler()
    : start_time(HAL_GetTick())
    , is_starting(true)
    , one_click_printing(false)
    , state_sent(true)
    , media_state(state_t::unknown) {
}

void GuiMediaEventsHandler::Tick() {
    Instance().tick();
}

void GuiMediaEventsHandler::tick() {
    if (is_starting) {
        if ((HAL_GetTick() - start_time) >= startup_finished_dellay) {
            marlin_event_clr(MARLIN_EVT_MediaRemoved);
            marlin_event_clr(MARLIN_EVT_MediaInserted);
            marlin_event_clr(MARLIN_EVT_MediaError);
            is_starting = false;
            media_state = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS))->media_inserted ? state_t::inserted : state_t::removed;
            // state_sent == true, set by ctor
        }
        return;
    }

    // normal run
    state_t actual_state = state_t::unknown;

    if (marlin_event_clr(MARLIN_EVT_MediaInserted))
        actual_state = state_t::inserted;
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved))
        actual_state = state_t::removed;
    if (marlin_event_clr(MARLIN_EVT_MediaError))
        actual_state = state_t::error;

    if (media_state == state_t::error)
        return; //error must be cleared manually

    switch (actual_state) {
    case state_t::inserted:
        one_click_printing = true;
        state_sent = false;
        break; // update after break
    case state_t::removed:
    case state_t::error:
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
    if (Instance().media_state == state_t::error)
        Instance().clr();
    //update
    Tick();
    //clear again
    if (Instance().media_state == state_t::error)
        Instance().clr();
}

GuiMediaEventsHandler::state_t GuiMediaEventsHandler::ConsumeMediaState() {
    Tick();                               //first update
    state_t ret = Instance().media_state; //remember
    if (ret != state_t::error)
        Instance().state_sent = false; //clear sent
    return ret;
}
