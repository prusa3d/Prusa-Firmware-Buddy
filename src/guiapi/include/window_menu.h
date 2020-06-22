#pragma once

#include "window.hpp"

struct window_class_menu_t {
    window_class_t cls;
};

void window_menu_set_item_index(window_t *window, int index);

extern const window_class_menu_t window_class_menu;
