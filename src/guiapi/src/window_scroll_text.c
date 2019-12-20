/*
 * 	window_scroll_text.c
 */

#include "window_scroll_text.h"
#include "gui.h"

void window_scroll_text_init(window_scroll_text_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = 0;
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
}
