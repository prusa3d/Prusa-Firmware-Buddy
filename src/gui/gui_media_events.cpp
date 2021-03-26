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
    , media_state(media_state_t::unknown) {
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
            media_state = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS))->media_inserted ? media_state_t::inserted : media_state_t::removed;
            state_sent = false;
        }
        return;
    }

    // normal run
    media_state_t actual_state = media_state_t::unknown;

    if (marlin_event_clr(MARLIN_EVT_MediaInserted))
        actual_state = media_state_t::inserted;
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved))
        actual_state = media_state_t::removed;
    if (marlin_event_clr(MARLIN_EVT_MediaError))
        actual_state = media_state_t::error;

    if (media_state == media_state_t::error)
        return; //error must be cleared manually

    switch (actual_state) {
    case media_state_t::inserted:
        one_click_printing = true;
        state_sent = false;
        break; // update after break
    case media_state_t::removed:
    case media_state_t::error:
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
    if (Instance().media_state == media_state_t::error)
        Instance().clr();
    //update
    Tick();
    //clear again
    if (Instance().media_state == media_state_t::error)
        Instance().clr();
}

bool GuiMediaEventsHandler::ConsumeSent(media_state_t &ret) {
    Tick();                       //first update
    ret = Instance().media_state; //remember
    bool sent = Instance().state_sent;
    if (ret != media_state_t::error)
        Instance().state_sent = true; //set sent
    return !sent;
}

media_state_t GuiMediaEventsHandler::Get() {
    return Instance().media_state;
}
