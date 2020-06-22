// window_list.h

#ifndef _WINDOW_LIST_H
#define _WINDOW_LIST_H

#include "window.hpp"

typedef struct _window_class_list_t {
    window_class_t cls;
} window_class_list_t;

typedef struct _window_list_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    rect_ui16_t icon_rect;
    int count;
    int index;
    int top_index;
    window_list_item_t *list_item;
} window_list_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_list_t window_class_list;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_LIST_H
