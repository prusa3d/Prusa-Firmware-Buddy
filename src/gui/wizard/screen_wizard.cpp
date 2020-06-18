// screen_wizard.c

#include "screen_wizard.h"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "selftest.h"
#include "wizard_ui.h"
#include "Marlin_PID_wrapper.h"
#include "filament.h"
#include "menu_vars.h"
#include "eeprom.h"
#include "filament_sensor.h"
#include "screens.h"
#include "../lang/i18n.h"

uint64_t wizard_mask = 0;

static int is_state_in_wizard_mask(wizard_state_t st) {
    return ((((uint64_t)1) << st) & wizard_mask) != 0;
}

static _TEST_STATE_t init_state(wizard_state_t st) {
    if (is_state_in_wizard_mask(st)) {
        return _TEST_START;
    } else {
        return _TEST_PASSED;
    }
}

void screen_wizard_init(screen_t *screen) {
    marlin_set_print_speed(100);
    pd->state = _STATE_START;

    int16_t id_frame = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    int16_t id_footer = window_create_ptr(WINDOW_CLS_FRAME, id_frame, gui_defaults.footer_sz, &(pd->frame_footer));
    window_hide(id_footer);

    int16_t id_body = window_create_ptr(WINDOW_CLS_FRAME, id_frame, gui_defaults.scr_body_sz, &(pd->frame_body));
    window_hide(id_body);

    int16_t id = window_create_ptr(WINDOW_CLS_TEXT, id_frame, rect_ui16(21, 0, 211, gui_defaults.header_sz.h), &(pd->header));
    window_set_alignment(id, ALIGN_LEFT_BOTTOM);

    window_set_text(id, wizard_get_caption(screen));

    status_footer_init(&(pd->footer), id_footer);

    pd->selftest.fans_axis_data.state_fan0 = init_state(_STATE_SELFTEST_FAN0);
    pd->selftest.fans_axis_data.state_fan1 = init_state(_STATE_SELFTEST_FAN1);
    pd->selftest.fans_axis_data.state_x = init_state(_STATE_SELFTEST_X);
    pd->selftest.fans_axis_data.state_y = init_state(_STATE_SELFTEST_Y);
    pd->selftest.fans_axis_data.state_z = init_state(_STATE_SELFTEST_Z);
    pd->selftest.cool_data.state_cool = init_state(_STATE_SELFTEST_COOL);
    pd->selftest.temp_data.state_preheat_nozzle = init_state(_STATE_SELFTEST_TEMP);
    pd->selftest.temp_data.state_preheat_bed = init_state(_STATE_SELFTEST_TEMP);
    pd->selftest.temp_data.state_temp_nozzle = init_state(_STATE_SELFTEST_TEMP);
    pd->selftest.temp_data.state_temp_bed = init_state(_STATE_SELFTEST_TEMP);
    pd->xyzcalib.state_home = init_state(_STATE_XYZCALIB_HOME);
    pd->xyzcalib.state_z = init_state(_STATE_XYZCALIB_Z);
    pd->xyzcalib.state_xy = _TEST_START; //init_state();
    pd->xyzcalib.state_xy_search = init_state(_STATE_XYZCALIB_XY_SEARCH);
    pd->xyzcalib.state_xy_measure = init_state(_STATE_XYZCALIB_XY_MEASURE);
    //pd->firstlay.state_heat                     = init_state();
    pd->firstlay.state_load = init_state(_STATE_FIRSTLAY_LOAD);
    pd->firstlay.state_print = init_state(_STATE_FIRSTLAY_PRINT);

    pd->flags = 0;

    //backup PID
    /*pd->Kp_bed = get_Kp_Bed();
	pd->Ki_bed = get_Ki_Bed();
	pd->Kd_bed = get_Kd_Bed();
	pd->Kp_noz = get_Kp_Noz();
	pd->Ki_noz = get_Ki_Noz();
	pd->Kd_noz = get_Kd_Noz();*/
}

void screen_wizard_done(screen_t *screen) {
    if (!marlin_processing())
        marlin_start_processing();
    /*
	//M301 - Set Hotend PID
	//M301 [C<value>] [D<value>] [E<index>] [I<value>] [L<value>] [P<value>]
	marlin_gcode_printf("M301 D%f I%f P%f", (double)(pd->Kd_noz), (double)(pd->Ki_noz), (double)(pd->Kp_noz));
	//M304 - Set Bed PID
	//M304 [D<value>] [I<value>] [P<value>]
	marlin_gcode_printf("M304 D%f I%f P%f", (double)(pd->Kd_bed), (double)(pd->Ki_bed), (double)(pd->Kp_bed));
*/

    //turn heaters off
    wizard_init(0, 0);
    window_destroy(pd->frame.win.id);
}

void screen_wizard_draw(screen_t *screen) {
}

int screen_wizard_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    static int inside_handler = 0;

    int16_t footer_id = pd->frame_footer.win.id;
    int16_t frame_id = pd->frame_body.win.id;
    selftest_fans_axis_screen_t *p_selftest_fans_axis_screen = &(pd->screen_variant.selftest_fans_axis_screen);
    selftest_cool_screen_t *p_selftest_cool_screen = &(pd->screen_variant.selftest_cool_screen);
    selftest_temp_screen_t *p_selftest_temp_screen = &(pd->screen_variant.selftest_temp_screen);
    selftest_data_t *p_selftest_data = &(pd->selftest);
    selftest_cool_data_t *p_selftest_cool_data = &(pd->selftest.cool_data);
    selftest_temp_data_t *p_selftest_temp_data = &(pd->selftest.temp_data);
    selftest_fans_axis_data_t *p_selftest_fans_axis_data = &(pd->selftest.fans_axis_data);
    firstlay_screen_t *p_firstlay_screen = &(pd->screen_variant.firstlay_screen);
    firstlay_data_t *p_firstlay_data = &(pd->firstlay);
    xyzcalib_screen_t *p_xyzcalib_screen = &(pd->screen_variant.xyzcalib_screen);
    xyzcalib_data_t *p_xyzcalib_data = &(pd->xyzcalib);

    if (pd->frame_footer.win.flg & WINDOW_FLG_VISIBLE) {
        status_footer_event(&(pd->footer), window, event, param);
    }

    //notify first layer calib (needed for baby steps)
    if (pd->state == _STATE_FIRSTLAY_PRINT) {
        if (event == WINDOW_EVENT_ENC_DN)
            wizard_firstlay_event_dn(p_firstlay_screen);

        if (event == WINDOW_EVENT_ENC_UP)
            wizard_firstlay_event_up(p_firstlay_screen);
    }

    if (event == WINDOW_EVENT_LOOP) {
        if (inside_handler == 0) {
            marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
            window_set_text(pd->header.win.id, wizard_get_caption(screen));
            inside_handler = 1;
            while (is_state_in_wizard_mask(pd->state) == 0)
                pd->state = wizard_state_t(int(pd->state) + 1); //skip disabled steps
            switch (pd->state) {
            case _STATE_START: {
#ifndef _DEBUG
                if (wizard_msgbox(
#else
                const char *btns[3] = { "SetDone", "YES", "NO" };
                switch (wizard_msgbox_btns(
#endif
                        "Welcome to the     \n"
                        "Original Prusa MINI\n"
                        "setup wizard.      \n"
                        "Would you like to  \n"
                        "continue?           ",
#ifndef _DEBUG
                        MSGBOX_BTN_YESNO, IDR_PNG_icon_pepa)
                    == MSGBOX_RES_YES) {
                    pd->state = _STATE_INIT;
                    window_show(footer_id);
                } else
                    screen_close();
#else
                    MSGBOX_BTN_CUSTOM3, IDR_PNG_icon_pepa, btns)) {
                case MSGBOX_RES_CUSTOM0:
                    eeprom_set_var(EEVAR_RUN_SELFTEST, variant8_ui8(0)); // clear selftest flag
                    eeprom_set_var(EEVAR_RUN_XYZCALIB, variant8_ui8(0)); // clear XYZ calib flag
                    eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(0)); // clear first layer flag
                    screen_close();
                    break;
                case MSGBOX_RES_CUSTOM1:
                    pd->state = _STATE_INIT;
                    window_show(footer_id);
                    break;
                case MSGBOX_RES_CUSTOM2:
                default:
                    screen_close();
                }
#endif
                break;
            }
            case _STATE_INIT:
                //PID of nozzle does not work with low temperatures well
                //have to preheat to lower temperature to avoid need of cooling
                pd->state = _STATE_INFO;
                window_show(footer_id);
                wizard_init(_START_TEMP_NOZ, _START_TEMP_BED);
                if (fs_get_state() == FS_DISABLED) {
                    fs_enable();
                    if (fs_wait_inicialized() == FS_NOT_CONNECTED)
                        fs_disable();
                }
                break;
            case _STATE_INFO:
                wizard_msgbox(
                    "The status bar is at\n"
                    "the bottom of the  \n"
                    "screen. It contains\n"
                    "information about: \n"
                    " - Nozzle temp.    \n"
                    " - Heatbed temp.   \n"
                    " - Printing speed  \n"
                    " - Z-axis height   \n"
                    " - Selected filament",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_FIRST;
                break;
            case _STATE_FIRST:
                wizard_msgbox(
                    "Press NEXT to run  \n"
                    "the Selftest, which\n"
                    "checks for         \n"
                    "potential issues   \n"
                    "related to         \n"
                    "the assembly.",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_SELFTEST_INIT;
                break;
            case _STATE_SELFTEST_INIT:
                pd->state = _STATE_SELFTEST_FAN0;
                //am i inicialized by screen before?
                if (!is_state_in_wizard_mask(_STATE_INIT)) {
                    window_show(footer_id);
                    wizard_init(_START_TEMP_NOZ, _START_TEMP_BED);
                }
                break;
            case _STATE_SELFTEST_FAN0:
                if (wizard_selftest_fan0(frame_id, p_selftest_fans_axis_screen, p_selftest_fans_axis_data) == 100)
                    pd->state = _STATE_SELFTEST_FAN1;
                break;
            case _STATE_SELFTEST_FAN1:
                if (wizard_selftest_fan1(frame_id, p_selftest_fans_axis_screen, p_selftest_fans_axis_data) == 100)
                    pd->state = _STATE_SELFTEST_X;
                break;
            case _STATE_SELFTEST_X:
                if (wizard_selftest_x(frame_id, p_selftest_fans_axis_screen, p_selftest_fans_axis_data) == 100)
                    pd->state = _STATE_SELFTEST_Y;
                break;
            case _STATE_SELFTEST_Y:
                if (wizard_selftest_y(frame_id, p_selftest_fans_axis_screen, p_selftest_fans_axis_data) == 100)
                    pd->state = _STATE_SELFTEST_Z;
                break;
            case _STATE_SELFTEST_Z:
                if (wizard_selftest_z(frame_id, p_selftest_fans_axis_screen, p_selftest_fans_axis_data) == 100)
                    pd->state = _STATE_SELFTEST_COOL;
                break;
            case _STATE_SELFTEST_COOL:
                if (wizard_selftest_cool(frame_id, p_selftest_cool_screen, p_selftest_cool_data) == 100)
                    pd->state = _STATE_SELFTEST_INIT_TEMP;
                break;
            case _STATE_SELFTEST_INIT_TEMP:
                //must start marlin
                pd->state = _STATE_SELFTEST_TEMP;
                window_show(footer_id);
                wizard_init_disable_PID(_START_TEMP_NOZ, _START_TEMP_BED);
                break;
            case _STATE_SELFTEST_TEMP:
                if (wizard_selftest_temp(frame_id, p_selftest_temp_screen, p_selftest_temp_data) == 100) {
                    pd->state = wizard_selftest_is_ok(frame_id, p_selftest_data)
                        ? _STATE_SELFTEST_PASS
                        : _STATE_SELFTEST_FAIL;
                    wizard_done_screen(screen);
                }
                break;
            case _STATE_SELFTEST_PASS:
                //need to show different msg box if XYZ calib shall not run
                eeprom_set_var(EEVAR_RUN_SELFTEST, variant8_ui8(0)); // clear selftest flag
                if (is_state_in_wizard_mask(_STATE_XYZCALIB_INIT))   //run XYZ
                    wizard_msgbox(
                        "Everything is alright. "
                        "I will run XYZ "
                        "calibration now. It will "
                        "take approximately "
                        "12 minutes.",
                        MSGBOX_BTN_NEXT, IDR_PNG_icon_pepa);
                else // do not run XYZ
                    wizard_msgbox(
                        "All tests finished successfully!", MSGBOX_BTN_DONE, IDR_PNG_icon_pepa);
                pd->state = _STATE_XYZCALIB_INIT;
                break;
            case _STATE_SELFTEST_FAIL:
                wizard_msgbox(
                    "The selftest failed\n"
                    "to finish.         \n"
                    "Double-check the   \n"
                    "printer's wiring   \n"
                    "and axes.          \n"
                    "Then restart       \n"
                    "the Selftest.      ",
                    MSGBOX_BTN_DONE, 0);
                screen_close();
                break;
            case _STATE_XYZCALIB_INIT:
                pd->state = _STATE_XYZCALIB_HOME;
                window_show(footer_id);
                wizard_init(0, 0);
                break;
            case _STATE_XYZCALIB_HOME:
                if (xyzcalib_home(frame_id, p_xyzcalib_screen, p_xyzcalib_data) == 100)
                    pd->state = _STATE_XYZCALIB_Z;
                break;
            case _STATE_XYZCALIB_Z:
                if (xyzcalib_z(frame_id, p_xyzcalib_screen, p_xyzcalib_data) == 100)
                    pd->state = _STATE_XYZCALIB_XY_MSG_CLEAN_NOZZLE;
                break;
            case _STATE_XYZCALIB_XY_MSG_CLEAN_NOZZLE:
                window_set_text(pd->screen_variant.xyzcalib_screen.text_state.win.id, "Calibration XY");
                wizard_msgbox1(
                    "Please clean the nozzle "
                    "for calibration. Click "
                    "NEXT when done.",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_MSG_IS_SHEET;
                break;
            case _STATE_XYZCALIB_XY_MSG_IS_SHEET:
                if (wizard_msgbox1(
                        "Is steel sheet "
                        "on heatbed?",
                        MSGBOX_BTN_YESNO, 0)
                    == MSGBOX_RES_YES)
                    pd->state = _STATE_XYZCALIB_XY_MSG_REMOVE_SHEET;
                else
                    pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_PAPER;
                break;
            case _STATE_XYZCALIB_XY_MSG_REMOVE_SHEET:
                wizard_msgbox1(
                    "Please remove steel "
                    "sheet from heatbed.",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_PAPER;
                break;
            case _STATE_XYZCALIB_XY_MSG_PLACE_PAPER:
                wizard_msgbox1(
                    "Place a sheet of paper "
                    "under the nozzle during "
                    "the calibration of first "
                    "4 points. "
                    "If the nozzle "
                    "catches the paper, power "
                    "off printer immediately!",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_SEARCH;
                break;
            case _STATE_XYZCALIB_XY_SEARCH:
                if (xyzcalib_xy_search(frame_id, p_xyzcalib_screen, p_xyzcalib_data) == 100)
                    pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_SHEET;
                break;
            case _STATE_XYZCALIB_XY_MSG_PLACE_SHEET:
                wizard_msgbox1(
                    "Please place steel sheet "
                    "on heatbed.",
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_MEASURE;
                break;
            case _STATE_XYZCALIB_XY_MEASURE:
                if (xyzcalib_xy_measure(frame_id, p_xyzcalib_screen, p_xyzcalib_data) == 100) {
                    pd->state = xyzcalib_is_ok(frame_id, p_xyzcalib_screen, p_xyzcalib_data)
                        ? _STATE_XYZCALIB_PASS
                        : _STATE_XYZCALIB_FAIL;
                    wizard_done_screen(screen);
                }
                break;
            case _STATE_XYZCALIB_PASS:
                eeprom_set_var(EEVAR_RUN_XYZCALIB, variant8_ui8(0)); // clear XYZ calib flag
                wizard_msgbox(
                    "Congratulations! "
                    "XYZ calibration is ok. "
                    "XY axes are "
                    "perpendicular.",
                    MSGBOX_BTN_NEXT, IDR_PNG_icon_pepa);
                pd->state = _STATE_FIRSTLAY_INIT;
                break;
            case _STATE_XYZCALIB_FAIL:
                wizard_msgbox(
                    "The XYZ calibration failed to finish. "
                    "Double-check the printer's wiring and axes, then restart the XYZ calibration.",
                    MSGBOX_BTN_DONE, 0);
                screen_close();
                break;
            case _STATE_FIRSTLAY_INIT: {
                pd->state = _STATE_FIRSTLAY_LOAD;
                window_show(footer_id);
                FILAMENT_t filament = get_filament();
                if (filament == FILAMENT_NONE || fs_get_state() == FS_NO_FILAMENT)
                    filament = FILAMENT_PLA;
                wizard_init(filaments[filament].nozzle, filaments[filament].heatbed);
                p_firstlay_screen->load_unload_state = LD_UNLD_INIT;
            } break;
            case _STATE_FIRSTLAY_LOAD:
                p_firstlay_screen->load_unload_state = wizard_load_unload(p_firstlay_screen->load_unload_state);
                if (p_firstlay_screen->load_unload_state == LD_UNLD_DONE)
                    pd->state = _STATE_FIRSTLAY_MSBX_CALIB;
                break;
            case _STATE_FIRSTLAY_MSBX_CALIB: {
                wizard_msgbox(
                    "Now, let's calibrate\n"
                    "the distance       \n"
                    "between the tip    \n"
                    "of the nozzle and  \n"
                    "the print sheet.   ",
                    MSGBOX_BTN_NEXT, 0);

                //show dialog only when values are not equal
                float diff = vars->z_offset - z_offset_def;
                if ((diff <= -z_offset_step) || (diff >= z_offset_step)) {
                    char buff[255];
                    //cannot use \n
                    snprintf(buff, sizeof(buff) / sizeof(char),
                        "Do you want to use\n"
                        "the current value?\n"
                        "Current: %0.3f.   \n"
                        "Default: %0.3f.   \n"
                        "Click NO to use the default value (recommended)",
                        (double)vars->z_offset, (double)z_offset_def);

                    if (wizard_msgbox(buff, MSGBOX_BTN_YESNO, 0) == MSGBOX_RES_NO) {
                        marlin_set_z_offset(z_offset_def);
                        eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(z_offset_def));
                    }
                }

                pd->state = _STATE_FIRSTLAY_MSBX_START_PRINT;
            } break;
            case _STATE_FIRSTLAY_MSBX_START_PRINT:
                wizard_msgbox(
                    //					"Observe the pattern\n"
                    //					"and turn the knob \n"
                    //					"to adjust the     \n"
                    //					"nozzle height in  \n"
                    //					"real time.        \n"
                    //					"Extruded plastic  \n"
                    //					"must stick to     \n"
                    //					"the print surface."
                    "In the next step, \n"
                    "use the knob to   \n"
                    "adjust the nozzle \n"
                    "height.           \n"
                    "Check the pictures\n"
                    "in the handbook   \n"
                    "for reference."

                    ,
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_FIRSTLAY_PRINT;
                break;
            case _STATE_FIRSTLAY_PRINT:
                if (wizard_firstlay_print(frame_id, p_firstlay_screen, p_firstlay_data, vars->z_offset) == 100)
                    pd->state = p_firstlay_data->state_print == _TEST_PASSED ? _STATE_FIRSTLAY_MSBX_REPEAT_PRINT : _STATE_FIRSTLAY_FAIL;
                break;
            case _STATE_FIRSTLAY_MSBX_REPEAT_PRINT:
                if (wizard_msgbox(_(
                                      "Do you want to     \n"
                                      "repeat the last    \n"
                                      "step and readjust  \n"
                                      "the distance       \n"
                                      "between the nozzle \n"
                                      "and heatbed?"),
                        MSGBOX_BTN_YESNO | MSGBOX_DEF_BUTTON1, 0)
                    == MSGBOX_RES_NO) {
                    pd->state = _STATE_FINISH;
                    marlin_set_z_offset(p_firstlay_screen->Z_offset);
                    eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(p_firstlay_screen->Z_offset));
                    eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(0)); // clear first layer flag
                    wizard_done_screen(screen);
                } else {
                    wizard_msgbox(_("Clean steel sheet."), MSGBOX_BTN_NEXT, 0);

                    pd->state = _STATE_FIRSTLAY_PRINT;
                    pd->firstlay.state_print = _TEST_START;

                    float z_val_to_store = p_firstlay_screen->Z_offset;
                    //show dialog only when values are not equal
                    float diff = z_val_to_store - z_offset_def;
                    if ((diff <= -z_offset_step) || (diff >= z_offset_step)) {
                        char buff[255];
                        snprintf(buff, sizeof(buff) / sizeof(char),
                            "Do you want to use last set value? "
                            "Last:  %0.3f.   "
                            "Default: %0.3f.   "
                            "Click NO to use default value.",
                            (double)p_firstlay_screen->Z_offset, (double)z_offset_def);

                        if (wizard_msgbox(buff, MSGBOX_BTN_YESNO, 0) == MSGBOX_RES_NO) {
                            z_val_to_store = z_offset_def;
                        }
                    }
                    marlin_set_z_offset(z_val_to_store);
                    eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(z_val_to_store));
                }
                break;
            case _STATE_FIRSTLAY_FAIL:
                wizard_msgbox(
                    "The first layer calibration failed to finish. "
                    "Double-check the printer's wiring, nozzle and axes, then restart the calibration.",
                    MSGBOX_BTN_DONE, 0);
                screen_close();
                break;
            case _STATE_FINISH:
                wizard_msgbox(
                    "Calibration successful!\n"
                    "Happy printing!",
                    MSGBOX_BTN_DONE, IDR_PNG_icon_pepa);
                screen_close();
                break;
            default:
                screen_close();
                break;
            }
            inside_handler = 0;
        }
    } else {
    }
    return 0;
}

const char *wizard_get_caption(screen_t *screen) {
    switch (pd->state) {
    case _STATE_START:
    case _STATE_INIT:
    case _STATE_INFO:
    case _STATE_FIRST:
        return "WIZARD";
    case _STATE_SELFTEST_INIT:
    case _STATE_SELFTEST_FAN0:
    case _STATE_SELFTEST_FAN1:
    case _STATE_SELFTEST_X:
    case _STATE_SELFTEST_Y:
    case _STATE_SELFTEST_Z:
    case _STATE_SELFTEST_COOL:
    case _STATE_SELFTEST_INIT_TEMP:
    case _STATE_SELFTEST_TEMP:
    case _STATE_SELFTEST_PASS:
    case _STATE_SELFTEST_FAIL:
        return "SELFTEST";
    case _STATE_XYZCALIB_INIT:
    case _STATE_XYZCALIB_HOME:
    case _STATE_XYZCALIB_Z:
    case _STATE_XYZCALIB_XY_MSG_CLEAN_NOZZLE:
    case _STATE_XYZCALIB_XY_MSG_IS_SHEET:
    case _STATE_XYZCALIB_XY_MSG_REMOVE_SHEET:
    case _STATE_XYZCALIB_XY_MSG_PLACE_PAPER:
    case _STATE_XYZCALIB_XY_SEARCH:
    case _STATE_XYZCALIB_XY_MSG_PLACE_SHEET:
    case _STATE_XYZCALIB_XY_MEASURE:
    case _STATE_XYZCALIB_PASS:
    case _STATE_XYZCALIB_FAIL:
        return "XYZ CALIBRATION";
    case _STATE_FIRSTLAY_INIT:
    case _STATE_FIRSTLAY_LOAD:
    case _STATE_FIRSTLAY_MSBX_CALIB:
    case _STATE_FIRSTLAY_MSBX_START_PRINT:
    case _STATE_FIRSTLAY_PRINT:
    case _STATE_FIRSTLAY_MSBX_REPEAT_PRINT:
    case _STATE_FIRSTLAY_FAIL:
        return "FIRST LAYER CALIB.";
    case _STATE_FINISH:
        return "WIZARD - OK";
    case _STATE_LAST:
        return "";
    }
    return ""; //to avoid warning
}

void wizard_done_screen(screen_t *screen) {
    window_destroy_children(pd->frame_body.win.id);
    window_invalidate(pd->frame_body.win.id);
}

screen_t screen_wizard = {
    0,
    0,
    screen_wizard_init,
    screen_wizard_done,
    screen_wizard_draw,
    screen_wizard_event,
    sizeof(screen_wizard_data_t), //data_size
    0,                            //pdata
};

screen_t *const get_scr_wizard() { return &screen_wizard; }
