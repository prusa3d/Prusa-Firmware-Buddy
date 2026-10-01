/**
 * @file gui_media_events.cpp
 * @author Radek Vana
 * @date 2020-11-11
 */

#include "gui_media_events.hpp"
#include "marlin_vars.hpp"
#include "usb_host/usb_host.h"

GuiMediaEventsHandler &GuiMediaEventsHandler::Instance() {
    static GuiMediaEventsHandler ret;
    return ret;
}

GuiMediaEventsHandler::GuiMediaEventsHandler()
    : one_click_printing(false)
    , state_sent(true)
    , media_state(MediaState_t::unknown) {
}

void GuiMediaEventsHandler::Tick() {
    Instance().tick();
}

void GuiMediaEventsHandler::tick() {
    const MediaState_t new_state = marlin_vars().media_inserted ? MediaState_t::inserted : MediaState_t::removed;
    if (new_state == media_state) {
        return;
    }

    if (new_state == MediaState_t::removed) {
        one_click_printing = false;
    } else if (!usb_host::is_media_inserted_since_startup()) {
        one_click_printing = true;
    }
    state_sent = false;
    media_state = new_state;
}

bool GuiMediaEventsHandler::ConsumeOneClickPrinting() {
    bool ret = Instance().one_click_printing;
    Instance().one_click_printing = false;
    return ret;
}

bool GuiMediaEventsHandler::ConsumeSent(MediaState_t &ret) {
    Tick(); // first update
    ret = Instance().media_state; // remember
    bool sent = Instance().state_sent;
    Instance().state_sent = true;
    return !sent;
}

MediaState_t GuiMediaEventsHandler::Get() {
    return Instance().media_state;
}
