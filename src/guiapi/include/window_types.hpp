// window_types.hpp
#pragma once

#include <inttypes.h>
#include <inplace_function.hpp>

class window_t;

using ButtonCallback = stdext::inplace_function<void(window_t &button)>;

struct window_list_t;
typedef void(window_list_item_t)(window_list_t *pwindow_list,
    uint16_t index, const char **pptext, uint16_t *pid_icon);

// to be safe, ctor has this 2 bool parameters, can't switch them
enum class is_hidden_t : uint8_t { no,
    yes,
    dev };
enum class is_enabled_t : bool { no,
    yes };
enum class expands_t : bool { no,
    yes };
enum class is_closed_on_click_t : bool { no,
    yes };
enum class is_closed_on_timeout_t : bool { no,
    yes };
enum class is_closed_on_printing_t : bool { no,
    yes };
enum class positioning : bool { absolute,
    relative };
enum class dense_t : bool { no,
    yes };
enum class show_disabled_extension_t : bool { no,
    yes };

// type of window
// carefull if any states are added - flags and getter must be modified
enum class win_type_t : uint8_t {
    normal, // single normal window in screen can have capture, registered in ctor, registration must succedd
    dialog, // can have capture, child of IDialog - modal window - multiple supported
    popup, // can't have capture, similar to dialog, but does not claim capture, cannot overlap or be overlapped
           //   by dialog (not registered / auto destroyed).
           // destroyed when any window tries to overlap it
};

// todo add can capture flag (needed in frame event and SetCapture)
union WindowFlags {
    uint32_t data;
    struct {
        win_type_t type : 2 = win_type_t::normal; // 00 .. 01 - type of window
        bool visible : 1 = true; // 02 - is visible
        bool enabled : 1 = true; // 03 - is enabled (can be focused)
        bool invalid : 1 = true; // 04 - content is invalid (draw)
        bool invalid_background : 1 = true; // 05 - some windows might suport not drawing background
        bool color_scheme_background : 1 = false; // 06 - select between color and pointer to color_scheme, for background color
        bool color_scheme_foreground : 1 = false; // 07 - select between color and pointer to color_scheme, for foreground color
        is_closed_on_click_t close_on_click : 1 = is_closed_on_click_t::no; // 09 - window id dialog
        bool hidden_behind_dialog : 1 = false; // 0A - there is an dialog over this window
        is_closed_on_timeout_t timeout_close : 1 = is_closed_on_timeout_t::no; // 0B - menu timeout flag - it's meant to be used in window_frame_t
        is_closed_on_printing_t print_close : 1 = is_closed_on_printing_t::no; // 0C - should be closed, if print is started
        bool shadow : 1 = false; // 0D - executable (causes darker colors)
        bool enforce_capture_when_not_visible : 1 = false; // 0E - normally invisible / hidden_behind_dialog windows does not get capture
        bool has_relative_subwins : 1 = false; // 0F - X Y coords of all children are relative to this, screen cannot have this flag because 1st level windows can be dialogs and they must not have relative coords
        bool multiline : 1 = false; // 10 - multiline text affect window_text_t anf its children
        bool has_long_hold_screen_action : 1 = false; // 13 - screen will use default callback for long press
        bool has_icon : 1 = false; // 14 - optional or alternative icon for window
        bool has_round_corners : 1 = false; // 15 - window has round corners with default corner radius
        union { // 18 .. 1F - 8bit variable used in child classes
            uint8_t align_data; // used in window_aligned_t
            struct {
                uint8_t button_count : 4; // used in RadioButton
                uint8_t button_index : 4; // used in RadioButton
            };
        } class_specific = { .align_data = 0 };
    };
};

// current state of button, event is stored into buffer on button change
enum class BtnState_t : uint8_t {
    Released,
    Pressed,
    Held,
    HeldAndLeft,
    HeldAndRight,
    HeldAndReleased
};

static_assert(sizeof(WindowFlags) == sizeof(WindowFlags::data), "WindowFlags structure invalid");
