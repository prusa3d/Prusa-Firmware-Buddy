// screen_wizard.cpp

#include "screen_wizard.hpp"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.hpp"
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
#endif //#if 0

void ScreenWizard::RunAll() {
    run_mask = WizardMaskAll();
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

void ScreenWizard::RunSelfTest() {
    run_mask = WizardMaskSelfTest();
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

void ScreenWizard::RunXYZCalib() {
    run_mask = WizardMaskXYZCalib();
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

void ScreenWizard::RunFirstLay() {
    run_mask = WizardMaskFirstLay();
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

void ScreenWizard::RunFirstLayerStandAlone() {
    run_mask = WizardMaskRange(WizardState_t::FIRSTLAY_MSBX_CALIB, WizardState_t::FIRSTLAY_last)
        | WizardMask(WizardState_t::EXIT);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

string_view_utf8 WizardGetCaption(WizardState_t st) {
    if (IsStateInWizardMask(st, WizardMaskStart())) {
        return _("WIZARD");
    }

    if (IsStateInWizardMask(st, WizardMaskSelfTest())) {
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
WizardState_t ScreenWizard::start_state = WizardState_t::START_first;

ScreenWizard::ResultArray ScreenWizard::ResultInitializer(uint64_t mask) {
    ResultArray ret;
    ret.fill(WizardTestState_t::DISABLED); //not needed, just to be safe;

    for (size_t i = size_t(WizardState_t::START_first); i <= size_t(WizardState_t::last); ++i) {
        ret[i] = InitState(WizardState_t(i), mask);
    }

    return ret;
}

ScreenWizard::ScreenWizard()
    : AddSuperWindow<window_frame_t>()
    , header(this, WizardGetCaption(WizardState_t::START_first))
    , footer(this)
    , results(ResultInitializer(run_mask))
    , state(start_state)
    , loopInProgress(false) {
    marlin_set_print_speed(100);
    start_state = WizardState_t::START_first;
    //marlin_set_exclusive_mode(1); //hope i will not need this
}

ScreenWizard::~ScreenWizard() {
    //if (!marlin_processing())
    //    marlin_start_processing(); //hope i will not need this
    //marlin_set_exclusive_mode(0); //hope i will not need this

    //turn heaters off
    //wizard_init(0, 0);
}

void ScreenWizard::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    if (event != GUI_event_t::LOOP) {
        SuperWindowEvent(sender, event, param);
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
    static const char en_text[] = N_("Welcome to the Original Prusa MINI setup wizard. Would you like to continue?");
    string_view_utf8 translatedText = _(en_text);
#ifdef _DEBUG
    const PhaseResponses &resp = Responses_IgnoreYesNo;
#else  //_DEBUG
    const PhaseResponses &resp = Responses_YesNo;
#endif //_DEBUG

    //IDR_PNG_icon_pepa
    switch (MsgBoxPepa(translatedText, resp)) {
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

//todo both is_multiline::no and is_multiline::yes does not work with \n
StateFncData StateFnc_INFO(StateFncData last_run) {
    static const char en_text[] = N_("The status bar is at\n"
                                     "the bottom of the  \n"
                                     "screen. It contains\n"
                                     "information about: \n"
                                     " - Nozzle temp.    \n"
                                     " - Heatbed temp.   \n"
                                     " - Printing speed  \n"
                                     " - Z-axis height   \n"
                                     " - Selected filament");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_FIRST(StateFncData last_run) {
    static const char en_text[] = N_("Press NEXT to run the Selftest, which checks for potential issues related to the assembly.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_FINISH(StateFncData last_run) {
    static const char en_text[] = N_("Calibration successful! Happy printing!");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
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

    ret[i++] = StateFnc_FIRSTLAY_FILAMENT_ASK;
    ret[i++] = StateFnc_FIRSTLAY_FILAMENT_ASK_PREHEAT;
    ret[i++] = StateFnc_FIRSTLAY_FILAMENT_LOAD;
    ret[i++] = StateFnc_FIRSTLAY_FILAMENT_UNLOAD;
    ret[i++] = StateFnc_FIRSTLAY_MSBX_CALIB;
    ret[i++] = StateFnc_FIRSTLAY_MSBX_USEVAL;
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
            //todo show bsod after display (spi) init
            static const char en_text[] = N_("Wizard states invalid");
            bsod(en_text);
        }
    }
#endif
    return ret;
}
