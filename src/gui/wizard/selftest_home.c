// selftest_home.c

#include "selftest_home.h"
#include "config.h"
#include "marlin_client.h"
#include "wizard_ui.h"

void wizard_init_screen_selftest_home(int16_t id_body, selftest_home_screen_t *p_screen, selftest_home_data_t *p_data) {
    int16_t id;
    window_destroy_children(id_body);
    window_show(id_body);
    window_invalidate(id_body);

    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_calib_home));
    window_set_text(id, "Calibrating home");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(100, 140, 40, 40), &(p_screen->icon_hourglass));
    window_set_icon_id(id, IDR_PNG_wizard_icon_hourglass);
}

int wizard_selftest_home(int16_t id_body, selftest_home_screen_t *p_screen, selftest_home_data_t *p_data) {
    static uint8_t phase = 0;
    if (p_data->state_home == _TEST_START) {
        wizard_init_screen_selftest_home(id_body, p_screen, p_data);
        marlin_gcode("M121");
        phase = 1;
    } else
        switch (phase) {
        case 1:
            marlin_gcode("G28 X");
            marlin_wait_motion(100);
            phase++;
            break;
        case 2:
            if ((marlin_busy() || marlin_motion()))
                break;
            phase++;
            break;
        case 3:
            marlin_gcode("G28 Y");
            marlin_wait_motion(100);
            phase++;
            break;
        case 4:
            if ((marlin_busy() || marlin_motion()))
                break;
            phase++;
            break;
        case 5:
            p_data->state_home = _TEST_PASSED;
            break;
        }
    int progress = wizard_timer(&p_screen->timer, 10000, &(p_data->state_home), _WIZ_TIMER);
    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}
