// xyzcalib.c

#include "xyzcalib.h"
#include "gui.h"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "screen_wizard.h"
#include "wizard_ui.h"

void wizard_init_screen_xyzcalib(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    int16_t id;
    window_destroy_children(id_body);
    window_show(id_body);
    window_invalidate(id_body);

    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_state));
    window_set_text(id, "Auto home");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));

    y += 12;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 66), &(p_screen->text_search));
    window_set_text(id, "");

    y += 66;

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16((240 - 100) / 2, y, 100, 100), &(p_screen->icon));
}

int xyzcalib_home(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_home == _TEST_START) {
        wizard_init_screen_xyzcalib(id_body, p_screen, p_data);
        window_set_icon_id(p_screen->icon.win.id, IDR_PNG_wizard_icon_autohome);
        marlin_gcode("G28");
        marlin_event_clr(MARLIN_EVT_CommandEnd);
    } else if (marlin_event_clr(MARLIN_EVT_CommandEnd))
        p_data->state_home = _TEST_PASSED;
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_home), _WIZ_TIMER);
    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

int xyzcalib_z(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_z == _TEST_START) {
        window_set_text(p_screen->text_state.win.id, "Calibrating Z");
        window_set_icon_id(p_screen->icon.win.id, IDR_PNG_wizard_icon_hourglass);
    }
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_z), _WIZ_TIMER_AUTOPASS);
    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

int xyzcalib_xy_search(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_xy_search == _TEST_START) {
        window_set_text(p_screen->text_search.win.id,
            "Searching bed\n"
            "calibration points");
        window_set_icon_id(p_screen->icon.win.id, IDR_PNG_wizard_icon_search);
    }
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_xy_search), _WIZ_TIMER_AUTOPASS);
    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

int xyzcalib_xy_measure(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_xy_measure == _TEST_START) {
        window_set_text(p_screen->text_search.win.id,
            "Measuring reference\n"
            "height of calib.\n"
            "points");
        window_set_icon_id(p_screen->icon.win.id, IDR_PNG_wizard_icon_measure);
        marlin_gcode("G29");
        marlin_event_clr(MARLIN_EVT_CommandEnd);
    } else if (marlin_event_clr(MARLIN_EVT_CommandEnd))
        p_data->state_xy_measure = _TEST_PASSED;
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_xy_measure), _WIZ_TIMER);
    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

int xyzcalib_is_ok(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    int ok = 1;
    ok &= (p_data->state_home == _TEST_PASSED);
    ok &= (p_data->state_z == _TEST_PASSED);
    ok &= (p_data->state_xy_search == _TEST_PASSED);
    ok &= (p_data->state_xy_measure == _TEST_PASSED);
    return ok;
}
