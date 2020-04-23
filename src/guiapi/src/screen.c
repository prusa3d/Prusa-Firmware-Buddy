// screen.c

#include "screen.h"
#include "gui.h"
#include "bsod.h"

#define SCREEN_MAX_SCREENS 48

// potential dependency of SCREEN_MAX_HISTORY and SCREEN_MAX_SCREENS is unclear
// but yet these two defines were kept in sync (same values)
#define SCREEN_MAX_HISTORY 48

screen_t *screen_0 = 0; //current screen

screen_t *screens[SCREEN_MAX_SCREENS];
uint16_t screen_count = 0;

int16_t screen_stack[SCREEN_MAX_HISTORY];
uint16_t screen_stack_count = 0;

int16_t screen_id(void) {
    if (screen_0)
        return screen_0->id;
    return -1;
}

int16_t screen_register(screen_t *pscreen) {
    int16_t id = -1;
    if ((pscreen != 0) && (screen_count < SCREEN_MAX_SCREENS)) {
        id = 0;
        if (screen_count == 0) //reset all pointers when starting
            memset(screens, 0, SCREEN_MAX_SCREENS * sizeof(screen_t *));
        else //find free id
            while ((id < SCREEN_MAX_SCREENS) && (screens[id]))
                id++;
        if (id < SCREEN_MAX_SCREENS) { //id is valid
            screens[id] = pscreen;     //set screen pointer
            pscreen->id = id;
            screen_count++; //increment count
        } else
            id = -1;
    } else {
        general_error("GUI", "Maximum number of screens reached.");
    }
    return id;
}

screen_t *screen_unregister(int16_t screen_id) {
    //TODO
    return 0;
}

void screen_stack_push(int16_t screen_id) {
    screen_t *pscreen;
    if (screen_stack_count < SCREEN_MAX_HISTORY)
        if ((screen_id >= 0) && (screen_id < SCREEN_MAX_SCREENS) && ((pscreen = screens[screen_id]) != 0))
            screen_stack[screen_stack_count++] = screen_id;
}

int16_t screen_stack_pop(void) {
    int16_t screen_id = -1;
    if (screen_stack_count > 0)
        screen_id = screen_stack[--screen_stack_count];
    return screen_id;
}

void screen_open(int16_t screen_id) {
    if (screen_0) {
        screen_stack_push(screen_0->id);
        screen_0->done(screen_0);
        if (screen_0->pdata && screen_0->data_size) {
            gui_free(screen_0->pdata);
            screen_0->pdata = 0;
        }
        screen_0 = 0;
    }
    if ((screen_id >= 0) && (screen_id < SCREEN_MAX_SCREENS) && ((screen_0 = screens[screen_id]) != 0)) {
        if (screen_0->data_size)
            screen_0->pdata = gui_malloc(screen_0->data_size);
        screen_0->init(screen_0);
        window_set_capture(0);
    }
}

void screen_close(void) {
    if (screen_0) {
        screen_0->done(screen_0);
        if (screen_0->pdata && screen_0->data_size) {
            gui_free(screen_0->pdata);
            screen_0->pdata = 0;
        }
        screen_0 = 0;
    }
    int16_t screen_id = screen_stack_pop();
    if ((screen_id >= 0) && (screen_id < SCREEN_MAX_SCREENS) && ((screen_0 = screens[screen_id]) != 0)) {
        if (screen_0->data_size)
            screen_0->pdata = gui_malloc(screen_0->data_size);
        screen_0->init(screen_0);
        window_set_capture(0);
    }
}

void screen_draw(void) {
    if (screen_0 && screen_0->draw)
        screen_0->draw(screen_0);
}

void screen_dispatch_event(window_t *window, uint8_t event, void *param) {
    int ret = 0;
    if (screen_0 && screen_0->event) {
        ret = screen_0->event(screen_0, window, event, param);
        if (screen_0 == 0)
            ret = 1;
    }
    if ((ret == 0) && window && window->event)
        window_dispatch_event(window, event, param);
}

screen_t *screen_get_curr(void) {
    return screen_0;
}
