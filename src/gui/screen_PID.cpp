/*
 * screen_PID.c
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */

#include "config.h"
#include "eeprom.h"
#include "screens.h"
#include "../lang/i18n.h"

#ifdef PIDCALIBRATION

    #include "gui.h"
    #include "status_footer.h"
    #include "math.h"
    #include "../Marlin/src/module/temperature.h"
    #include "marlin_client.h"

    #define SPIN_DIGITS     6
    #define SPIN_PRECISION  2
    #define SPIN_INT_DIGITS (SPIN_DIGITS - SPIN_PRECISION)

enum { _BED = -1,
    _EXTRUDER = 0 };
typedef enum {
    AT_idle,
    AT_extruder,
    AT_bed //,
    //AT_extruder_done,
    //AT_bed_done
} autotune_state_t;

struct _PID_t {
    int16_t ID;
    float *autotune_temp;
    float raw_Ki;
    float raw_Kd;
    float *p_Kp;
    float *p_Ki;
    float *p_Kd;
    float Kp_last;
    float Ki_last;
    float Kd_last;
};

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;

    window_text_t btAutoTuneApply_E;
    window_spin_t spinAutoTn_E;
    window_list_t list_RW_E; //choose read write PID
    window_spin_t spinKp_E[SPIN_DIGITS];
    window_spin_t spinKi_E[SPIN_DIGITS];
    window_spin_t spinKd_E[SPIN_DIGITS];

    window_text_t btAutoTuneApply_B;
    window_spin_t spinAutoTn_B;
    window_list_t list_RW_B; //choose read write PID
    window_spin_t spinKp_B[SPIN_DIGITS];
    window_spin_t spinKi_B[SPIN_DIGITS];
    window_spin_t spinKd_B[SPIN_DIGITS];

    window_text_t textExit;
    status_footer_t footer;

    size_t list_RW_E_index_actual;
    size_t list_RW_E_index_last;
    size_t list_RW_B_index_actual;
    size_t list_RW_B_index_last;

    uint16_t dot_coordsKp_E[2];
    uint16_t dot_coordsKi_E[2];
    uint16_t dot_coordsKd_E[2];

    uint16_t dot_coordsKp_B[2];
    uint16_t dot_coordsKi_B[2];
    uint16_t dot_coordsKd_B[2];

    rect_ui16_t rect_E;
    rect_ui16_t rectKp_E;
    rect_ui16_t rectKi_E;
    rect_ui16_t rectKd_E;

    rect_ui16_t rect_B;
    rect_ui16_t rectKp_B;
    rect_ui16_t rectKi_B;
    rect_ui16_t rectKd_B;

    int16_t idsDigits_Kp_E[6];
    int16_t idsDigits_Ki_E[6];
    int16_t idsDigits_Kd_E[6];

    int16_t idsDigits_Kp_B[6];
    int16_t idsDigits_Ki_B[6];
    int16_t idsDigits_Kd_B[6];

    float autotune_temp_B;
    float autotune_temp_E;

    _PID_t _PID_E;
    _PID_t _PID_B;

    autotune_state_t autotune_state;

    int redraw;

} screen_PID_data_t;

    #define pd ((screen_PID_data_t *)screen->pdata)

    #define AUTO_TN_DEFAULT_CL COLOR_WHITE
    #define AUTO_TN_ACTIVE_CL  COLOR_RED

enum {
    TAG_QUIT = 10,
    TAG_AUTOTUNE_APPLY_E,
    TAG_CHANGE_VAL_E,
    TAG_AUTOTUNE_APPLY_B,
    TAG_CHANGE_VAL_B,
    TAG_RW_E,
    TAG_RW_B

};

//-----------------------------------------------------------------------------
//methods

//-----------------------------------------------------------------------------
//pid
void _PID_copy_and_scale_PID_i(_PID_t *ths);

void _PID_copy_and_scale_PID_d(_PID_t *ths);

void _PID_autotune(_PID_t *ths);

//0	the contents of both memory blocks are equal
int _PID_actualize(_PID_t *ths);

void _PID_ctor(_PID_t *ths, int16_t id, float *autotune_temp);

void _PID_set(_PID_t *ths, float Kp, float Ki, float Kd);

//-----------------------------------------------------------------------------
//autotune
bool __autotune(screen_t *screen, autotune_state_t *a_tn_st);

void _autotune_B(screen_t *screen, autotune_state_t *a_tn_st);

void _autotune_E(screen_t *screen, autotune_state_t *a_tn_st);

//-----------------------------------------------------------------------------
//gui spins
void generate_spin_single_digit(int16_t &id0, int16_t &id, window_spin_t &spin,
    uint16_t &col, uint16_t row, uint16_t offset, uint16_t row_h);

void enable_digits_write_mode(int16_t *ids, size_t sz);

void disable_digits_write_mode(int16_t *ids, size_t sz);

//i am not using size_t numOfDigits, size_t precision
//need to be signed because of for cycles
void generate_spin_digits(screen_t *screen, int numOfDigits, int precision,
    int16_t &id0, int16_t *ids, window_spin_t *pSpin, uint16_t dot_coords[2],
    uint16_t col, uint16_t row, uint16_t row_h);

//-----------------------------------------------------------------------------
//list
const char *list_RW_strings[] = { "READ", "WRITE" };
    #define list_RW_strings_sz (sizeof(list_RW_strings) / sizeof(const char *))

void window_list_RW_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon);

//-----------------------------------------------------------------------------
//screen update methods
void disp_single_PID_digit(uint8_t singlePIDdigit, int16_t id);

void disp_single_PID_param(uint32_t singlePIDparam,
    int16_t *ids, size_t numOfDigits);

void dispPID(_PID_t &PID, int16_t *KpIds, int16_t *KiIds, int16_t *KdIds,
    size_t numOfDigits, size_t precision);

uint8_t get_single_PIDdigitFromDisp(int16_t id);

//i am not using size_t, need negative values in for cycle
float get_single_PIDparamFromDisp(int16_t *ids, int numOfDigits, int precision);

//-----------------------------------------------------------------------------
//btn autotune / apply
static const char *btnAutoTuneOrApplystrings[] = { N_("AutTn"), N_("Apply") };
    #define btnAutoTuneOrApplystrings_sz (sizeof(btnAutoTuneOrApplystrings) / sizeof(const char *))

void screen_PID_init(screen_t *screen) {
    pd->redraw = 1;
    pd->list_RW_E_index_actual = 0;
    pd->list_RW_E_index_last = 0;
    pd->list_RW_B_index_actual = 0;
    pd->list_RW_B_index_last = 0;
    pd->autotune_state = AT_idle;
    pd->autotune_temp_E = 150;
    pd->autotune_temp_B = 70;

    _PID_ctor(&(pd->_PID_E), _EXTRUDER, &pd->autotune_temp_E);
    _PID_ctor(&(pd->_PID_B), _BED, &pd->autotune_temp_B);

    int16_t id;
    uint16_t col = 2;
    uint16_t row2draw = 0;
    uint16_t row_h = 22;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME,
        -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(0, 0, display::GetW(), row_h), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"PID adjustment");

    //EXTRUDER
    row2draw = row_h;

    pd->rect_E = rect_ui16(col, row2draw, 100, row_h);
    row2draw += row_h;

    id = window_create_ptr(WINDOW_CLS_LIST,
        id0, rect_ui16(col, row2draw, 100, row_h), &(pd->list_RW_E));
    window_set_item_count(id, list_RW_strings_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_RW_item);
    window_set_tag(id, TAG_RW_E);

    row2draw += 25;

    pd->rectKp_E = rect_ui16(col, row2draw, 25, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Kp_E, pd->spinKp_E, pd->dot_coordsKp_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    pd->rectKi_E = rect_ui16(col, row2draw, 30, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Ki_E, pd->spinKi_E, pd->dot_coordsKi_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    pd->rectKd_E = rect_ui16(col, row2draw, 50, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Kd_E, pd->spinKd_E, pd->dot_coordsKd_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 68, row_h),
        &(pd->btAutoTuneApply_E));
    window_set_text(id, _(btnAutoTuneOrApplystrings[0]));
    window_enable(id);
    window_set_tag(id, TAG_AUTOTUNE_APPLY_E);

    id = window_create_ptr(WINDOW_CLS_SPIN,
        id0, rect_ui16(col + 70, row2draw, 40, row_h),
        &(pd->spinAutoTn_E));
    window_set_format(id, "%f");
    window_set_min_max_step(id, 100.0F, 250.0F, 5.0F);
    window_set_value(id, pd->autotune_temp_E);
    row2draw += row_h;

    //BED
    col = 122;
    row2draw = row_h;

    pd->rect_B = rect_ui16(col, row2draw, 100, row_h);
    row2draw += row_h;

    id = window_create_ptr(WINDOW_CLS_LIST, id0,
        rect_ui16(col, row2draw, 100, row_h), &(pd->list_RW_B));
    window_set_item_count(id, list_RW_strings_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_RW_item);
    window_set_tag(id, TAG_RW_B);

    row2draw += 25;

    pd->rectKp_B = rect_ui16(col, row2draw, 25, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Kp_B, pd->spinKp_B, pd->dot_coordsKp_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    pd->rectKi_B = rect_ui16(col, row2draw, 30, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Ki_B, pd->spinKi_B, pd->dot_coordsKi_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    pd->rectKd_B = rect_ui16(col, row2draw, 50, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, pd->idsDigits_Kd_B, pd->spinKd_B, pd->dot_coordsKd_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 68, row_h),
        &(pd->btAutoTuneApply_B));
    window_set_text(id, _(btnAutoTuneOrApplystrings[0]));
    window_enable(id);
    window_set_tag(id, TAG_AUTOTUNE_APPLY_B);

    id = window_create_ptr(WINDOW_CLS_SPIN,
        id0, rect_ui16(col + 70, row2draw, 40, row_h),
        &(pd->spinAutoTn_B));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, 50.0F, 110.0F, 5.0F);
    window_set_value(id, pd->autotune_temp_B);
    row2draw += row_h;

    //exit and footer

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 245, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);

    status_footer_init(&(pd->footer), id0);

    dispPID((pd->_PID_E), pd->idsDigits_Kp_E, pd->idsDigits_Ki_E, pd->idsDigits_Kd_E,
        SPIN_DIGITS, SPIN_PRECISION);
    dispPID((pd->_PID_B), pd->idsDigits_Kp_B, pd->idsDigits_Ki_B, pd->idsDigits_Kd_B,
        SPIN_DIGITS, SPIN_PRECISION);
}

void screen_PID_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_PID_draw(screen_t *screen) {
}

int screen_PID_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (status_footer_event(&(pd->footer), window, event, param)) {
        return 1;
    }

    status_footer_event(&(pd->footer), window, event, param);
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            if (pd->autotune_state != AT_idle)
                return 0; //button should not be accessible
            screen_close();
            return 1;
        case TAG_AUTOTUNE_APPLY_E:
            if (pd->list_RW_E_index_actual == 0) {
                //run autotune
                _autotune_E(screen, &(pd->autotune_state));
            } else {
                //apply values
                _PID_set(&(pd->_PID_E),
                    get_single_PIDparamFromDisp(pd->idsDigits_Kp_E,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(pd->idsDigits_Ki_E,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(pd->idsDigits_Kd_E,
                        SPIN_DIGITS, SPIN_PRECISION));
                eeprom_set_var(EEVAR_PID_NOZ_P, variant8_flt(Temperature::temp_hotend[0].pid.Kp));
                eeprom_set_var(EEVAR_PID_NOZ_I, variant8_flt(Temperature::temp_hotend[0].pid.Ki));
                eeprom_set_var(EEVAR_PID_NOZ_D, variant8_flt(Temperature::temp_hotend[0].pid.Kd));
            }
            break;
        case TAG_AUTOTUNE_APPLY_B:
            if (pd->list_RW_B_index_actual == 0) {
                //run autotune
                _autotune_B(screen, &(pd->autotune_state));
            } else {
                //apply values
                _PID_set(&(pd->_PID_B),
                    get_single_PIDparamFromDisp(pd->idsDigits_Kp_B,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(pd->idsDigits_Ki_B,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(pd->idsDigits_Kd_B,
                        SPIN_DIGITS, SPIN_PRECISION));
                eeprom_set_var(EEVAR_PID_BED_P, variant8_flt(Temperature::temp_bed.pid.Kp));
                eeprom_set_var(EEVAR_PID_BED_I, variant8_flt(Temperature::temp_bed.pid.Ki));
                eeprom_set_var(EEVAR_PID_BED_D, variant8_flt(Temperature::temp_bed.pid.Kd));
            }
            break;
        case TAG_CHANGE_VAL_E:
            //not used, there is apply button
            break;
        case TAG_CHANGE_VAL_B:
            //not used, there is apply button
            break;
        }
    if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case TAG_RW_E:
            break;
        case TAG_RW_B:
            break;
        }
    }
    if (event == WINDOW_EVENT_LOOP) {

        if (pd->autotune_state != AT_idle) {
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) //wait for MARLIN_EVT_CommandEnd
            {
                window_set_color_text(pd->btAutoTuneApply_B.win.id, AUTO_TN_DEFAULT_CL);
                window_set_color_text(pd->btAutoTuneApply_E.win.id, AUTO_TN_DEFAULT_CL);
                window_set_color_text(pd->textExit.win.id, AUTO_TN_DEFAULT_CL);
                window_enable(pd->textExit.win.id);
                pd->autotune_state = AT_idle;
            }
        }

        pd->autotune_temp_E = window_get_value(pd->spinAutoTn_E.window.win.id);
        pd->autotune_temp_B = window_get_value(pd->spinAutoTn_B.window.win.id);

        pd->list_RW_E_index_actual = window_get_item_index(pd->list_RW_E.win.id);
        if (pd->list_RW_E_index_actual != pd->list_RW_E_index_last) {
            if (pd->list_RW_E_index_actual == 0) {
                disable_digits_write_mode(pd->idsDigits_Kp_E,
                    sizeof(pd->idsDigits_Kp_E) / sizeof(pd->idsDigits_Kp_E[0]));
                disable_digits_write_mode(pd->idsDigits_Ki_E,
                    sizeof(pd->idsDigits_Ki_E) / sizeof(pd->idsDigits_Ki_E[0]));
                disable_digits_write_mode(pd->idsDigits_Kd_E,
                    sizeof(pd->idsDigits_Kd_E) / sizeof(pd->idsDigits_Kd_E[0]));
                window_set_text(pd->btAutoTuneApply_E.win.id,
                    _(btnAutoTuneOrApplystrings[0]));
            } else {
                enable_digits_write_mode(pd->idsDigits_Kp_E,
                    sizeof(pd->idsDigits_Kp_E) / sizeof(pd->idsDigits_Kp_E[0]));
                enable_digits_write_mode(pd->idsDigits_Ki_E,
                    sizeof(pd->idsDigits_Ki_E) / sizeof(pd->idsDigits_Ki_E[0]));
                enable_digits_write_mode(pd->idsDigits_Kd_E,
                    sizeof(pd->idsDigits_Kd_E) / sizeof(pd->idsDigits_Kd_E[0]));
                window_set_text(pd->btAutoTuneApply_E.win.id,
                    _(btnAutoTuneOrApplystrings[1]));
            }

            pd->list_RW_E_index_last = pd->list_RW_E_index_actual;
        }

        pd->list_RW_B_index_actual = window_get_item_index(pd->list_RW_B.win.id);
        if (pd->list_RW_B_index_actual != pd->list_RW_B_index_last) {
            if (pd->list_RW_B_index_actual == 0) {
                disable_digits_write_mode(pd->idsDigits_Kp_B,
                    sizeof(pd->idsDigits_Kp_B) / sizeof(pd->idsDigits_Kp_B[0]));
                disable_digits_write_mode(pd->idsDigits_Ki_B,
                    sizeof(pd->idsDigits_Ki_B) / sizeof(pd->idsDigits_Ki_B[0]));
                disable_digits_write_mode(pd->idsDigits_Kd_B,
                    sizeof(pd->idsDigits_Kd_B) / sizeof(pd->idsDigits_Kd_B[0]));
                window_set_text(pd->btAutoTuneApply_B.win.id,
                    _(btnAutoTuneOrApplystrings[0]));
            } else {
                enable_digits_write_mode(pd->idsDigits_Kp_B,
                    sizeof(pd->idsDigits_Kp_B) / sizeof(pd->idsDigits_Kp_B[0]));
                enable_digits_write_mode(pd->idsDigits_Ki_B,
                    sizeof(pd->idsDigits_Ki_B) / sizeof(pd->idsDigits_Ki_B[0]));
                enable_digits_write_mode(pd->idsDigits_Kd_B,
                    sizeof(pd->idsDigits_Kd_B) / sizeof(pd->idsDigits_Kd_B[0]));
                window_set_text(pd->btAutoTuneApply_B.win.id,
                    _(btnAutoTuneOrApplystrings[1]));
            }

            pd->list_RW_B_index_last = pd->list_RW_B_index_actual;
        }

        if (pd->redraw) {
            pd->redraw = 0;
            display::FillRect(rect_ui16(pd->dot_coordsKp_E[0],
                                  pd->dot_coordsKp_E[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(pd->dot_coordsKi_E[0],
                                  pd->dot_coordsKi_E[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(pd->dot_coordsKd_E[0],
                                  pd->dot_coordsKd_E[1], 2, 2),
                COLOR_WHITE);

            display::DrawText(pd->rect_E, "NOZZLE", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKp_E, "Kp", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKi_E, "Ki", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKd_E, "Kd", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);

            display::FillRect(rect_ui16(pd->dot_coordsKp_B[0],
                                  pd->dot_coordsKp_B[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(pd->dot_coordsKi_B[0],
                                  pd->dot_coordsKi_B[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(pd->dot_coordsKd_B[0],
                                  pd->dot_coordsKd_B[1], 2, 2),
                COLOR_WHITE);

            display::DrawText(pd->rect_B, "BED", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKp_B, "Kp", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKi_B, "Ki", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(pd->rectKd_B, "Kd", resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
        }

        if (_PID_actualize(&(pd->_PID_E)) != 0) {
            dispPID((pd->_PID_E), pd->idsDigits_Kp_E, pd->idsDigits_Ki_E, pd->idsDigits_Kd_E,
                SPIN_DIGITS, SPIN_PRECISION);
        }
        if (_PID_actualize(&(pd->_PID_B)) != 0) {
            dispPID((pd->_PID_B), pd->idsDigits_Kp_B, pd->idsDigits_Ki_B, pd->idsDigits_Kd_B,
                SPIN_DIGITS, SPIN_PRECISION);
        }
    }
    return 0;
}

screen_t screen_PID = {
    0,
    0,
    screen_PID_init,
    screen_PID_done,
    screen_PID_draw,
    screen_PID_event,
    sizeof(screen_PID_data_t), //data_size
    0,                         //pdata
};

screen_t *const get_scr_PID() { return &screen_PID; }

//-----------------------------------------------------------------------------

void _PID_copy_and_scale_PID_i(_PID_t *ths) {
    *ths->p_Ki = scalePID_i(ths->raw_Ki);
    thermalManager.updatePID();
}

void _PID_copy_and_scale_PID_d(_PID_t *ths) {
    *ths->p_Kd = scalePID_d(ths->raw_Kd);
    thermalManager.updatePID();
}

void _PID_autotune(_PID_t *ths) {
    marlin_event_clr(MARLIN_EVT_CommandEnd);
    marlin_gcode_printf("M303 U1 E%i S%i", ths->ID, int(*ths->autotune_temp));
    if (ths->ID >= 0) {
        eeprom_set_var(EEVAR_PID_NOZ_P, variant8_flt(Temperature::temp_hotend[0].pid.Kp));
        eeprom_set_var(EEVAR_PID_NOZ_I, variant8_flt(Temperature::temp_hotend[0].pid.Ki));
        eeprom_set_var(EEVAR_PID_NOZ_D, variant8_flt(Temperature::temp_hotend[0].pid.Kd));
    } else {
        eeprom_set_var(EEVAR_PID_BED_P, variant8_flt(Temperature::temp_bed.pid.Kp));
        eeprom_set_var(EEVAR_PID_BED_I, variant8_flt(Temperature::temp_bed.pid.Ki));
        eeprom_set_var(EEVAR_PID_BED_D, variant8_flt(Temperature::temp_bed.pid.Kd));
    }
}

//0	the contents of both memory blocks are equal
int _PID_actualize(_PID_t *ths) {
    int ret_Kp = memcmp(&(ths->Kp_last), ths->p_Kp, sizeof(ths->Kp_last));
    int ret_Ki = memcmp(&(ths->Ki_last), ths->p_Ki, sizeof(ths->Ki_last));
    int ret_Kd = memcmp(&(ths->Kd_last), ths->p_Kd, sizeof(ths->Kd_last));

    int ret = ret_Kp | ret_Ki | ret_Kd;

    if (ret != 0) {
        ths->raw_Ki = unscalePID_i(*ths->p_Ki); // 0.01f, 9990
        ths->raw_Kd = unscalePID_d(*ths->p_Kd); //1, 9990
        //ths->raw_Ki = unscalePID_i(PID_PARAM(Ki, ths->ID));// 0.01f, 9990
        //ths->raw_Kd = unscalePID_d(PID_PARAM(Kd, ths->ID));//1, 9990

        ths->Kp_last = *ths->p_Kp;
        ths->Ki_last = *ths->p_Ki;
        ths->Kd_last = *ths->p_Kd;
    }

    return ret;
}

void _PID_ctor(_PID_t *ths, int16_t id, float *autotune_temp) {
    ths->ID = id;
    ths->autotune_temp = autotune_temp;

    //stupid Marlin has bed PID defined unlike extruder
    if (id >= 0) {
        ths->p_Kp = &PID_PARAM(Kp, ths->ID);
        ths->p_Ki = &PID_PARAM(Ki, ths->ID);
        ths->p_Kd = &PID_PARAM(Kd, ths->ID);
    } else {
        //should i use temp_bed.pid or tune_pid?
        ths->p_Kp = &Temperature::temp_bed.pid.Kp;
        ths->p_Ki = &Temperature::temp_bed.pid.Ki;
        ths->p_Kd = &Temperature::temp_bed.pid.Kd;
    }

    ths->raw_Ki = unscalePID_i(*ths->p_Ki);
    ths->raw_Kd = unscalePID_d(*ths->p_Kd);

    //to be able to check changes
    ths->Kp_last = *ths->p_Kp;
    ths->Ki_last = *ths->p_Ki;
    ths->Kd_last = *ths->p_Kd;
}

void _PID_set(_PID_t *ths, float Kp, float Ki, float Kd) {
    *ths->p_Kp = Kp;
    ths->raw_Ki = Ki;
    ths->raw_Kd = Kd;
    _PID_copy_and_scale_PID_i(ths);
    _PID_copy_and_scale_PID_d(ths);
}

//-----------------------------------------------------------------------------
//autotune
bool __autotune(screen_t *screen, autotune_state_t *a_tn_st) {
    if ((*a_tn_st) != AT_idle)
        return 0;
    window_disable(pd->textExit.win.id);
    window_set_color_text(pd->textExit.win.id, AUTO_TN_ACTIVE_CL);
    return 1;
}

void _autotune_B(screen_t *screen, autotune_state_t *a_tn_st) {
    if (__autotune(screen, a_tn_st)) {
        _PID_autotune(&(pd->_PID_B));
        window_set_color_text(pd->btAutoTuneApply_B.win.id, AUTO_TN_ACTIVE_CL);
        *a_tn_st = AT_extruder;
    }
}

void _autotune_E(screen_t *screen, autotune_state_t *a_tn_st) {
    if (__autotune(screen, a_tn_st)) {
        _PID_autotune(&(pd->_PID_E));
        window_set_color_text(pd->btAutoTuneApply_E.win.id, AUTO_TN_ACTIVE_CL);
        *a_tn_st = AT_bed;
    }
}

//-----------------------------------------------------------------------------
//gui spins
void generate_spin_single_digit(int16_t &id0, int16_t &id, window_spin_t &spin,
    uint16_t &col, uint16_t row, uint16_t offset, uint16_t row_h) {
    id = window_create_ptr(WINDOW_CLS_SPIN, id0,
        rect_ui16(col, row, offset, row_h), &spin);
    spin.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%d");
    window_set_min_max_step(id, 0.0F, 9.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
};

void enable_digits_write_mode(int16_t *ids, size_t sz) {
    for (size_t i = 0; i < sz; ++i)
        window_enable(ids[i]);
}

void disable_digits_write_mode(int16_t *ids, size_t sz) {
    for (size_t i = 0; i < sz; ++i)
        window_disable(ids[i]);
}

//i am not using size_t numOfDigits, size_t precision
//need to be signed because of for cycles
void generate_spin_digits(screen_t *screen, int numOfDigits, int precision,
    int16_t &id0, int16_t *ids, window_spin_t *pSpin, uint16_t dot_coords[2],
    uint16_t col, uint16_t row, uint16_t row_h) {
    int16_t offset = 12;
    dot_coords[0] = col + offset * (numOfDigits - precision) + 1;
    dot_coords[1] = row + 14;

    for (int i = numOfDigits - 1; i >= precision; --i) {
        generate_spin_single_digit(id0, ids[i], pSpin[i], col, row, offset, row_h);
    }

    col += 3;

    for (int i = precision - 1; i >= 0; --i) {
        generate_spin_single_digit(id0, ids[i], pSpin[i], col, row, offset, row_h);
    }

    disable_digits_write_mode(ids, numOfDigits);
}

//-----------------------------------------------------------------------------
//screen update methods
void disp_single_PID_digit(uint8_t singlePIDdigit, int16_t id) {
    if (window_get_item_index(id) != singlePIDdigit) {
        window_set_item_index(id, singlePIDdigit);
        //window_set_value(id,singlePIDdigit);
    }
}

void disp_single_PID_param(uint32_t singlePIDparam,
    int16_t *ids, size_t numOfDigits) {
    for (size_t i = 0; i < numOfDigits; ++i) {
        disp_single_PID_digit(singlePIDparam % 10, ids[i]);
        singlePIDparam = singlePIDparam / 10;
    }
}

void dispPID(_PID_t &PID, int16_t *KpIds, int16_t *KiIds, int16_t *KdIds,
    size_t numOfDigits, size_t precision) {
    float multiplier = pow(10.0F, float(precision));
    uint32_t Kp = PID.Kp_last * multiplier;
    uint32_t Ki = PID.raw_Ki * multiplier; //scaled
    uint32_t Kd = PID.raw_Kd * multiplier; //scaled

    disp_single_PID_param(Kp, KpIds, numOfDigits);
    disp_single_PID_param(Ki, KiIds, numOfDigits);
    disp_single_PID_param(Kd, KdIds, numOfDigits);
}

uint8_t get_single_PIDdigitFromDisp(int16_t id) {
    return window_get_item_index(id);
}

//i am not using size_t, need negative values in for cycle
float get_single_PIDparamFromDisp(int16_t *ids, int numOfDigits, int precision) {
    int val = 0;
    for (int i = numOfDigits - 1; i >= 0; --i) {
        val *= 10;
        val += get_single_PIDdigitFromDisp(ids[i]);
    }

    float multiplier = pow(10.0F, float(precision));
    return float(val) / multiplier;
}

//-----------------------------------------------------------------------------
//list
void window_list_RW_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index < list_RW_strings_sz)
        *pptext = list_RW_strings[index];
    else
        *pptext = "Index ERROR";
    *pid_icon = 0;
}
#endif //PIDCALIBRATION
