// selftest_temp_cool.c

#include "selftest_cool.h"
#include "config.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "wizard_ui.h"
#include "guitypes.h" //font_meas_text
#include "wizard_progress_bar.h"

//-----------------------------------------------------------------------------
//function definitions
void _wizard_cool_actualize_temperatures(selftest_cool_data_t *p_data); //screen - temporary data

//-----------------------------------------------------------------------------
//function declarations
void wizard_init_screen_selftest_cool(int16_t id_body, selftest_cool_screen_t *p_screen, selftest_cool_data_t *p_data) {
    window_destroy_children(id_body);
    window_t *pWin = window_ptr(id_body);
    if (pWin != 0) {
        pWin->Show();
        pWin->Invalidate();
    }
    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_waiting_cd));
    p_screen->text_waiting_cd.SetText(_("Waiting for cooldown"));

    y += 22;

    window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));

    y += 22;

    window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(10, y, WIZARD_X_SPACE, 22), &(p_screen->curr_nozzle_temp));
    // a nasty hack - need to put the translated format string somewhere and keep it there past the lifetime of this screen

    // r=1 c=15
    static const char nozzleFmt2Translate[] = N_("Nozzle: %.1f_C");
    static char nozzleFmt[30]; // this will eat RAM unnecessarily for the lifetime of the FW
    // - but since the selftest is a subject to change in near future,
    // I'll keep it here as a temporary solution.

    // And because of idiotic gettext I must force the \177 character by a piece of code - OMG!
    // @@TODO this really needs some sane workaround :(
    _(nozzleFmt2Translate).copyToRAM(nozzleFmt, sizeof(nozzleFmt));
    nozzleFmt[strlen(nozzleFmt) - 2] = '\177';
    p_screen->curr_nozzle_temp.SetFormat(nozzleFmt);

    y += 22;

    window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(10, y, WIZARD_X_SPACE, 22), &(p_screen->curr_bed_temp));
    // r=1 c=15
    static const char bedFmt2Translate[] = N_("Bed: %.1f_C");
    static char bedFmt[30];
    _(bedFmt2Translate).copyToRAM(bedFmt, sizeof(bedFmt));
    bedFmt[strlen(bedFmt) - 2] = '\177';
    p_screen->curr_bed_temp.SetFormat(bedFmt);

    y += 22;

    window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(10, y, WIZARD_X_SPACE - 10, 22), &(p_screen->target_nozzle));
    // r=1 c=15
    static const char nozzleTgtFmt2Translate[] = N_("Noz. target: %.0f_C");
    static char nozzleTgtFmt[30];
    _(nozzleTgtFmt2Translate).copyToRAM(nozzleTgtFmt, sizeof(nozzleTgtFmt));
    nozzleTgtFmt[strlen(nozzleTgtFmt) - 2] = '\177';
    p_screen->target_nozzle.SetFormat(nozzleTgtFmt);
    p_screen->target_nozzle.SetValue(_CALIB_TEMP_NOZ);

    y += 22;

    window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(10, y, WIZARD_X_SPACE - 10, 22), &(p_screen->target_bed));
    // r=1 c=15
    static const char bedTgtFmt2Translate[] = N_("Bed. target: %.0f_C");
    static char bedTgtFmt[30];
    _(bedTgtFmt2Translate).copyToRAM(bedTgtFmt, sizeof(bedTgtFmt));
    bedTgtFmt[strlen(bedTgtFmt) - 2] = '\177';
    p_screen->target_bed.SetFormat(bedTgtFmt);
    p_screen->target_bed.SetValue(_CALIB_TEMP_BED);

    y += 35;

    window_create_ptr(WINDOW_CLS_ICON, id_body, rect_ui16(100, y, 40, 40), &(p_screen->icon_hourglass));
    p_screen->icon_hourglass.SetIdRes(IDR_PNG_wizard_icon_hourglass);
}

void _wizard_cool_actualize_temperatures(selftest_cool_data_t *p_data) {
    marlin_manage_heater();
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK_TEMP_CURR);
    float t_noz = vars->temp_nozzle;
    float t_bed = vars->temp_bed;

    if (t_noz < p_data->temp_noz)
        p_data->temp_noz = t_noz;
    if (t_bed < p_data->temp_bed)
        p_data->temp_bed = t_bed;
}

int wizard_selftest_cool(int16_t id_body, selftest_cool_screen_t *p_screen, selftest_cool_data_t *p_data) {
    if (p_data->state_cool == _TEST_START) {
        p_data->temp_noz = 1000;
        p_data->temp_bed = 1000;
        _wizard_cool_actualize_temperatures(p_data);
        if ((p_data->temp_bed < _CALIB_TEMP_BED) && (p_data->temp_noz < _CALIB_TEMP_NOZ)) {
            p_data->state_cool = _TEST_PASSED;
            return 100;
        }
        wizard_init_screen_selftest_cool(id_body, p_screen, p_data);

        p_data->start_nozzle_temp = 10 * (p_data->temp_noz - _CALIB_TEMP_NOZ);
        p_data->start_bed_temp = 10 * (p_data->temp_bed - _CALIB_TEMP_BED);
        //------------------------------------
        marlin_stop_processing();
        hwio_fan_set_pwm(0, 255);
        hwio_fan_set_pwm(1, 255);
        //------------------------------------
    } else
        _wizard_cool_actualize_temperatures(p_data);

    float diff_n = (p_data->temp_noz - _CALIB_TEMP_NOZ) < 0 ? 0 : (p_data->temp_noz - _CALIB_TEMP_NOZ);
    float diff_b = (p_data->temp_bed - _CALIB_TEMP_BED) < 0 ? 0 : (p_data->temp_bed - _CALIB_TEMP_BED);

    float nozzle_progress, bed_progress, nozzle_procentage, bed_procentage, lower_procentage;

    nozzle_progress = p_data->start_nozzle_temp - 10 * diff_n;
    nozzle_procentage = (100 / p_data->start_nozzle_temp) * nozzle_progress;

    bed_progress = p_data->start_bed_temp - 10 * diff_b;
    bed_procentage = (100 / p_data->start_bed_temp) * bed_progress;

    lower_procentage = nozzle_procentage < bed_procentage ? nozzle_procentage : bed_procentage;

    int time_progress = wizard_timer(&p_data->timer, _COOLDOWN_TIMEOUT, &(p_data->state_cool), _WIZ_TIMER_AUTOFAIL);

    float progress = lower_procentage >= time_progress ? lower_procentage : time_progress;

    if (diff_n < 0.1F && diff_b < 0.1F) {
        progress = 100;
        p_data->state_cool = _TEST_PASSED;
    }
    //-------------------------------------
    if (progress == 100) {
        hwio_fan_set_pwm(0, 0);
        hwio_fan_set_pwm(1, 0);
        marlin_start_processing();
        marlin_settings_load();
    }
    //-------------------------------------

    p_screen->curr_nozzle_temp.SetValue(p_data->temp_noz);
    p_screen->curr_bed_temp.SetValue(p_data->temp_bed);
    p_screen->progress.color_progress = lower_procentage >= time_progress ? COLOR_LIME : COLOR_ORANGE;
    p_screen->progress.SetValue(progress);
    return progress;
}
