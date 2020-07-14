#pragma once

#include "gui.hpp"
#include "marlin_events.h"
#include "window_text.hpp"
#include "window_frame.hpp"

typedef enum {
    HEADER_ISTATE_OFF,
    HEADER_ISTATE_ON,
    HEADER_ISTATE_ACTIVE
} header_states_t;

typedef enum {
    HEADER_ICON_USB, // must be first!
    HEADER_ICON_LAN,
    HEADER_ICON_WIFI
    // for next icon, update HEADER_ICON_COUNT define !
} header_icons_t;

#define HEADER_ICON_COUNT HEADER_ICON_WIFI + 1

struct window_class_header_t {
    window_class_t cls;
};

struct window_header_t : public window_frame_t {
    uint16_t id_res;
    header_states_t icons[HEADER_ICON_COUNT]; // usb, lan, wifi
    window_text_t label;
    // char time[10];

    void SetIcon(int16_t id_res);
    void SetText(string_view_utf8 txt);
    header_states_t GetState(header_icons_t icon) const;
    bool EventClr_MediaInserted();
    bool EventClr_MediaRemoved();
    bool EventClr_MediaError();
    void EventClr();
    //private:
    void icon_off(header_icons_t icon);
    void icon_on(header_icons_t icon);
    void icon_activate(header_icons_t icon);

    window_header_t(window_t *parent, window_t *prev);
};

extern int16_t WINDOW_CLS_HEADER;

extern const window_class_header_t window_class_header;
