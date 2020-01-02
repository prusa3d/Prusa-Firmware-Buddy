
#ifndef WINDOW_SCROLL_TEXT_H
#define WINDOW_SCROLL_TEXT_H

#include "window.h"

#define TEXT_ROLL_DELAY_MS 500
#define TEXT_ROLL_INITIAL_DELAY_MS 4000

typedef enum {
    ROLL_SETUP = 0,
    ROLL_GO = 1,
    ROLL_STOP = 2,
    ROLL_RESTART = 3,
} TXTROLL_PHASE_t;

typedef struct _window_class_scroll_text_t {
    window_class_t cls;
} window_class_scroll_text_t;

typedef struct _txtroll_t {
    uint8_t phase;
    uint8_t setup;
    uint16_t progress;
    uint16_t count;
    uint8_t px_cd;
    rect_ui16_t rect;
} txtroll_t;

typedef struct _window_scroll_text_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    char *text;
    padding_ui8_t padding;
    uint8_t alignment;
    txtroll_t roll;
} window_scroll_text_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_scroll_text_t window_class_scroll_text;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //WINDOW_SCROLL_TEXT_H
