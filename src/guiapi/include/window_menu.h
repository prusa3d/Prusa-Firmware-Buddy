#pragma once

#include "window.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_menu_t {
    window_class_t cls;
} window_class_menu_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void window_menu_set_item_index(window_t *window, int index);

extern const window_class_menu_t window_class_menu;

#ifdef __cplusplus
}
#endif //__cplusplus
