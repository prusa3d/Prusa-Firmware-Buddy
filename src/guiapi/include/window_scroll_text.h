
#ifndef WINDOW_SCROLL_TEXT_H
#define WINDOW_SCROLL_TEXT_H

#include "window.h"

#define TEXT_ROLL_DELAY_MS 100
#define TEXT_ROLL_INITIAL_DELAY_MS 1000

typedef struct _window_class_scroll_text_t {
    window_class_t cls;
} window_class_scroll_text_t;

typedef struct _window_scroll_text_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    char *text;
    padding_ui8_t padding;
    uint8_t alignment;
    uint8_t roll_flag;
    uint16_t roll_progress;
    uint16_t roll_count;
    uint8_t width_countdown;
    rect_ui16_t text_rect;
} window_scroll_text_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_scroll_text_t window_class_scroll_text;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //WINDOW_SCROLL_TEXT_H
