/**
 * @file gui_media_events.cpp
 * @author Radek Vana
 * @date 2020-11-11
 */

#include "gui_media_events.hpp"
#include "marlin_client.hpp"
#include "marlin_events.h"
#include "gui_time.hpp" //gui::GetTick()
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
    MediaState_t actual_state = MediaState_t::unknown;

    if (marlin_client::event_clr(marlin_server::Event::MediaInserted)) {
        actual_state = MediaState_t::inserted;
    }
    if (marlin_client::event_clr(marlin_server::Event::MediaRemoved)) {
        actual_state = MediaState_t::removed;
    }
    if (marlin_client::event_clr(marlin_server::Event::MediaError)) {
        actual_state = MediaState_t::error;
    }

    if (media_state == MediaState_t::error) {
        return; // error must be cleared manually
    }

    switch (actual_state) {
    case MediaState_t::inserted:
        if (!device_connected_at_startup()) {
            one_click_printing = true;
        }
        state_sent = false;
        break; // update after break
    case MediaState_t::removed:
    case MediaState_t::error:
        one_click_printing = false;
        state_sent = false;
        break; // update after break
    default:
        return; // nothing happened, nothing to do .. just return
    }

    media_state = actual_state; // update
}

void GuiMediaEventsHandler::SetOneClickPrinting() {
    Instance().one_click_printing = true;
}

bool GuiMediaEventsHandler::ConsumeOneClickPrinting() {
    bool ret = Instance().one_click_printing;
    Instance().one_click_printing = false;
    return ret;
}

void GuiMediaEventsHandler::ClrMediaError() {
    // clear
    if (Instance().media_state == MediaState_t::error) {
        Instance().clr();
    }
    // update
    Tick();
    // clear again
    if (Instance().media_state == MediaState_t::error) {
        Instance().clr();
    }
}

bool GuiMediaEventsHandler::ConsumeSent(MediaState_t &ret) {
    Tick(); // first update
    ret = Instance().media_state; // remember
    bool sent = Instance().state_sent;
    if (ret != MediaState_t::error) {
        Instance().state_sent = true; // set sent
    }
    return !sent;
}

MediaState_t GuiMediaEventsHandler::Get() {
    return Instance().media_state;
}
