/*
 * screen_PID.cpp
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */
#if 0
    #include "config.h"
    #include "eeprom.h"
    #include "../lang/i18n.h"

//#ifdef PIDCALIBRATION

    #include "screen_PID.hpp"
    #include "status_footer.h"
    #include "math.h"
    #include "../Marlin/src/module/temperature.h"
    #include "marlin_client.h"



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

    #define pd ((screen_PID_data_t *)screen->pdata)

    #define AUTO_TN_DEFAULT_CL           COLOR_WHITE
    #define AUTO_TN_ACTIVE_CL            COLOR_RED

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
void generate_spin_single_digit(int16_t &id0, window_spin_t &spin,
    uint16_t &col, uint16_t row, uint16_t offset, uint16_t row_h);

void enable_digits_write_mode(window_spin_t *spiners, size_t sz);

void disable_digits_write_mode(window_spin_t *spiners, size_t sz);

//i am not using size_t numOfDigits, size_t precision
//need to be signed because of for cycles
void generate_spin_digits(screen_t *screen, int numOfDigits, int precision,
    int16_t &id0, window_spin_t *pSpin, uint16_t dot_coords[2],
    uint16_t col, uint16_t row, uint16_t row_h);

//-----------------------------------------------------------------------------
//list
const char *list_RW_strings[] = { "READ", "WRITE" };
    #define list_RW_strings_sz           (sizeof(list_RW_strings) / sizeof(const char *))

void window_list_RW_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon);

//-----------------------------------------------------------------------------
//screen update methods
void disp_single_PID_digit(uint8_t singlePIDdigit, window_spin_t *pspin);

void disp_single_PID_param(uint32_t singlePIDparam,
    window_spin_t *spins, size_t numOfDigits);

void dispPID(_PID_t &PID, window_spin_t *Kps, window_spin_t *Kis, window_spin_t *Kds,
    size_t numOfDigits, size_t precision);

//i am not using size_t, need negative values in for cycle
float get_single_PIDparamFromDisp(window_spin_t *spins, int numOfDigits, int precision);

//-----------------------------------------------------------------------------
//btn autotune / apply
static const char *btnAutoTuneOrApplystrings[] = { N_("AutTn"), N_("Apply") };
    #define btnAutoTuneOrApplystrings_sz (sizeof(btnAutoTuneOrApplystrings) / sizeof(const char *))


static const uint16_t row_h = 22;
static const uint16_t left_col = 22;

screen_PID_data_t::screen_PID_data_t()
    : window_frame_t(&footer)
    , footer(this)

    , textMenuName(this, rect_ui16(0, 0, display::GetW(), row_h),_("PID adjustment"))

    , btAutoTuneApply_E(this, )
    , spinAutoTn_E(this, )
    , list_RW_E(this, rect_ui16(left_col, 2*row_h, 100, row_h)) //choose read write PID
    , spinKp_E {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}
    , spinKi_E {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}
    , spinKd_E {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}

    , btAutoTuneApply_B(this, )
    , spinAutoTn_B(this, )
    , list_RW_B(this, ) //choose read write PID
    , spinKp_B {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}
    , spinKi_B {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}
    , spinKd_B {{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}},{this, {0}}}

    , list_RW_E_index_actual(0)
    , list_RW_E_index_last(0)
    , list_RW_B_index_actual(0)
    , list_RW_B_index_last(0)

    , rect_E(rect_ui16(left_col, row_h, 100, row_h))
    , rectKp_E (rect_ui16(left_col, 2*row_h+25, 25, row_h))
    , rectKi_E (rect_ui16(left_col, 3*row_h+25, 30, row_h))
    , rectKd_E (rect_ui16(left_col, 4*row_h+25, 50, row_h))

    , rect_B;
    , rectKp_B;
    , rectKi_B;
    , rectKd_B;

    , autotune_temp_E(150)
    , autotune_temp_B(70)

    , autotune_state(AT_idle)
    , redraw(true)
{

    textMenuName.font = resource_font(IDR_FNT_BIG);



    _PID_ctor(&(_PID_E), _EXTRUDER, &autotune_temp_E);
    _PID_ctor(&(_PID_B), _BED, &autotune_temp_B);

    uint16_t col = 2;



    //EXTRUDER
    uint16_t row2draw = row_h;

    rect_E = rect_ui16(col, row2draw, 100, row_h);
    row2draw += row_h;


    list_RW_E.SetItemCount(list_RW_strings_sz);
    list_RW_E.SetItemIndex(0);
    list_RW_E.SetCallback(window_list_RW_item);
    list_RW_E.SetTag(TAG_RW_E);

    row2draw += 25;

    rectKp_E = rect_ui16(col, row2draw, 25, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKp_E, dot_coordsKp_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    rectKi_E = rect_ui16(col, row2draw, 30, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKi_E, dot_coordsKi_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    rectKd_E = rect_ui16(col, row2draw, 50, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKd_E, dot_coordsKd_E,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 68, row_h),
        &(btAutoTuneApply_E));
    btAutoTuneApply_E.SetText(_(btnAutoTuneOrApplystrings[0]));
    btAutoTuneApply_E.Enable();
    btAutoTuneApply_E.SetTag(TAG_AUTOTUNE_APPLY_E);

    window_create_ptr(WINDOW_CLS_SPIN,
        id0, rect_ui16(col + 70, row2draw, 40, row_h),
        &(spinAutoTn_E));
    spinAutoTn_E.SetFormat("%f");
    spinAutoTn_E.SetMinMaxStep(100.0F, 250.0F, 5.0F);
    spinAutoTn_E.SetValue(autotune_temp_E);
    row2draw += row_h;

    //BED
    col = 122;
    row2draw = row_h;

    rect_B = rect_ui16(col, row2draw, 100, row_h);
    row2draw += row_h;

    window_create_ptr(WINDOW_CLS_LIST, id0,
        rect_ui16(col, row2draw, 100, row_h), &(list_RW_B));
    list_RW_B.SetItemCount(list_RW_strings_sz);
    list_RW_B.SetItemIndex(0);
    list_RW_B.SetCallback(window_list_RW_item);
    list_RW_B.SetTag(TAG_RW_B);

    row2draw += 25;

    rectKp_B = rect_ui16(col, row2draw, 25, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKp_B, dot_coordsKp_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    rectKi_B = rect_ui16(col, row2draw, 30, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKi_B, dot_coordsKi_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    rectKd_B = rect_ui16(col, row2draw, 50, row_h);
    generate_spin_digits(screen, SPIN_DIGITS, SPIN_PRECISION,
        id0, spinKd_B, dot_coordsKd_B,
        col + 25, row2draw, row_h);
    row2draw += row_h;

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(col, row2draw, 68, row_h),
        &(btAutoTuneApply_B));
    btAutoTuneApply_B.SetText(_(btnAutoTuneOrApplystrings[0]));
    btAutoTuneApply_B.Enable();
    btAutoTuneApply_B.SetTag(TAG_AUTOTUNE_APPLY_B);

    window_create_ptr(WINDOW_CLS_SPIN,
        id0, rect_ui16(col + 70, row2draw, 40, row_h),
        &(spinAutoTn_B));
    spinAutoTn_B.SetFormat("%.0f");
    spinAutoTn_B.SetMinMaxStep(50.0F, 110.0F, 5.0F);
    spinAutoTn_B.SetValue(autotune_temp_B);
    row2draw += row_h;

    //exit and footer

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 245, 60, 22), &(textExit));
    textExit.font = resource_font(IDR_FNT_BIG);
    textExit.SetText(_("EXIT"));
    textExit.Enable();
    textExit.SetTag(TAG_QUIT);

    dispPID((_PID_E), spinKp_E, spinKi_E, spinKd_E,
        SPIN_DIGITS, SPIN_PRECISION);
    dispPID((_PID_B), spinKp_B, spinKi_B, spinKd_B,
        SPIN_DIGITS, SPIN_PRECISION);
}

void screen_PID_done(screen_t *screen) {
    window_destroy(id);
}

void screen_PID_draw(screen_t *screen) {
}

int screen_PID_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    /* if (status_footer_event(&(footer), window, event, param)) {
        return 1;
    }*/

    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            if (autotune_state != AT_idle)
                return 0; //button should not be accessible
            screen_close();
            return 1;
        case TAG_AUTOTUNE_APPLY_E:
            if (list_RW_E_index_actual == 0) {
                //run autotune
                _autotune_E(screen, &(autotune_state));
            } else {
                //apply values
                _PID_set(&(_PID_E),
                    get_single_PIDparamFromDisp(spinKp_E,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(spinKi_E,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(spinKd_E,
                        SPIN_DIGITS, SPIN_PRECISION));
                eeprom_set_var(EEVAR_PID_NOZ_P, variant8_flt(Temperature::temp_hotend[0].pid.Kp));
                eeprom_set_var(EEVAR_PID_NOZ_I, variant8_flt(Temperature::temp_hotend[0].pid.Ki));
                eeprom_set_var(EEVAR_PID_NOZ_D, variant8_flt(Temperature::temp_hotend[0].pid.Kd));
            }
            break;
        case TAG_AUTOTUNE_APPLY_B:
            if (list_RW_B_index_actual == 0) {
                //run autotune
                _autotune_B(screen, &(autotune_state));
            } else {
                //apply values
                _PID_set(&(_PID_B),
                    get_single_PIDparamFromDisp(spinKp_B,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(spinKi_B,
                        SPIN_DIGITS, SPIN_PRECISION),
                    get_single_PIDparamFromDisp(spinKd_E,
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

        if (autotune_state != AT_idle) {
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) //wait for MARLIN_EVT_CommandEnd
            {
                btAutoTuneApply_B.SetTextColor(AUTO_TN_DEFAULT_CL);
                btAutoTuneApply_E.SetTextColor(AUTO_TN_DEFAULT_CL);
                textExit.SetTextColor(AUTO_TN_DEFAULT_CL);
                textExit.Enable();
                autotune_state = AT_idle;
            }
        }

        autotune_temp_E = spinAutoTn_E.GetValue();
        autotune_temp_B = spinAutoTn_B.GetValue();

        list_RW_E_index_actual = list_RW_E.GetItemIndex();
        if (list_RW_E_index_actual != list_RW_E_index_last) {
            if (list_RW_E_index_actual == 0) {
                disable_digits_write_mode(spinKp_E,
                    sizeof(spinKp_E) / sizeof(spinKp_E[0]));
                disable_digits_write_mode(spinKi_E,
                    sizeof(spinKi_E) / sizeof(spinKi_E[0]));
                disable_digits_write_mode(spinKd_E,
                    sizeof(spinKd_E) / sizeof(spinKd_E[0]));
                btAutoTuneApply_E.SetText(
                    _(btnAutoTuneOrApplystrings[0]));
            } else {
                enable_digits_write_mode(spinKp_E,
                    sizeof(spinKp_E) / sizeof(spinKp_E[0]));
                enable_digits_write_mode(spinKi_E,
                    sizeof(spinKi_E) / sizeof(spinKi_E[0]));
                enable_digits_write_mode(spinKd_E,
                    sizeof(spinKd_E) / sizeof(spinKd_E[0]));
                btAutoTuneApply_E.SetText(
                    _(btnAutoTuneOrApplystrings[1]));
            }

            list_RW_E_index_last = list_RW_E_index_actual;
        }

        list_RW_B_index_actual = list_RW_B.GetItemIndex();
        if (list_RW_B_index_actual != list_RW_B_index_last) {
            if (list_RW_B_index_actual == 0) {
                disable_digits_write_mode(spinKp_B,
                    sizeof(spinKp_B) / sizeof(spinKp_B[0]));
                disable_digits_write_mode(spinKi_B,
                    sizeof(spinKi_B) / sizeof(spinKi_B[0]));
                disable_digits_write_mode(spinKd_B,
                    sizeof(spinKd_B) / sizeof(spinKd_B[0]));
                btAutoTuneApply_B.SetText(
                    _(btnAutoTuneOrApplystrings[0]));
            } else {
                enable_digits_write_mode(spinKp_B,
                    sizeof(spinKp_B) / sizeof(spinKp_B[0]));
                enable_digits_write_mode(spinKi_B,
                    sizeof(spinKi_B) / sizeof(spinKi_B[0]));
                enable_digits_write_mode(spinKd_B,
                    sizeof(spinKd_B) / sizeof(spinKd_B[0]));
                btAutoTuneApply_B.SetText(
                    _(btnAutoTuneOrApplystrings[1]));
            }

            list_RW_B_index_last = list_RW_B_index_actual;
        }

        if (redraw) {
            redraw = 0;
            display::FillRect(rect_ui16(dot_coordsKp_E[0],
                                  dot_coordsKp_E[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(dot_coordsKi_E[0],
                                  dot_coordsKi_E[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(dot_coordsKd_E[0],
                                  dot_coordsKd_E[1], 2, 2),
                COLOR_WHITE);

            display::DrawText(rect_E, _("NOZZLE"), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            static const char kp[] = "Kp";
            display::DrawText(rectKp_E, string_view_utf8::MakeCPUFLASH((const uint8_t *)kp), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            static const char ki[] = "Ki";
            display::DrawText(rectKi_E, string_view_utf8::MakeCPUFLASH((const uint8_t *)ki), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            static const char kd[] = "Kd";
            display::DrawText(rectKd_E, string_view_utf8::MakeCPUFLASH((const uint8_t *)kd), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);

            display::FillRect(rect_ui16(dot_coordsKp_B[0],
                                  dot_coordsKp_B[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(dot_coordsKi_B[0],
                                  dot_coordsKi_B[1], 2, 2),
                COLOR_WHITE);
            display::FillRect(rect_ui16(dot_coordsKd_B[0],
                                  dot_coordsKd_B[1], 2, 2),
                COLOR_WHITE);

            display::DrawText(rect_B, _("BED"), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(rectKp_B, string_view_utf8::MakeCPUFLASH((const uint8_t *)kp), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(rectKi_B, string_view_utf8::MakeCPUFLASH((const uint8_t *)ki), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
            display::DrawText(rectKd_B, string_view_utf8::MakeCPUFLASH((const uint8_t *)kd), resource_font(IDR_FNT_NORMAL),
                COLOR_BLACK, COLOR_ORANGE);
        }

        if (_PID_actualize(&(_PID_E)) != 0) {
            dispPID((_PID_E), spinKp_E, spinKi_E, spinKd_E,
                SPIN_DIGITS, SPIN_PRECISION);
        }
        if (_PID_actualize(&(_PID_B)) != 0) {
            dispPID((_PID_B), spinKp_B, spinKi_B, spinKd_B,
                SPIN_DIGITS, SPIN_PRECISION);
        }
    }
    return 0;
}

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
    textExit.Disable();
    textExit.SetTextColor(AUTO_TN_ACTIVE_CL);
    return 1;
}

void _autotune_B(screen_t *screen, autotune_state_t *a_tn_st) {
    if (__autotune(screen, a_tn_st)) {
        _PID_autotune(&(_PID_B));
        btAutoTuneApply_B.SetTextColor(AUTO_TN_ACTIVE_CL);
        *a_tn_st = AT_extruder;
    }
}

void _autotune_E(screen_t *screen, autotune_state_t *a_tn_st) {
    if (__autotune(screen, a_tn_st)) {
        _PID_autotune(&(_PID_E));
        btAutoTuneApply_E.SetTextColor(AUTO_TN_ACTIVE_CL);
        *a_tn_st = AT_bed;
    }
}

//-----------------------------------------------------------------------------
//gui spins
void generate_spin_single_digit(int16_t &id0, window_spin_t &spin,
    uint16_t &col, uint16_t row, uint16_t offset, uint16_t row_h) {
    window_create_ptr(WINDOW_CLS_SPIN, id0,
        rect_ui16(col, row, offset, row_h), &spin);
    spin.PrintAsInt();
    spin.SetFormat("%d");
    spin.SetMinMaxStep(0.0F, 9.0F, 1.0F);
    spin.SetValue(0.0F);
    col += offset;
};

void enable_digits_write_mode(window_spin_t *spiners, size_t sz) {
    for (size_t i = 0; i < sz; ++i)
        spiners[i].Enable();
}

void disable_digits_write_mode(window_spin_t *spiners, size_t sz) {
    for (size_t i = 0; i < sz; ++i)
        spiners[i].Disable();
}

//i am not using size_t numOfDigits, size_t precision
//need to be signed because of for cycles
void generate_spin_digits(screen_t *screen, int numOfDigits, int precision,
    int16_t &id0, window_spin_t *pSpin, uint16_t dot_coords[2],
    uint16_t col, uint16_t row, uint16_t row_h) {
    int16_t offset = 12;
    dot_coords[0] = col + offset * (numOfDigits - precision) + 1;
    dot_coords[1] = row + 14;

    for (int i = numOfDigits - 1; i >= precision; --i) {
        generate_spin_single_digit(id0, pSpin[i], col, row, offset, row_h);
    }

    col += 3;

    for (int i = precision - 1; i >= 0; --i) {
        generate_spin_single_digit(id0, pSpin[i], col, row, offset, row_h);
    }

    disable_digits_write_mode(pSpin, numOfDigits);
}

//-----------------------------------------------------------------------------
//screen update methods
void disp_single_PID_digit(uint8_t singlePIDdigit, window_spin_t *pspin) {
    if (pspin->GetItemIndex() != singlePIDdigit) {
        pspin->SetItemIndex(singlePIDdigit);
    }
}

void disp_single_PID_param(uint32_t singlePIDparam,
    window_spin_t *spins, size_t numOfDigits) {
    for (size_t i = 0; i < numOfDigits; ++i) {
        disp_single_PID_digit(singlePIDparam % 10, &spins[i]);
        singlePIDparam = singlePIDparam / 10;
    }
}

void dispPID(_PID_t &PID, window_spin_t *Kps, window_spin_t *Kis, window_spin_t *Kds,
    size_t numOfDigits, size_t precision) {
    float multiplier = pow(10.0F, float(precision));
    uint32_t Kp = PID.Kp_last * multiplier;
    uint32_t Ki = PID.raw_Ki * multiplier; //scaled
    uint32_t Kd = PID.raw_Kd * multiplier; //scaled

    disp_single_PID_param(Kp, Kps, numOfDigits);
    disp_single_PID_param(Ki, Kis, numOfDigits);
    disp_single_PID_param(Kd, Kds, numOfDigits);
}

//i am not using size_t, need negative values in for cycle
float get_single_PIDparamFromDisp(window_spin_t *spins, int numOfDigits, int precision) {
    int val = 0;
    for (int i = numOfDigits - 1; i >= 0; --i) {
        val *= 10;
        val += spins[i].GetItemIndex();
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
#endif //#if 0
