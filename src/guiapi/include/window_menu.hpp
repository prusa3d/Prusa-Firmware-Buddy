#pragma once

#include "window_menu.h"
#include <stdint.h>
#include <array>

struct window_menu_t;

#pragma pack(push)
#pragma pack(1)

typedef enum {
    WI_LABEL = 0, // types - exclusive
    WI_SPIN,
    WI_SWITCH,
    WI_SELECT,
    WI_SPIN_FL,

    WI_DISABLED = 1 << 8, // flags - non exclusive
    WI_HIDDEN = 1 << 9,
} window_item_type_t;

//WI_SPIN
//where all values are divided by 1000
struct WI_SPIN_t {
    int32_t value;
    const int32_t *range;
};

//WI_SPIN_FL
struct WI_SPIN_FL_t {
    float value;
    const char *prt_format;
    const float *range;
};

//WI_SWITCH | WI_SELECT
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
struct WI_SWITCH_SELECT_t {
    uint32_t index;
    const char **strings;
};

class WindowMenuItem {
    WindowMenuItem(uint16_t type, const char *text, uint16_t id_icon);

public:
    WindowMenuItem(const char *label, uint16_t id_icon = 0, uint16_t flags = WI_LABEL); // does not initialize union
    WindowMenuItem(WI_SPIN_t wi_spin, const char *text, uint16_t id_icon = 0);
    WindowMenuItem(WI_SPIN_FL_t wi_spin_fl, const char *text, uint16_t id_icon = 0);
    WindowMenuItem(WI_SWITCH_SELECT_t wi_switch_select, const char *text, uint16_t id_icon, bool switch_not_select);
    /**
	 * Type : WI_LABEL || WI_SPIN || WI_SWITCH || WI_SELECT
	 * visibility bit WI_DISABLED | WI_HIDDEN
	 */
    uint16_t type;
    uint16_t id_icon;
    std::array<char, 23> label;

    union Data {
        Data() {}
        Data(const WI_SPIN_t &r) { wi_spin = r; }
        Data(const WI_SPIN_FL_t &r) { wi_spin_fl = r; }
        Data(const WI_SWITCH_SELECT_t &r) { wi_switch_select = r; }

        WI_SPIN_t wi_spin;
        WI_SPIN_FL_t wi_spin_fl;
        WI_SWITCH_SELECT_t wi_switch_select;
    };
    Data data;
};

typedef void(window_menu_items_t)(window_menu_t *pwindow_menu,
    uint16_t index, WindowMenuItem **ppitem, void *data);

struct window_menu_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    color_t color_disabled;
    font_t *font;
    padding_ui8_t padding;
    rect_ui16_t icon_rect;
    uint8_t alignment;
    int count;
    int index;
    int top_index;
    uint8_t mode;
    window_menu_items_t *menu_items;
    void *data;
    uint8_t src_event; // source event
    void *src_param;   // source event data
};

#pragma pack(pop)
