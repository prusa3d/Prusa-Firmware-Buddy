// gui.c
#include "display.h"
#include "gui.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "sound_C_wrapper.h"

#define GUI_FLG_INVALID 0x0001

uint16_t gui_flags = 0;

#ifdef GUI_JOGWHEEL_SUPPORT
int gui_jogwheel_encoder = 0;
int gui_jogwheel_button_down = 0;
#endif //GUI_JOGWHEEL_SUPPORT

#ifdef GUI_USE_RTOS
osThreadId gui_task_handle = 0;
#endif //GUI_USE_RTOS

gui_defaults_t gui_defaults = {
    COLOR_BLACK,                // color_back;
    COLOR_WHITE,                // color_text;
    COLOR_SILVER,               // color_disabled;
    0,                          // font;
    0,                          // font_big;
    { 2, 2, 2, 2 },             // padding; padding_ui8(2,2,2,2)
    ALIGN_LEFT_TOP,             // alignment;
    { 0, 0, 240, 32 - 0 },      // default header location & size
    { 0, 32, 240, 267 - 32 },   // default screen body location & size
    { 0, 32, 240, 320 - 32 },   // screen body without footer - location & size
    { 0, 267, 240, 320 - 267 }, // default footer location & size
    30,                         // default button height
    6,                          // btn_spacing: 12 pixels spacing between buttons, 6 from margins
    10,                         // default frame width
};

gui_loop_cb_t *gui_loop_cb = 0;
uint32_t gui_loop_tick = 0;

void *gui_malloc(unsigned int size) {
    return malloc(size);
}

void gui_free(void *ptrx) {
    free(ptrx);
}

void gui_init(void) {
    display::Init();
#ifdef GUI_JOGWHEEL_SUPPORT
    jogwheel_init();
    gui_reset_jogwheel();
#endif //GUI_JOGWHEEL_SUPPORT
    gui_task_handle = osThreadGetId();
}

extern window_t *window_0;

void gui_redraw(void) {
    if (gui_flags & GUI_FLG_INVALID) {
        screen_draw();
        if (window_0)
            window_0->cls->draw(window_0);
        if (window_popup_ptr)
            window_popup_ptr->cls->draw(window_popup_ptr);
        //window_draw(0);
        gui_flags &= ~GUI_FLG_INVALID;
    }
}

void gui_invalidate(void) {
    gui_flags |= GUI_FLG_INVALID;
#ifdef GUI_USE_RTOS
    osSignalSet(gui_task_handle, GUI_SIG_REDRAW);
#endif //GUI_USE_RTOS
}

#define GUI_DELAY_MIN  1
#define GUI_DELAY_MAX  10
#define GUI_DELAY_LOOP 100

#ifdef GUI_WINDOW_SUPPORT

static uint8_t guiloop_nesting = 0;
uint8_t gui_get_nesting(void) { return guiloop_nesting; }

void gui_loop(void) {
    ++guiloop_nesting;
    uint32_t delay;
    uint32_t tick;
    #ifdef GUI_JOGWHEEL_SUPPORT

    // Encoder sound moved from guimain to gui loop to control encoder sounds in
    // every gui screens. Previous method wasn't everywhere.
    if ((jogwheel_changed & 1) && jogwheel_button_down) { //button changed and pressed
        Sound_Play(eSOUND_TYPE_ButtonEcho);
    } else if (jogwheel_changed & 2) { // encoder changed
        Sound_Play(eSOUND_TYPE_EncoderMove);
    }

    if (jogwheel_changed) {
        if (gui_loop_cb)
            gui_loop_cb();
        jogwheel_changed = 0;
        if ((jogwheel_encoder != gui_jogwheel_encoder)) {
            int dif = jogwheel_encoder - gui_jogwheel_encoder;
            if (dif > 0)
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_ENC_UP, (void *)dif);
            else if (dif < 0)
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_ENC_DN, (void *)-dif);
            gui_jogwheel_encoder = jogwheel_encoder;
            gui_reset_menu_timer();
        }
        if (!jogwheel_button_down ^ !gui_jogwheel_button_down) {
            if (gui_jogwheel_button_down)
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_BTN_UP, 0);
            else
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_BTN_DN, 0);
            gui_jogwheel_button_down = jogwheel_button_down;
            gui_reset_menu_timer();
        }
    }

    #endif //GUI_JOGWHEEL_SUPPORT
    delay = gui_timers_cycle();
    if (delay < GUI_DELAY_MIN)
        delay = GUI_DELAY_MIN;
    if (delay > GUI_DELAY_MAX)
        delay = GUI_DELAY_MAX;
    #ifdef GUI_USE_RTOS
    osEvent evt = osSignalWait(GUI_SIG_REDRAW, delay);
    if ((evt.status == osEventSignal) && (evt.value.signals & GUI_SIG_REDRAW))
    #endif //GUI_USE_RTOS

        gui_redraw();
    tick = HAL_GetTick();
    if ((tick - gui_loop_tick) >= GUI_DELAY_LOOP) {
        if (gui_loop_cb)
            gui_loop_cb();
        gui_loop_tick = tick;
        screen_dispatch_event(0, WINDOW_EVENT_LOOP, 0);
    }
    --guiloop_nesting;

    // -- reset menu timer when we're in dialog
    if (guiloop_nesting > 0) {
        gui_reset_menu_timer();
    }
}

void gui_reset_menu_timer() {
    if (menu_timeout_enabled) {
        if (gui_get_menu_timeout_id() >= 0) {
            gui_timer_reset(gui_get_menu_timeout_id());
        } else {
            gui_timer_create_timeout((uint32_t)MENU_TIMEOUT_MS, (int16_t)-1);
        }
    }
}

/// Creates message box with provided informations
/// \returns message box id
int gui_msgbox_ex(const char *title, const char *text, uint16_t flags,
    rect_ui16_t rect, uint16_t id_icon, const char **buttons) {

    window_msgbox_t msgbox;
    window_t *window_popup_tmp = window_popup_ptr; //save current window_popup_ptr
    const int16_t id_capture = window_capture();
    const int16_t id = window_create_ptr(WINDOW_CLS_MSGBOX, 0, rect, &msgbox);
    msgbox.title = title;
    msgbox.text = text;
    msgbox.flags = flags;
    msgbox.id_icon = id_icon;
    memset(msgbox.buttons, 0, 3 * sizeof(char *));
    const int btn = flags & MSGBOX_MSK_BTN;
    if ((btn >= MSGBOX_BTN_CUSTOM1) && (btn <= MSGBOX_BTN_CUSTOM3) && buttons) {
        const int count = btn - MSGBOX_BTN_CUSTOM1 + 1;
        memcpy(msgbox.buttons, buttons, count * sizeof(char *));
    }
    window_popup_ptr = (window_t *)&msgbox;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
    // window_popup_ptr is set to null after destroying msgbox
    // msgbox destroys itself when the user presses any button
    while (window_popup_ptr) {
        gui_loop();
    }
    window_popup_ptr = window_popup_tmp; // restore previous window_popup_ptr
    window_invalidate(0);
    window_set_capture(id_capture);
    return msgbox.res;
}

int gui_msgbox(const char *text, uint16_t flags) {
    return gui_msgbox_ex(0, text, flags, gui_defaults.scr_body_sz, 0, 0);
}

// specific function for PROMPT message box with soundStandardPrompt sound
// This is because of infinitely repeating sound signal that has to be stopped
// additionally
int gui_msgbox_prompt(const char *text, uint16_t flags) {
    Sound_Play(eSOUND_TYPE_StandardPrompt);
    return gui_msgbox_ex(0, text, flags, gui_defaults.scr_body_sz, 0, 0);
}

#endif //GUI_WINDOW_SUPPORT

#ifdef GUI_JOGWHEEL_SUPPORT
void gui_reset_jogwheel(void) {
    gui_jogwheel_encoder = jogwheel_encoder;
    gui_jogwheel_button_down = jogwheel_button_down;
}
#endif //GUI_JOGWHEEL_SUPPORT
