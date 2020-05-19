#pragma once

#include "window_menu.h"
#include "display_helper.h"

typedef struct _window_menu_t window_menu_t;

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
typedef struct
{
    int32_t value;
    const int32_t *range;
} WI_SPIN_t;

//WI_SPIN_FL
typedef struct
{
    float value;
    const char *prt_format;
    const float *range;
} WI_SPIN_FL_t;

//WI_SWITCH | WI_SELECT
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
typedef struct
{
    uint32_t index;
    const char **strings;
} WI_SWITCH_SELECT_t;

typedef struct _window_menu_item_t {
    char label[23];
    uint16_t id_icon;
    /**
	 * Type : WI_LABEL || WI_SPIN || WI_SWITCH || WI_SELECT
	 * visibility bit WI_DISABLED | WI_HIDDEN
	 */
    uint16_t type;

    union {
        WI_SPIN_t wi_spin;
        WI_SPIN_FL_t wi_spin_fl;
        WI_SWITCH_SELECT_t wi_switch_select;
    };
} window_menu_item_t;

typedef void(window_menu_items_t)(window_menu_t *pwindow_menu,
    uint16_t index,
    window_menu_item_t **ppitem,
    void *data);

typedef struct _window_menu_t {
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
    txtroll_t roll;    // text roll variables for too long text
    uint8_t last_index;
} window_menu_t;

#pragma pack(pop)
