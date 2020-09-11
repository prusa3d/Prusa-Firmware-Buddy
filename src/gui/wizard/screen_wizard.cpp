// screen_wizard.cpp

#include "screen_wizard.hpp"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "filament.h"
#include "eeprom.h"
#include "filament_sensor.hpp"
#include "i18n.h"
#include "bsod.h"
#include "RAII.hpp"
#include "ScreenHandler.hpp"

#include "selftest.hpp"
#include "firstlay.hpp"
#include "xyzcalib.hpp"

#if 0

int screen_wizard_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    static int inside_handler = 0;

    int16_t frame_id = pd->frame_body.id;
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

    if (pd->frame_footer.IsVisible()) {
        //status_footer_event(&(pd->footer), window, event, param);
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
            pd->header.SetText(wizard_get_caption(screen));
            inside_handler = 1;
            while (is_state_in_wizard_mask(pd->state) == 0)
                pd->state = wizard_state_t(int(pd->state) + 1); //skip disabled steps
            switch (pd->state) {
            case _STATE_START: {
    #ifndef _DEBUG
                if (wizard_msgbox(
    #else
                const char *btns[3] = { "SetDone", "YES", "NO" }; // intentionally not translated, this is a debug code path
                switch (wizard_msgbox_btns(
    #endif
                        _("Welcome to the     \n"
                          "Original Prusa MINI\n"
                          "setup wizard.      \n"
                          "Would you like to  \n"
                          "continue?           "),
    #ifndef _DEBUG
                        MSGBOX_BTN_YESNO, IDR_PNG_icon_pepa)
                    == MSGBOX_RES_YES) {
                    pd->state = _STATE_INIT;
                    pd->frame_footer.Show();
                } else
                    Screens::Access()->Close();
    #else
                    MSGBOX_BTN_CUSTOM3, IDR_PNG_icon_pepa, btns)) {
                case MSGBOX_RES_CUSTOM0:
                    eeprom_set_var(EEVAR_RUN_SELFTEST, variant8_ui8(0)); // clear selftest flag
                    eeprom_set_var(EEVAR_RUN_XYZCALIB, variant8_ui8(0)); // clear XYZ calib flag
                    eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(0)); // clear first layer flag
                    Screens::Access()->Close();
                    break;
                case MSGBOX_RES_CUSTOM1:
                    pd->state = _STATE_INIT;
                    pd->frame_footer.Show();
                    break;
                case MSGBOX_RES_CUSTOM2:
                default:
                    Screens::Access()->Close();
                }
    #endif
                break;
            }
            case _STATE_INIT:
                //PID of nozzle does not work with low temperatures well
                //have to preheat to lower temperature to avoid need of cooling
                pd->state = _STATE_INFO;
                pd->frame_footer.Show();
                wizard_init(_START_TEMP_NOZ, _START_TEMP_BED);
                if (fs_get_state() == Disabled) {
                    fs_enable();
                    if (fs_wait_initialized() == fsensor_t::NotConnected)
                        fs_disable();
                }
                break;
            case _STATE_INFO:
                wizard_msgbox(_(
                                  "The status bar is at\n"
                                  "the bottom of the  \n"
                                  "screen. It contains\n"
                                  "information about: \n"
                                  " - Nozzle temp.    \n"
                                  " - Heatbed temp.   \n"
                                  " - Printing speed  \n"
                                  " - Z-axis height   \n"
                                  " - Selected filament"),
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_FIRST;
                break;
            case _STATE_FIRST:
                wizard_msgbox(_(
                                  "Press NEXT to run  \n"
                                  "the Selftest, which\n"
                                  "checks for         \n"
                                  "potential issues   \n"
                                  "related to         \n"
                                  "the assembly."),
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_SELFTEST_INIT;
                break;
            case _STATE_SELFTEST_INIT:
                pd->state = _STATE_SELFTEST_FAN0;
                //am i inicialized by screen before?
                if (!is_state_in_wizard_mask(_STATE_INIT)) {
                    pd->frame_footer.Show();
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
                pd->frame_footer.Show();
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
                    wizard_msgbox(_(
                                      "Everything is alright. "
                                      "I will run XYZ "
                                      "calibration now. It will "
                                      "take approximately "
                                      "12 minutes."),
                        MSGBOX_BTN_NEXT, IDR_PNG_icon_pepa);
                else // do not run XYZ
                    wizard_msgbox(_(
                                      "All tests finished successfully!"),
                        MSGBOX_BTN_DONE, IDR_PNG_icon_pepa);
                pd->state = _STATE_XYZCALIB_INIT;
                break;
            case _STATE_SELFTEST_FAIL:
                wizard_msgbox(_(
                                  "The selftest failed\n"
                                  "to finish.         \n"
                                  "Double-check the   \n"
                                  "printer's wiring   \n"
                                  "and axes.          \n"
                                  "Then restart       \n"
                                  "the Selftest.      "),
                    MSGBOX_BTN_DONE, 0);
                Screens::Access()->Close();
                break;
            case _STATE_XYZCALIB_INIT:
                pd->state = _STATE_XYZCALIB_HOME;
                pd->frame_footer.Show();
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
                pd->screen_variant.xyzcalib_screen.text_state.SetText(_("Calibration XY"));
                wizard_msgbox1(_(
                                   "Please clean the nozzle "
                                   "for calibration. Click "
                                   "NEXT when done."),
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_MSG_IS_SHEET;
                break;
            case _STATE_XYZCALIB_XY_MSG_IS_SHEET:
                if (wizard_msgbox1(_(
                                       "Is steel sheet "
                                       "on heatbed?"),
                        MSGBOX_BTN_YESNO, 0)
                    == MSGBOX_RES_YES)
                    pd->state = _STATE_XYZCALIB_XY_MSG_REMOVE_SHEET;
                else
                    pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_PAPER;
                break;
            case _STATE_XYZCALIB_XY_MSG_REMOVE_SHEET:
                wizard_msgbox1(_(
                                   "Please remove steel "
                                   "sheet from heatbed."),
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_PAPER;
                break;
            case _STATE_XYZCALIB_XY_MSG_PLACE_PAPER:
                wizard_msgbox1(_(
                                   "Place a sheet of paper "
                                   "under the nozzle during "
                                   "the calibration of first "
                                   "4 points. "
                                   "If the nozzle "
                                   "catches the paper, power "
                                   "off printer immediately!"),
                    MSGBOX_BTN_NEXT, 0);
                pd->state = _STATE_XYZCALIB_XY_SEARCH;
                break;
            case _STATE_XYZCALIB_XY_SEARCH:
                if (xyzcalib_xy_search(frame_id, p_xyzcalib_screen, p_xyzcalib_data) == 100)
                    pd->state = _STATE_XYZCALIB_XY_MSG_PLACE_SHEET;
                break;
            case _STATE_XYZCALIB_XY_MSG_PLACE_SHEET:
                wizard_msgbox1(_(
                                   "Please place steel sheet "
                                   "on heatbed."),
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
                wizard_msgbox(_(
                                  "Congratulations! "
                                  "XYZ calibration is ok. "
                                  "XY axes are "
                                  "perpendicular."),
                    MSGBOX_BTN_NEXT, IDR_PNG_icon_pepa);
                pd->state = _STATE_FIRSTLAY_INIT;
                break;
            case _STATE_XYZCALIB_FAIL:
                wizard_msgbox(_(
                                  "The XYZ calibration failed to finish. "
                                  "Double-check the printer's wiring and axes, then restart the XYZ calibration."),
                    MSGBOX_BTN_DONE, 0);
                Screens::Access()->Close();
                break;
            case _STATE_FIRSTLAY_INIT: {
                pd->state = _STATE_FIRSTLAY_LOAD;
                pd->frame_footer.Show();
                FILAMENT_t filament = get_filament();
                if (filament == FILAMENT_NONE || fs_get_state() == NoFilament)
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
                wizard_msgbox(_(
                                  "Now, let's calibrate\n"
                                  "the distance       \n"
                                  "between the tip    \n"
                                  "of the nozzle and  \n"
                                  "the print sheet.   "),
                    MSGBOX_BTN_NEXT, 0);

                //show dialog only when values are not equal
                float diff = vars->z_offset - z_offset_def;
                if ((diff <= -z_offset_step) || (diff >= z_offset_step)) {
                    char buff[20 * 7];
                    {
                        char fmt[20 * 7];
                        // c=20 r=6
                        static const char fmt2Translate[] = N_("Do you want to use\n"
                                                               "the current value?\n"
                                                               "Current: %0.3f.   \n"
                                                               "Default: %0.3f.   \n"
                                                               "Click NO to use the default value (recommended)");
                        _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
                        snprintf(buff, sizeof(buff) / sizeof(char), fmt, (double)vars->z_offset, (double)z_offset_def);
                    }
                    if (wizard_msgbox(string_view_utf8::MakeRAM((const uint8_t *)buff), MSGBOX_BTN_YESNO, 0) == MSGBOX_RES_NO) {
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
                    _("In the next step, \n"
                      "use the knob to   \n"
                      "adjust the nozzle \n"
                      "height.           \n"
                      "Check the pictures\n"
                      "in the handbook   \n"
                      "for reference.")

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
                        char buff[20 * 7];
                        {
                            char fmt[20 * 7];
                            // c=20 r=6
                            static const char fmt2Translate[] = N_("Do you want to use last set value? "
                                                                   "Last:  %0.3f.   "
                                                                   "Default: %0.3f.   "
                                                                   "Click NO to use default value.");
                            _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
                            snprintf(buff, sizeof(buff) / sizeof(char), fmt, (double)p_firstlay_screen->Z_offset, (double)z_offset_def);
                        }
                        if (wizard_msgbox(string_view_utf8::MakeRAM((const uint8_t *)buff), MSGBOX_BTN_YESNO, 0) == MSGBOX_RES_NO) {
                            z_val_to_store = z_offset_def;
                        }
                    }
                    marlin_set_z_offset(z_val_to_store);
                    eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(z_val_to_store));
                }
                break;
            case _STATE_FIRSTLAY_FAIL:
                wizard_msgbox(_(
                                  "The first layer calibration failed to finish. "
                                  "Double-check the printer's wiring, nozzle and axes, then restart the calibration."),
                    MSGBOX_BTN_DONE, 0);
                Screens::Access()->Close();
                break;
            case _STATE_FINISH:
                wizard_msgbox(_(
                                  "Calibration successful!\n"
                                  "Happy printing!"),
                    MSGBOX_BTN_DONE, IDR_PNG_icon_pepa);
                Screens::Access()->Close();
                break;
            default:
                Screens::Access()->Close();
                break;
            }
            inside_handler = 0;
        }
    } else {
    }
    return 0;
}

#endif //#if 0

string_view_utf8 WizardGetCaption(WizardState_t st) {
    if (IsStateInWizardMask(st, WizardMaskStart())) {
        return _("WIZARD");
    }

    if (IsStateInWizardMask(st, WizardMaskSelftest())) {
        return _("SELFTEST");
    }

    if (IsStateInWizardMask(st, WizardMaskXYZCalib())) {
        return _("XYZ CALIBRATION");
    }

    if (IsStateInWizardMask(st, WizardMaskFirstLay())) {
        return _("FIRST LAYER CALIB.");
    }

    if (st == WizardState_t::FINISH) {
        return _("WIZARD - OK");
    }

    return string_view_utf8::MakeNULLSTR(); //to avoid warning
}

ScreenWizard::StateArray ScreenWizard::states = StateInitializer();

uint64_t ScreenWizard::run_mask = WizardMaskAll();

ScreenWizard::ResultArray ScreenWizard::ResultInitializer(uint64_t mask) {
    ResultArray ret;
    ret.fill(WizardTestState_t::DISABLED); //not needed, just to be safe;

    for (size_t i = size_t(WizardState_t::START_first); i <= size_t(WizardState_t::last); ++i) {
        ret[i] = InitState(WizardState_t(i), mask);
    }

    return ret;
}

ScreenWizard::ScreenWizard()
    : window_frame_t()
    , header(this, WizardGetCaption(WizardState_t::START_first))
    , footer(this)
    , results(ResultInitializer(run_mask))
    , state(WizardState_t::START_first)
    , loopInProgress(false) {
    marlin_set_print_speed(100);

    //marlin_set_exclusive_mode(1); //hope i will not need this
}

ScreenWizard::~ScreenWizard() {
    //if (!marlin_processing())
    //    marlin_start_processing(); //hope i will not need this
    //marlin_set_exclusive_mode(0); //hope i will not need this

    //turn heaters off
    //wizard_init(0, 0);
}

void ScreenWizard::windowEvent(window_t *sender, uint8_t event, void *param) {

    if (event != WINDOW_EVENT_LOOP) {
        window_frame_t::windowEvent(sender, event, param);
        return;
    }

    //loop might be blocking
    if (loopInProgress)
        return;
    AutoRestore<bool> AR(loopInProgress);
    loopInProgress = true;

    StateFnc stateFnc = states[size_t(state)];                                 // actual state function (action)
    StateFncData data = stateFnc(StateFncData(state, results[size_t(state)])); // perform state action

    results[size_t(state)] = data.GetResult(); // store result of actual state
    if (state != data.GetState()) {
        state = data.GetState();                                      // change state
        while (results[size_t(state)] == WizardTestState_t::DISABLED) // check for disabled result == skip state
            state = WizardState_t(int(state) + 1);                    // skip disabled states
        header.SetText(WizardGetCaption(state));                      // change caption
    }
}

const PhaseResponses Responses_IgnoreYesNo = { Response::Ignore, Response::Yes, Response::No, Response::_none };

StateFncData StateFnc_START(StateFncData last_run) {
    string_view_utf8 title = _("Welcome to the     \n"
                               "Original Prusa MINI\n"
                               "setup wizard.      \n"
                               "Would you like to  \n"
                               "continue?           ");
#ifdef _DEBUG
    const PhaseResponses &resp = Responses_IgnoreYesNo;
#else  //_DEBUG
    const PhaseResponses &resp = Responses_YesNo;
#endif //_DEBUG

    //IDR_PNG_icon_pepa
    switch (MsgBoxPepa(title, resp)) {
#ifdef _DEBUG
    case Response::Ignore:
        eeprom_set_var(EEVAR_RUN_SELFTEST, variant8_ui8(0)); // clear selftest flag
        eeprom_set_var(EEVAR_RUN_XYZCALIB, variant8_ui8(0)); // clear XYZ calib flag
        eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(0)); // clear first layer flag
        return StateFncData(WizardState_t::EXIT, WizardTestState_t::PASSED);
#endif //_DEBUG
    case Response::Yes:
        return last_run.PassToNext();
    case Response::No:
    default:
        return StateFncData(WizardState_t::EXIT, WizardTestState_t::PASSED);
    }
}

StateFncData StateFnc_INIT(StateFncData last_run) {
    //wizard_init(_START_TEMP_NOZ, _START_TEMP_BED);
    if (fs_get_state() == fsensor_t::Disabled) {
        fs_enable();
        if (fs_wait_initialized() == fsensor_t::NotConnected)
            fs_disable();
    }
    return last_run.PassToNext();
}

StateFncData StateFnc_INFO(StateFncData last_run) {
    string_view_utf8 title = _("The status bar is at\n"
                               "the bottom of the  \n"
                               "screen. It contains\n"
                               "information about: \n"
                               " - Nozzle temp.    \n"
                               " - Heatbed temp.   \n"
                               " - Printing speed  \n"
                               " - Z-axis height   \n"
                               " - Selected filament");
    MsgBox(title, Responses_NEXT);
    return last_run.PassToNext();
}

StateFncData StateFnc_FIRST(StateFncData last_run) {
    string_view_utf8 title = _(
        "Press NEXT to run  \n"
        "the Selftest, which\n"
        "checks for         \n"
        "potential issues   \n"
        "related to         \n"
        "the assembly.");
    MsgBox(title, Responses_NEXT);
    return last_run.PassToNext();
}

StateFncData StateFnc_FINISH(StateFncData last_run) {
    static const string_view_utf8 title = _(
        "Calibration successful!\n"
        "Happy printing!");
    return last_run.PassToNext();
}

StateFncData StateFnc_EXIT(StateFncData last_run) {
    Screens::Access()->Close();
    return last_run;
}

ScreenWizard::StateArray ScreenWizard::StateInitializer() {
    StateArray ret = { { nullptr } };
    //todo rewrite .. template/macro or just enum values instead size_t i
    size_t i = 0;
    ret[i++] = StateFnc_START;
    ret[i++] = StateFnc_INIT;
    ret[i++] = StateFnc_INFO;
    ret[i++] = StateFnc_FIRST;

    ret[i++] = StateFnc_SELFTEST_INIT;
    ret[i++] = StateFnc_SELFTEST_FAN0;
    ret[i++] = StateFnc_SELFTEST_FAN1;
    ret[i++] = StateFnc_SELFTEST_X;
    ret[i++] = StateFnc_SELFTEST_Y;
    ret[i++] = StateFnc_SELFTEST_Z;
    ret[i++] = StateFnc_SELFTEST_COOL;
    ret[i++] = StateFnc_SELFTEST_INIT_TEMP;
    ret[i++] = StateFnc_SELFTEST_TEMP;
    ret[i++] = StateFnc_SELFTEST_PASS;
    ret[i++] = StateFnc_SELFTEST_FAIL;

    ret[i++] = StateFnc_SELFTEST_AND_XYZCALIB;

    ret[i++] = StateFnc_XYZCALIB_INIT;
    ret[i++] = StateFnc_XYZCALIB_HOME;
    ret[i++] = StateFnc_XYZCALIB_Z;
    ret[i++] = StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE;
    ret[i++] = StateFnc_XYZCALIB_XY_MSG_IS_SHEET;
    ret[i++] = StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET;
    ret[i++] = StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER;
    ret[i++] = StateFnc_XYZCALIB_XY_SEARCH;
    ret[i++] = StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET;
    ret[i++] = StateFnc_XYZCALIB_XY_MEASURE;
    ret[i++] = StateFnc_XYZCALIB_PASS;
    ret[i++] = StateFnc_XYZCALIB_FAIL;

    ret[i++] = StateFnc_FIRSTLAY_INIT;
    ret[i++] = StateFnc_FIRSTLAY_LOAD;
    ret[i++] = StateFnc_FIRSTLAY_MSBX_CALIB;
    ret[i++] = StateFnc_FIRSTLAY_MSBX_START_PRINT;
    ret[i++] = StateFnc_FIRSTLAY_PRINT;
    ret[i++] = StateFnc_FIRSTLAY_MSBX_REPEAT_PRINT;
    ret[i++] = StateFnc_FIRSTLAY_PASS;
    ret[i++] = StateFnc_FIRSTLAY_FAIL;

    ret[i++] = StateFnc_FINISH;
    ret[i++] = StateFnc_EXIT;

#ifdef _DEBUG
    //check if all states are assigned, hope it will be optimized out
    for (size_t i = size_t(WizardState_t::START_first); i <= size_t(WizardState_t::last); ++i) {
        if (ret[i] == nullptr) {
            //bsod will not work, but it will cause freeze
            bsod("Wizard states invalid");
        }
    }
#endif
    return ret;
}
