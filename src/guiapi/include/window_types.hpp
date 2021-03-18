//window_types.hpp
#pragma once

#include <inttypes.h>
#include <functional>

using ButtonCallback = std::function<void()>;

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

//to be safe, ctor has this 2 bool parameters, can't switch them
enum class is_hidden_t : bool { no,
    yes };
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
        bool checked : 1;                          // 05 - is checked/selected
        bool timer : 1;                            // 06 - window has timers
        is_closed_on_click_t close_on_click : 1;   // 07 - window id dialog
        bool hidden_behind_dialog : 1;             // 08 - there is an dialog over this window
        is_closed_on_timeout_t timeout_close : 1;  // 09 - menu timeout flag - it's meant to be used in window_frame_t
        is_closed_on_serial_t serial_close : 1;    // 0A - serial printing screen open close
        bool shadow : 1;                           // 0B - darker colors
        bool enforce_capture_when_not_visible : 1; // 0C - normally invisible / hidden_behind_dialog windows does not get capture
        bool custom2 : 1;                          // 0D - this flag can be defined in parent
        bool custom1 : 1;                          // 0E - this flag can be defined in parent
        bool custom0 : 1;                          // 0F - this flag can be defined in parent

        // here would be 2 unused Bytes (structure data alignment),
        // make them accessible to be used in child to save RAM
        union {
            uint16_t mem_space_u16;
            int16_t mem_space_s16;
            std::array<uint8_t, 2> mem_array_u08;
            std::array<int8_t, 2> mem_array_s08;
        };
    };

    constexpr WindowFlags(uint32_t dt = 0)
        : data(dt) {}
};

// current state of button, event is stored into buffer on button change
enum class BtnState_t : uint8_t {
    Released,
    Pressed,
    Held
};

static_assert(sizeof(WindowFlags) == sizeof(WindowFlags::data), "WindowFlags structure invalid");
