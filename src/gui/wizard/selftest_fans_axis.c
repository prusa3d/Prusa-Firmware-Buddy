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


static float _get_pos(int axis) {
	return marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_X + axis))->pos[axis];
}




//returns phase modofication
typedef int (*selftest_phase)(selftest_fans_axis_data_t *p_data,
	    uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos);

typedef struct
{
    const size_t sz;
    const selftest_phase *p_phases;
} _cl_st_ax;


static int ph_init(selftest_fans_axis_data_t *p_data,
	    uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos){
    marlin_start_processing(); // enable processing
    marlin_gcode("M211 S0"); // disable software endstops
    marlin_gcode("M120"); // enable hw endstop detection
    return 1;//next phase
}

static int ph_move_to_max(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
	marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
	p_data->axis_max[axis] = pos; // save current position
	pos += dir * max; // calc target position
	marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to maximum (XY)
	marlin_wait_motion(100); // wait for motion start (max 100ms)
	return 1;//next phase
}

static int ph_wait_motion(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
	if (marlin_motion()) return 0;//wait
	else return 1;//next phase
}
/*
int ph_measure_max(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir) {
    float dis = pos - p_data->axis_max[axis]; // calculate traveled distance
    _dbg("dis = %.3f", (double)dis);
    if ((int)(dis + 0.5F) >= max) // check distance >= max
    { //  (round to millimeters)
       *state = _TEST_FAILED; // fail - endstop not triggered
        return 0;
    }
    return 1;//next phase
}*/

static int ph_move_to_min(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
    marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
    p_data->axis_min[axis] = pos; // save current position
    pos -= dir * max; // calc target position
    marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to maximum
    marlin_wait_motion(100); // wait for motion start (max 100ms)
    return 1;//next phase
}

static int ph_measure_min(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
    float dis = dir * (p_data->axis_min[axis] - pos); // calculate traveled distance
    _dbg("dis = %.3f", (double)dis);
    if ((int)(dis + 0.5F) >= max) // check distance >= max
    { //  (round to millimeters)
        _dbg("endstop not reached");
        *state = _TEST_FAILED; // fail - endstop not triggered
        return 0;
    }
    if ((int)(dis + 0.5F) <= min) // check distance <= min
    { //  (round to millimeters)
        _dbg("distance to short");
        *state = _TEST_FAILED; // fail - axis length invalid
        return 0;
    }
    return 1;
}

static int ph_measure_max(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
    float dis = dir * (pos - p_data->axis_max[axis]); // calculate traveled distance
    _dbg("dis = %.3f", (double)dis);
    if ((int)(dis + 0.5F) >= max) // check distance >= max
    { //  (round to millimeters)
        _dbg("endstop not reached");
        *state = _TEST_FAILED; // fail - endstop not triggered
        return 0;
    }
    if ((int)(dis + 0.5F) <= min) // check distance <= min
    { //  (round to millimeters)
        _dbg("distance to short");
        *state = _TEST_FAILED; // fail - axis length invalid
        return 0;
    }
    return 1;
}

static int ph_finish(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir, char achar, float pos) {
    _dbg("finished");
    *state = _TEST_PASSED;
    return 0;
}

static const selftest_phase phasesXY[] = {
		ph_init,
		ph_move_to_max,
		ph_wait_motion,
		ph_move_to_min,
		ph_wait_motion,
		ph_measure_min,
		ph_move_to_max,
		ph_wait_motion,
		ph_measure_max,
		ph_finish
};

static const _cl_st_ax axisXY = {
		sizeof(phasesXY)/sizeof(phasesXY[0]),
		phasesXY
};


static const selftest_phase phasesZ[] = {
		ph_init,
		ph_move_to_max,
		ph_wait_motion,
		ph_move_to_min,
		ph_wait_motion,
		ph_measure_min,
		ph_move_to_max,
		ph_wait_motion,
		ph_measure_max,
		ph_finish
};

static const _cl_st_ax axisZ = {
		sizeof(phasesZ)/sizeof(phasesZ[0]),
		phasesZ
};
void wizard_selftest_axis(const _cl_st_ax* _ths, selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir) {
    static uint8_t phase = 0;
    char achar = _axis_char[axis];
    float pos = _get_pos(axis);
    if (*state == _TEST_START) {
    	phase = 0;
    }

    if ( ((size_t)phase) >= _ths->sz) {
    	*state = _TEST_FAILED;
    }
    else {
    	phase += _ths->p_phases[phase](p_data, state, axis, fr, min, max, dir, achar, pos);
    }
}

/*
typedef enum
{
	PH_INIT,
	PH_Z_SPECIFIC,
	PH_Z_SPECIFIC_WAIT_COMPLETATION,
	PH_NOT_Z_SPECIFIC
}phases_t;

void wizard_selftest_axis(selftest_fans_axis_data_t *p_data,
		uint8_t *state, int axis, int fr, int min, int max, int dir) {
    static uint8_t phase = 0;
    char achar = _axis_char[axis];
    float pos = _get_pos(axis);;
    float dis;
    if (*state == _TEST_START) {
    	*state = _TEST_RUN;
    	phase = PH_INIT;
    }
    switch (phase) {
    case PH_INIT: // phase 1 - init and start move to maximum
        marlin_start_processing(); // enable processing
        marlin_gcode("M211 S0"); // disable software endstops
        marlin_gcode("M120"); // enable hw endstop detection
        if (axis == 2)// axes (Z)
        	phase = PH_Z_SPECIFIC; // next phase
        else
        	phase = PH_NOT_Z_SPECIFIC;//skip Z specific code
        break;
    case PH_Z_SPECIFIC://Z specific
    	marlin_gcode_printf("G28"); // home all axes (Z)
    	marlin_wait_motion(100);
    	phase++; // next phase
    	break;
    case PH_Z_SPECIFIC_WAIT_COMPLETATION:
    	if (marlin_motion()) break; // axis is moving - wait
    	phase++; // next phase
    	break;
    case PH_NOT_Z_SPECIFIC:
        marlin_gcode_printf("G92 %c%.3f", achar, (double)pos); // set position to current
        p_data->axis_max[axis] = pos; // save current position
        pos += dir * max; // calc target position
        marlin_gcode_printf("G1 %c%.3f F%d", achar, (double)pos, fr); // start move to maximum (XY)
        marlin_wait_motion(100); // wait for motion start (max 100ms)
        phase++; // next phase
        break;
    case PH_NOT_Z_SPECIFIC+1: // phase 2 - wait while motion, then read position and start move to minimum
        if (marlin_motion())
            break; // axis is moving - wait
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
    case PH_NOT_Z_SPECIFIC+2: // phase 3 - wait while motion, then read position and start move to maximum
        if (marlin_motion())
            break; // axis is moving - wait
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
    case PH_NOT_Z_SPECIFIC+3: // phase 3 - wait while motion, then read position and finish
        if (marlin_motion())
            break; // axis is moving - wait
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
*/
int wizard_selftest_x(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_x == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(&axisXY, p_data, &(p_data->state_x), 0,
        _SELFTEST_X_FR, _SELFTEST_X_MIN, _SELFTEST_X_MAX, 1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_X_TIME, &(p_data->state_x), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_x_axis.win.id, p_data->state_x);
    return progress;
}

int wizard_selftest_y(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_y == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(&axisXY, p_data, &(p_data->state_y), 1,
        _SELFTEST_Y_FR, _SELFTEST_Y_MIN, _SELFTEST_Y_MAX, -1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_Y_TIME, &(p_data->state_y), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, 33.3F + (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_y_axis.win.id, p_data->state_y);
    return progress;
}

int wizard_selftest_z(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data) {
    if (p_data->state_z == _TEST_START)
        wizard_init_screen_selftest_fans_axis(id_body, p_screen, p_data);
    wizard_selftest_axis(&axisZ, p_data, &(p_data->state_z), 2,
        _SELFTEST_Z_FR, _SELFTEST_Z_MIN, _SELFTEST_Z_MAX, -1);
    int progress = wizard_timer(&p_screen->timer0, _SELFTEST_Z_TIME, &(p_data->state_z), _WIZ_TIMER);
    window_set_value(p_screen->progress_axis.win.id, 66.6F + (float)progress / 3);
    wizard_update_test_icon(p_screen->icon_z_axis.win.id, p_data->state_z);
    return progress;
}
