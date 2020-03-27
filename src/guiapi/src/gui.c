// gui.c

#include "gui.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"

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
    COLOR_BLACK,              //color_back;
    COLOR_WHITE,              //color_text;
    COLOR_SILVER,             //color_disabled;
    0,                        //font;
    0,                        //font_big;
    { 2, 2, 2, 2 },           //padding; padding_ui8(2,2,2,2)
    ALIGN_LEFT_TOP,           //alignment;
    { 0, 32, 240, 320 - 96 }, //msg box size
    6,                        // btn_spacing: 12 pixels spacing between buttons, 6 from margins
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
    display->init();
#ifdef GUI_JOGWHEEL_SUPPORT
    jogwheel_init();
    gui_reset_jogwheel();
#endif //GUI_JOGWHEEL_SUPPORT
    gui_task_handle = osThreadGetId();
}

extern window_t *window_0;
extern window_t *window_1;

void gui_redraw(void) {
    if (gui_flags & GUI_FLG_INVALID) {
        screen_draw();
        if (window_0)
            window_0->cls->draw(window_0);
        if (window_1)
            window_1->cls->draw(window_1);
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

            //=======MENU_TIMEOUT=========
            if (menu_timeout_enabled == 1) {
                if (gui_get_menu_timeout_id() >= 0)
                    gui_timer_reset(gui_get_menu_timeout_id());
                else
                    gui_timer_create_timeout((uint32_t)MENU_TIMEOUT_MS, (int16_t)-1);
            }
        }
        if (!jogwheel_button_down ^ !gui_jogwheel_button_down) {
            if (gui_jogwheel_button_down)
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_BTN_UP, 0);
            else
                screen_dispatch_event(window_capture_ptr, WINDOW_EVENT_BTN_DN, 0);
            gui_jogwheel_button_down = jogwheel_button_down;

            //=======MENU_TIMEOUT=========
            if (menu_timeout_enabled == 1) {
                if (gui_get_menu_timeout_id() >= 0)
                    gui_timer_reset(gui_get_menu_timeout_id());
                else
                    gui_timer_create_timeout((uint32_t)MENU_TIMEOUT_MS, (int16_t)-1);
            }
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
}

int gui_msgbox_ex(const char *title, const char *text, uint16_t flags,
    rect_ui16_t rect, uint16_t id_icon, const char **buttons) {
    window_msgbox_t msgbox;
    window_t *window_1_tmp = window_1; //save current window_1
    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_MSGBOX, 0, rect, &msgbox);
    msgbox.title = title;
    msgbox.text = text;
    msgbox.flags = flags;
    msgbox.id_icon = id_icon;
    memset(msgbox.buttons, 0, 3 * sizeof(char *));
    int btn = flags & MSGBOX_MSK_BTN;
    if ((btn >= MSGBOX_BTN_CUSTOM1) && (btn <= MSGBOX_BTN_CUSTOM3) && buttons) {
        int count = btn - MSGBOX_BTN_CUSTOM1 + 1;
        memcpy(msgbox.buttons, buttons, count * sizeof(char *));
    }
    window_1 = (window_t *)&msgbox;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
    //window_1 is set null after destroying msgbox
    //msgbox destroys itself when user pres any button
    while (window_1) {
        gui_loop();
    }
    window_1 = window_1_tmp; // restore previos window_1
    window_invalidate(0);
    window_set_capture(id_capture);
    return msgbox.res;
}

int gui_msgbox(const char *text, uint16_t flags) {
    return gui_msgbox_ex(0, text, flags, gui_defaults.msg_box_sz, 0, 0);
}

#endif //GUI_WINDOW_SUPPORT

#ifdef GUI_JOGWHEEL_SUPPORT
void gui_reset_jogwheel(void) {
    gui_jogwheel_encoder = jogwheel_encoder;
    gui_jogwheel_button_down = jogwheel_button_down;
}
#endif //GUI_JOGWHEEL_SUPPORT
