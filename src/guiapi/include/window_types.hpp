//window_types.hpp
#pragma once

#include <inttypes.h>
#include <functional>

using ButtonCallback = std::function<void()>;

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

//to be safe, ctor has this 2 bool parameters, can't switch them
enum class is_hidden_t : uint8_t { no,
    yes,
    dev };
enum class is_enabled_t : bool { no,
    yes };
enum class is_focused_t : bool { no,
    yes };
enum class is_selected_t : bool { no,
    yes };
enum class expands_t : bool { no,
    yes };
enum class is_closed_on_click_t : bool { no,
    yes };
enum class is_closed_on_timeout_t : bool { no,
    yes };
enum class is_closed_on_serial_t : bool { no,
    yes };
enum class positioning : bool { absolute,
    relative };

//type of window
//carefull if any states are added - flags and getter must be modified
enum class win_type_t : uint8_t {
    normal,       // single normal window in screen can have capture, registered in ctor, registration must succedd
    dialog,       // can have capture, child of IDialog - modal window - multiple supported
    popup,        // can't have capture, similar to dialog, but does not claim capture, cannot overlap or be overlapped
                  //   by dialog (not registered / auto destroyed).
                  // destroyed when any window tries to overlap it
    strong_dialog // can have capture, stays on absolute top (normal dialog can open under it, but
                  //   will not get capture), only user can close it
                  // last open strong dialog is on top
};

//todo add can capture flag (needed in frame event and SetCapture)
union WindowFlags {
    uint32_t data;
    struct {
        uint8_t type : 2;                          // 00 .. 01 - type of window
        bool visible : 1;                          // 02 - is visible
        bool enabled : 1;                          // 03 - is enabled (can be focused)
        bool invalid : 1;                          // 04 - content is invalid (draw)
        bool color_scheme_background : 1;          // 05 - select between color and pointer to color_scheme, for background color
        bool color_scheme_foreground : 1;          // 06 - select between color and pointer to color_scheme, for foreground color
        bool timer : 1;                            // 07 - window has timers
        is_closed_on_click_t close_on_click : 1;   // 08 - window id dialog
        bool hidden_behind_dialog : 1;             // 09 - there is an dialog over this window
        is_closed_on_timeout_t timeout_close : 1;  // 0A - menu timeout flag - it's meant to be used in window_frame_t
        is_closed_on_serial_t serial_close : 1;    // 0B - serial printing screen open close
        bool shadow : 1;                           // 0C - darker colors
        bool enforce_capture_when_not_visible : 1; // 0D - normally invisible / hidden_behind_dialog windows does not get capture
        bool has_relative_subwins : 1;             // 0E - X Y coords of all children are relative to this, screen cannot have this flag because 1st level windows can be dialogs and they must not have relative coords
        bool multiline : 1;                        // 0F - multiline text affect window_text_t anf its children
        bool blink : 1;                            // 10 - for 2 state blinking
        bool custom6 : 1;                          // 11 - this flag can be defined in child class
        bool custom5 : 1;                          // 12 - this flag can be defined in child class
        bool custom4 : 1;                          // 13 - this flag can be defined in child class
        bool custom3 : 1;                          // 14 - this flag can be defined in child class
        bool custom2 : 1;                          // 15 - this flag can be defined in child class
        bool custom1 : 1;                          // 16 - this flag can be defined in child class
        bool custom0 : 1;                          // 17 - this flag can be defined in child class
        union {                                    // 18 .. 1F - 8bit variable used in child classes
            uint8_t align_data;                    // used in window_aligned_t
            struct {
                uint8_t button_count : 4; // used in RadioButton
                uint8_t button_index : 4; // used in RadioButton
            };
        };
    };

    constexpr WindowFlags(uint32_t dt = 0)
        : data(dt) {}
};

// current state of button, event is stored into buffer on button change
enum class BtnState_t : uint8_t {
    Released,
    Pressed,
    Held,
    HeldAndLeft,
    HeldAndRigth,
    HeldAndReleased
};

static_assert(sizeof(WindowFlags) == sizeof(WindowFlags::data), "WindowFlags structure invalid");
