// firstlay.c

#include <stdio.h>
#include <string.h>

#include "firstlay.h"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "wizard_ui.h"
#include "wizard_types.h"
#include "wizard_progress_bar.h"
#include "guitypes.h" //font_meas_text
#include "menu_vars.h"
#include "filament.h"
#include "../lang/i18n.h"
#include <algorithm>
#define V__GCODES_HEAD_BEGIN                 \
    "M107",    /*fan off */                  \
        "G90", /*use absolute coordinates*/  \
        "M83", /*extruder relative mode*/    \
        "G21", /* set units to millimeters*/ \
        "G90", /* use absolute coordinates*/ \
        "M83", /* use relative distances for extrusion*/

//todo generate me
const char *V2_gcodes_head_PLA[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D215", //nozzle target
    "M140 S60",                           //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S60",                           //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S215",                          //nozzle target
    "M109 S215",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_PLA_sz = sizeof(V2_gcodes_head_PLA) / sizeof(V2_gcodes_head_PLA[0]);

const char *V2_gcodes_head_PETG[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D230", //nozzle target
    "M140 S85",                           //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S85",                           //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S230",                          //nozzle target
    "M109 S230",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_PETG_sz = sizeof(V2_gcodes_head_PETG) / sizeof(V2_gcodes_head_PETG[0]);

const char *V2_gcodes_head_ASA[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D260", //nozzle target
    "M140 S100",                          //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S100",                          //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S260",                          //nozzle target
    "M109 S260",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_ASA_sz = sizeof(V2_gcodes_head_ASA) / sizeof(V2_gcodes_head_ASA[0]);

const char *V2_gcodes_head_ABS[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D260", //nozzle target
    "M140 S100",                          //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S100",                          //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S255",                          //nozzle target
    "M109 S255",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_ABS_sz = sizeof(V2_gcodes_head_ABS) / sizeof(V2_gcodes_head_ABS[0]);

const char *V2_gcodes_head_PC[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D260", //nozzle target
    "M140 S100",                          //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S100",                          //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S275",                          //nozzle target
    "M109 S275",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_PC_sz = sizeof(V2_gcodes_head_PC) / sizeof(V2_gcodes_head_PC[0]);

const char *V2_gcodes_head_FLEX[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D240", //nozzle target
    "M140 S50",                           //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S50",                           //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S240",                          //nozzle target
    "M109 S240",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_FLEX_sz = sizeof(V2_gcodes_head_FLEX) / sizeof(V2_gcodes_head_FLEX[0]);

const char *V2_gcodes_head_HIPS[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D260", //nozzle target
    "M140 S100",                          //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S100",                          //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S220",                          //nozzle target
    "M109 S220",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_HIPS_sz = sizeof(V2_gcodes_head_HIPS) / sizeof(V2_gcodes_head_HIPS[0]);

const char *V2_gcodes_head_PP[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S" PREHEAT_TEMP_STRING " D260", //nozzle target
    "M140 S100",                          //bed target
    "M109 S" PREHEAT_TEMP_STRING,         //wait for nozzle temp
    "M190 S100",                          //wait for bed temp
    "G28",                                /*autohome*/
    "G29",                                /*meshbed leveling*/
    "M104 S240",                          //nozzle target
    "M109 S240",                          //wait for nozzle temp
};
const size_t V2_gcodes_head_PP_sz = sizeof(V2_gcodes_head_PP) / sizeof(V2_gcodes_head_PP[0]);

//EXTRUDE_PER_MM  0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.041575

//todo generate me
const char *V2_gcodes_body[] = {
    "G1 Z4 F1000",
    "G1 X0 Y-2 Z0.2 F3000.0",
    "G1 E6 F2000",
    "G1 X60 E9 F1000.0",
    "G1 X100 E12.5 F1000.0",
    "G1 Z2 E-6 F2100.00000",

    "G1 X10 Y150 Z0.2 F3000",
    "G1 E6 F2000"

    "G1 F1000",
    //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)
    "G1 X170 Y150 E5.322", //160 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 5.322
    "G1 X170 Y130 E0.665", //20 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 0.665
    "G1 X10  Y130 E5.322",
    "G1 X10  Y110 E0.665",
    "G1 X170 Y110 E5.322",
    "G1 X170 Y90  E0.665",
    "G1 X10  Y90  E5.322",
    "G1 X10  Y70  E0.665",
    "G1 X170 Y70  E5.322",
    "G1 X170 Y50  E0.665",
    "G1 X10  Y50  E5.322",

    //frame around
    "G1 X10    Y17    E1.371975",  //33 * 0.041575 = 1.371975
    "G1 X31    Y17    E1.288825",  //31 * 0.041575 = 1.288825
    "G1 X31    Y30.5  E0.5612625", //13.5 * 0.041575 = 0.5612625
    "G1 X10.5  Y30.5  E0.832",     //20 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.832
    "G1 X10.5  Y30.0  E0.0208",    //0.5 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.0208

    "G1 F1000",
    "G1 X30.5  Y30.0  E0.832",
    "G1 X30.5  Y29.5  E0.0208",
    "G1 X10.5  Y29.5  E0.832",
    "G1 X10.5  Y29.0  E0.0208",
    "G1 X30.5  Y29.0  E0.832",
    "G1 X30.5  Y28.5  E0.0208",
    "G1 X10.5  Y28.5  E0.832",
    "G1 X10.5  Y28.0  E0.0208",
    "G1 X30.5  Y28.0  E0.832",
    "G1 X30.5  Y27.5  E0.0208",
    "G1 X10.5  Y27.5  E0.832",
    "G1 X10.5  Y27.0  E0.0208",
    "G1 X30.5  Y27.0  E0.832",
    "G1 X30.5  Y26.5  E0.0208",
    "G1 X10.5  Y26.5  E0.832",
    "G1 X10.5  Y26.0  E0.0208",
    "G1 X30.5  Y26.0  E0.832",
    "G1 X30.5  Y25.5  E0.0208",
    "G1 X10.5  Y25.5  E0.832",
    "G1 X10.5  Y25.0  E0.0208",
    "G1 X30.5  Y25.0  E0.832",
    "G1 X30.5  Y24.5  E0.0208",
    "G1 X10.5  Y24.5  E0.832",
    "G1 X10.5  Y24.0  E0.0208",
    "G1 X30.5  Y24.0  E0.832",
    "G1 X30.5  Y23.5  E0.0208",
    "G1 X10.5  Y23.5  E0.832",
    "G1 X10.5  Y23.0  E0.0208",
    "G1 X30.5  Y23.0  E0.832",
    "G1 X30.5  Y22.5  E0.0208",
    "G1 X10.5  Y22.5  E0.832",
    "G1 X10.5  Y22.0  E0.0208",
    "G1 X30.5  Y22.0  E0.832",
    "G1 X30.5  Y21.5  E0.0208",
    "G1 X10.5  Y21.5  E0.832",
    "G1 X10.5  Y21.0  E0.0208",
    "G1 X30.5  Y21.0  E0.832",
    "G1 X30.5  Y20.5  E0.0208",
    "G1 X10.5  Y20.5  E0.832",
    "G1 X10.5  Y20.0  E0.0208",
    "G1 X30.5  Y20.0  E0.832",
    "G1 X30.5  Y19.5  E0.0208",
    "G1 X10.5  Y19.5  E0.832",
    "G1 X10.5  Y19.0  E0.0208",
    "G1 X30.5  Y19.0  E0.832",
    "G1 X30.5  Y18.5  E0.0208",
    "G1 X10.5  Y18.5  E0.832",
    "G1 X10.5  Y18.0  E0.0208",
    "G1 X30.5  Y18.0  E0.832",
    "G1 X30.5  Y17.5  E0.0208",

    "G1 Z2 E-6 F2100",
    "G1 X178 Y0 Z10 F3000",

    "G4",

    "M107",
    "M104 S0", // turn off temperature
    "M140 S0", // turn off heatbed
    "M84"      // disable motors
};
const size_t V2_gcodes_body_sz = sizeof(V2_gcodes_body) / sizeof(V2_gcodes_body[0]);

//todo use marlin api
const size_t commands_in_queue_size = 8;
const size_t commands_in_queue_use_max = 6;
const size_t max_gcodes_in_one_run = 20; //milion of small gcodes could be done instantly but block gui

static uint32_t line_head = 0;
static uint32_t line_body = 0;

static const char **head_gcode = NULL;
static const char **body_gcode = NULL;
static size_t head_gcode_sz = -1;
static size_t body_gcode_sz = -1;
static size_t gcode_sz = -1;
static size_t G28_pos = -1;
static size_t G29_pos = -1;

int _get_progress();

void _set_gcode_first_lines();

//returns remaining lines
#if DEBUG_TERM == 0
static const char *_wizard_firstlay_text = N_("Once the printer   \n"
                                              "starts extruding   \n"
                                              "plastic, adjust    \n"
                                              "the nozzle height  \n"
                                              "by turning the knob\n"
                                              "until the filament \n"
                                              "sticks to the print\n"
                                              "sheet.");
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count);
#else
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term);
#endif

void _wizard_firstlay_Z_step(firstlay_screen_t *p_screen);

void wizard_init_screen_firstlay(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data) {
    //marlin_vars_t* vars        = marlin_update_vars( MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) );
    //p_screen->Z_offset         = vars->z_offset;
    p_screen->Z_offset_request = 0;

    int16_t id;
    window_destroy_children(id_body);
    window_show(id_body);
    window_invalidate(id_body);

    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;
#if DEBUG_TERM == 0
    point_ui16_t pt;
    pt = font_meas_text(resource_font(IDR_FNT_NORMAL), _wizard_firstlay_text);
    pt.x += 5;
    pt.y += 5;
    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, pt.x, pt.y), &(p_screen->text_state));
    p_screen->text_state.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, _wizard_firstlay_text);

    y += pt.y + 5;
#else
    id = window_create_ptr(WINDOW_CLS_TERM, id_body,
        rect_ui16(10, y,
            resource_font(IDR_FNT_SMALL)->w * FIRSTLAY_SCREEN_TERM_X,
            resource_font(IDR_FNT_SMALL)->h * FIRSTLAY_SCREEN_TERM_Y),
        &(p_screen->term));
    p_screen->term.font = resource_font(IDR_FNT_SMALL);
    term_init(&(p_screen->terminal), FIRSTLAY_SCREEN_TERM_X, FIRSTLAY_SCREEN_TERM_Y, p_screen->term_buff);
    p_screen->term.term = &(p_screen->terminal);

    y += 18 * FIRSTLAY_SCREEN_TERM_Y + 3;
#endif
    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 110, 22), &(p_screen->text_Z_pos));
    window_set_text(id, "Z height:");

    id = window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(x + 110, y, 70, 22), &(p_screen->spin_baby_step));
    window_set_format(id, "%.3f");
    window_set_value(id, p_screen->Z_offset);
    p_screen->spin_baby_step.color_text = COLOR_GRAY;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x + 110 + 70, y, WIZARD_X_SPACE - x - 110 - 70, 22),
        &(p_screen->text_direction_arrow));
    window_set_text(id, "-|+");

    y += 22 + 10;

    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));
}

int wizard_firstlay_print(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    if (p_data->state_print == _TEST_START) {
        p_screen->state = _FL_INIT;
        p_data->state_print = _TEST_RUN;

        body_gcode = V2_gcodes_body;
        body_gcode_sz = V2_gcodes_body_sz;
        switch (get_filament()) {
        case FILAMENT_PETG:
            head_gcode = V2_gcodes_head_PETG;
            head_gcode_sz = V2_gcodes_head_PETG_sz;
            break;
        case FILAMENT_ASA:
            head_gcode = V2_gcodes_head_ASA;
            head_gcode_sz = V2_gcodes_head_ASA_sz;
            break;
        case FILAMENT_ABS:
            head_gcode = V2_gcodes_head_ABS;
            head_gcode_sz = V2_gcodes_head_ABS_sz;
            break;
        case FILAMENT_PC:
            head_gcode = V2_gcodes_head_PC;
            head_gcode_sz = V2_gcodes_head_PC_sz;
            break;
        case FILAMENT_FLEX:
            head_gcode = V2_gcodes_head_FLEX;
            head_gcode_sz = V2_gcodes_head_FLEX_sz;
            break;
        case FILAMENT_HIPS:
            head_gcode = V2_gcodes_head_HIPS;
            head_gcode_sz = V2_gcodes_head_HIPS_sz;
            break;
        case FILAMENT_PP:
            head_gcode = V2_gcodes_head_PP;
            head_gcode_sz = V2_gcodes_head_PP_sz;
            break;
        case FILAMENT_PLA:
        default:
            head_gcode = V2_gcodes_head_PLA;
            head_gcode_sz = V2_gcodes_head_PLA_sz;
            break;
        }

        gcode_sz = body_gcode_sz + head_gcode_sz;

        //G28 must be before G29, both must be present or head is invalid
        //find "G29" == MBL
        //FIXME use strstr() instead
        for (G29_pos = 0; (G29_pos < head_gcode_sz) && strcmp(head_gcode[G29_pos], "G29"); ++G29_pos)
            ; //no body
        //find "G28" == autohome needed for retry
        for (G28_pos = 0; (G28_pos < G29_pos) && strcmp(head_gcode[G28_pos], "G28"); ++G28_pos)
            ; //no body
        if (G28_pos >= G29_pos || G29_pos >= head_gcode_sz) {
            //error no G29
            p_data->state_print = _TEST_FAILED;
            return 100;
        }
    }

    int remaining_lines;
    switch (p_screen->state) {
    case _FL_INIT:
        p_screen->Z_offset = z_offset;
        wizard_init_screen_firstlay(id_body, p_screen, p_data);
#if DEBUG_TERM == 1
        term_printf(&p_screen->terminal, "INITIALIZED\n");
        window_invalidate(p_screen->term.win.id);
#endif
        _set_gcode_first_lines();
        p_screen->state = _FL_GCODE_HEAD;
        marlin_error_clr(MARLIN_ERR_ProbingFailed);
#if DEBUG_TERM == 1
        term_printf(&p_screen->terminal, "HEAD\n");
        window_invalidate(p_screen->term.win.id);
#endif
        break;
    case _FL_GCODE_HEAD:
        //have to wait to next state after MBL to check error
        if (line_head > G29_pos && marlin_error(MARLIN_ERR_ProbingFailed)) {
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            marlin_gcode("G0 Z30"); //Z 30mm
            marlin_gcode("M84");    //Disable steppers
            if (wizard_msgbox(_("Mesh bed leveling failed?"), MSGBOX_BTN_RETRYCANCEL, 0) == MSGBOX_RES_RETRY) {
                //RETRY
                line_head = G28_pos;
            } else {
                //CANCEL
                p_data->state_print = _TEST_FAILED;
                return 100;
            }
        }
#if DEBUG_TERM == 0
        remaining_lines = _run_gcode_line(&line_head, head_gcode,
            head_gcode_sz);
#else
        remaining_lines = _run_gcode_line(&line_head, head_gcode,
            head_gcode_sz, &p_screen->term);
#endif
        if (remaining_lines < 1) {
            p_screen->state = _FL_GCODE_BODY;
#if DEBUG_TERM == 1
            term_printf(&p_screen->terminal, "BODY\n");
            window_invalidate(p_screen->term.win.id);
#endif
            p_screen->Z_offset_request = 0; //ignore Z_offset_request variable changes until now
            p_screen->spin_baby_step.color_text = COLOR_ORANGE;
            window_invalidate(p_screen->spin_baby_step.win.id);
        }
        break;
    case _FL_GCODE_BODY:
        _wizard_firstlay_Z_step(p_screen);
#if DEBUG_TERM == 0
        remaining_lines = _run_gcode_line(&line_body, body_gcode,
            body_gcode_sz);
#else
        remaining_lines = _run_gcode_line(&line_body, body_gcode,
            body_gcode_sz, &p_screen->term);
#endif
        if (remaining_lines < 1) {
            p_screen->state = _FL_GCODE_DONE;
        }
        break;
    case _FL_GCODE_DONE:
#if DEBUG_TERM == 1
        term_printf(&p_screen->terminal, "PASSED\n");
        window_invalidate(p_screen->term.win.id);
#endif
        p_data->state_print = _TEST_PASSED;
        p_screen->Z_offset_request = 0;
        return 100;
    }

    int progress = _get_progress(); //max 99

    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

void wizard_firstlay_event_dn(firstlay_screen_t *p_screen) {
#if DEBUG_TERM == 1
    //todo term is bugged spinner can make it not showing
    window_invalidate(p_screen->term.win.id);
#endif
    p_screen->Z_offset_request -= z_offset_step;
}

void wizard_firstlay_event_up(firstlay_screen_t *p_screen) {
#if DEBUG_TERM == 1
    //todo term is bugged spinner can make it not showing
    window_invalidate(p_screen->term.win.id);
#endif
    p_screen->Z_offset_request += z_offset_step;
}

void _wizard_firstlay_Z_step(firstlay_screen_t *p_screen) {
    int16_t numb_id = p_screen->spin_baby_step.win.id;
    int16_t arrow_id = p_screen->text_direction_arrow.win.id;

    //need last step to ensure correct behavior on limits
    float _step_last = p_screen->Z_offset;
    p_screen->Z_offset += p_screen->Z_offset_request;

    if (p_screen->Z_offset > z_offset_max)
        p_screen->Z_offset = z_offset_max;
    if (p_screen->Z_offset < z_offset_min)
        p_screen->Z_offset = z_offset_min;

    marlin_do_babysteps_Z(p_screen->Z_offset - _step_last);

    if (p_screen->Z_offset_request > 0) {
        window_set_value(numb_id, p_screen->Z_offset);
        window_set_text(arrow_id, "+++");
    } else if (p_screen->Z_offset_request < 0) {
        window_set_value(numb_id, p_screen->Z_offset);
        window_set_text(arrow_id, "---");
    }

    p_screen->Z_offset_request = 0;
}

int _get_progress() {
    //if ( _is_gcode_end_line() ) return 100;
    return std::min(99, int(100 * (line_head + 1 + line_body + 1) / gcode_sz));
}

//returns progress
void _set_gcode_first_lines() {
    line_head = 0;
    line_body = 0;
}

#if DEBUG_TERM == 0
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count)
#else
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term)
#endif
{
    size_t gcodes_in_this_run = 0;

    //todo "while" does not work ...why?, something with  commands_in_queue?
    //while(commands_in_queue < commands_in_queue_use_max){
    if (marlin_get_gqueue() < 1) {
        if ((*p_line) < gcodes_count) {
            ++gcodes_in_this_run;
            marlin_gcode(gcodes[*p_line]);
#if DEBUG_TERM == 1
            term_printf(term->term, "%s\n", gcodes[*p_line]);
            window_invalidate(term->win.id);
#endif
            ++(*p_line);
        }
    }

    //commands_in_queue does not reflect commands added in this run
    return gcodes_count - (*p_line) + marlin_get_gqueue() + gcodes_in_this_run;
}
