// selftest_fans_axis.c

#include "selftest.h"
#include "dbg.h"
#include "config.h"
#include "hwio_a3ides.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "wizard_ui.h"

extern uint32_t Tacho_FAN0;
extern uint32_t Tacho_FAN1;

void wizard_init_screen_selftest_fans_axis(int16_t id_body, selftest_fans_axis_screen_t *p_screen,
    selftest_fans_axis_data_t *p_data) {
    int16_t id;
    window_destroy_children(id_body);
    window_show(id_body);
    window_invalidate(id_body);

    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_fan_test));
    window_set_text(id, "Fan test");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress_fan));

    y += 12;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 200, 22), &(p_screen->text_extruder_fan));
    window_set_text(id, "Hotend fan");

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(x + 200, y, 22, 22), &(p_screen->icon_extruder_fan));
    window_set_icon_id(id, wizard_get_test_icon_resource(p_data->state_fan0));

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 200, 22), &(p_screen->text_print_fan));
    window_set_text(id, "Print fan");

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(x + 200, y, 22, 22), &(p_screen->icon_print_fan));
    window_set_icon_id(id, wizard_get_test_icon_resource(p_data->state_fan1));

    y += 44;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_checking_axis));
    window_set_text(id, "Checking axes");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress_axis));

    y += 12;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 200, 22), &(p_screen->text_x_axis));
    window_set_text(id, "X-axis");

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(x + 200, y, 22, 22), &(p_screen->icon_x_axis));
    window_set_icon_id(id, wizard_get_test_icon_resource(p_data->state_x));

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 200, 22), &(p_screen->text_y_axis));
    window_set_text(id, "Y-axis");

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(x + 200, y, 22, 22), &(p_screen->icon_y_axis));
    window_set_icon_id(id, wizard_get_test_icon_resource(p_data->state_y));

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 200, 22), &(p_screen->text_z_axis));
    window_set_text(id, "Z-axis");

    id = window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(x + 200, y, 22, 22), &(p_screen->icon_z_axis));
    window_set_icon_id(id, wizard_get_test_icon_resource(p_data->state_z));
}

int wizard_selftest_fan0(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_fan0 == _TEST_START) {
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
        marlin_stop_processing();
        hwio_fan_set_pwm(0, 255);
        Tacho_FAN0 = 0;
    }
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_FAN0_TIME, &(p_data->state_fan0), _WIZ_TIMER_AUTOPASS);
    if (progress == 100) {
        if ((Tacho_FAN0 < _SELFTEST_FAN0_MIN) || (Tacho_FAN0 > _SELFTEST_FAN0_MAX))
            p_data->state_fan0 = _TEST_FAILED;
        hwio_fan_set_pwm(0, 0);
    }
    window_set_value(p_screen->progress_fan.win.id, (float)progress / 2);
    wizard_update_test_icon(p_screen->icon_extruder_fan.win.id, p_data->state_fan0);
    return progress;
}

int wizard_selftest_fan1(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_fan1 == _TEST_START) {
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
        marlin_stop_processing();
        hwio_fan_set_pwm(1, 255);
        Tacho_FAN1 = 0;
    }
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_FAN1_TIME, &(p_data->state_fan1), _WIZ_TIMER_AUTOPASS);
    if (progress == 100) {
        if ((Tacho_FAN1 < _SELFTEST_FAN1_MIN) || (Tacho_FAN1 > _SELFTEST_FAN1_MAX))
            p_data->state_fan1 = _TEST_FAILED;
        hwio_fan_set_pwm(1, 0);
    }
    window_set_value(p_screen->progress_fan.win.id, 50.0F + (float)progress / 2);
    wizard_update_test_icon(p_screen->icon_print_fan.win.id, p_data->state_fan1);
    return progress;
}

const char _axis_char[4] = { 'X', 'Y', 'Z', 'E' };

void wizard_selftest_axis(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data,
    uint8_t *state, int axis, int time, int fr, int min, int max, int dir) {
    static uint8_t phase = 0;
    char achar = _axis_char[axis];
    float pos;
    float dis;
    uint64_t varmsk = MARLIN_VAR_MSK(MARLIN_VAR_POS_X + axis);
    if (*state == _TEST_START)
        phase = 1;
    switch (phase) {
    case 1: // phase 1 - init and start move to maximum
        marlin_start_processing(); // enable processing
        marlin_gcode("M211 S0"); // disable software endstops
        marlin_gcode("M120"); // enable hw endstop detection
        pos = marlin_update_vars(varmsk)->pos[axis]; // update variable pos[]
        marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
        p_data->axis_max[axis] = pos; // save current position
        pos += dir * max; // calc target position
        if (axis == 2)
            marlin_gcode_printf("G28"); // home all axes (Z)
        else
        {
            marlin_gcode_printf("G1 %c%.3f F%d", achar,
                    (double)(pos - dir * (max + 1.92F)), fr / 4);
            marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to maximum (XY)
        }
        marlin_wait_motion(100); // wait for motion start (max 100ms)
        phase++; // next phase
        break;
    case 2: // phase 2 - wait while motion, then read position and start move to minimum
        if (marlin_motion())
            break; // axis is moving - wait
        pos = marlin_update_vars(varmsk)->pos[axis]; // update variable pos[]
        dis = pos - p_data->axis_max[axis]; // calculate traveled distance
        _dbg("dis = %.3f", (double)dis);
        if ((int)(dis + 0.5F) >= max) // check distance >= max
        { //  (round to millimeters)
            *state = _TEST_FAILED; // fail - endstop not triggered
            break;
        }
        marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
        p_data->axis_min[axis] = pos; // save current position
        pos -= dir * max; // calc target position
        marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to maximum
        marlin_wait_motion(100); // wait for motion start (max 100ms)
        phase++; // next phase
        break;
    case 3: // phase 3 - wait while motion, then read position and start move to maximum
        if (marlin_motion())
            break; // axis is moving - wait
        pos = marlin_update_vars(varmsk)->pos[axis]; // update variable pos[]
        dis = dir * (p_data->axis_min[axis] - pos); // calculate traveled distance
        _dbg("dis = %.3f", (double)dis);
        if ((int)(dis + 0.5F) >= max) // check distance >= max
        { //  (round to millimeters)
            _dbg("endstop not reached");
            *state = _TEST_FAILED; // fail - endstop not triggered
            break;
        }
        if ((int)(dis + 0.5F) <= min) // check distance <= min
        { //  (round to millimeters)
            _dbg("distance to short");
            *state = _TEST_FAILED; // fail - axis length invalid
            break;
        }
        marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
        p_data->axis_max[axis] = pos; // save current position
        pos += dir * max;
        if (axis == 2)
            marlin_gcode_printf("G28 Z", achar, (double)pos, fr); // home Z (Z)
        else
            marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to minimum
        marlin_wait_motion(100); // wait for motion start (max 100ms)
        phase++; // next phase
        break;
    case 4: // phase 3 - wait while motion, then read position and finish
        if (marlin_motion())
            break; // axis is moving - wait
        pos = marlin_update_vars(varmsk)->pos[axis]; // update variable pos[]
        dis = dir * (pos - p_data->axis_max[axis]); // calculate traveled distance
        _dbg("dis = %.3f", (double)dis);
        if ((int)(dis + 0.5F) >= max) // check distance >= max
        { //  (round to millimeters)
            _dbg("endstop not reached");
            *state = _TEST_FAILED; // fail - endstop not triggered
            break;
        }
        if ((int)(dis + 0.5F) <= min) // check distance <= min
        { //  (round to millimeters)
            _dbg("distance to short");
            *state = _TEST_FAILED; // fail - axis length invalid
            break;
        }
        _dbg("finished");
        *state = _TEST_PASSED;
        break;
    }
}

int wizard_selftest_x(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_x == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(id_body, p_screen, p_data, &(p_data->state_x), 0,
        _SELFTEST_X_TIME, _SELFTEST_X_FR, _SELFTEST_X_MIN, _SELFTEST_X_MAX, 1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_X_TIME, &(p_data->state_x), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_x_axis.win.id, p_data->state_x);
    return progress;
}

int wizard_selftest_y(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_y == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(id_body, p_screen, p_data, &(p_data->state_y), 1,
        _SELFTEST_Y_TIME, _SELFTEST_Y_FR, _SELFTEST_Y_MIN, _SELFTEST_Y_MAX, -1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_Y_TIME, &(p_data->state_y), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, 33.3F + (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_y_axis.win.id, p_data->state_y);
    return progress;
}

int wizard_selftest_z(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_z == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(id_body, p_screen, p_data, &(p_data->state_z), 2,
        _SELFTEST_Z_TIME, _SELFTEST_Z_FR, _SELFTEST_Z_MIN, _SELFTEST_Z_MAX, -1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_Z_TIME, &(p_data->state_z), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, 66.6F + (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_z_axis.win.id, p_data->state_z);
    return progress;
}
