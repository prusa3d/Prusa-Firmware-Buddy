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
#include "RAII.hpp"
#include "ScreenHandler.hpp"

#include "selftest.hpp"
#include "firstlay.hpp"
#include "xyzcalib.hpp"

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

string_view_utf8 WizardGetCaption(WizardState_t st) {
    if (IsStateInWizardMask(st, WizardMaskStart())) {
        return _("WIZARD");
    }

    if (IsStateInWizardMask(st, WizardMaskRange(WizardState_t::SELFTEST_first, WizardState_t::SELFTEST_last))) {
        return _("SELFTEST");
    }

    if (IsStateInWizardMask(st, WizardMaskXYZCalib())) {
        return _("XYZ CALIBRATION");
    }

    if (IsStateInWizardMask(st, WizardMaskFirstLay())) {
        return _("FIRST LAYER CALIBRATION");
    }

    if (st == WizardState_t::FINISH) {
        return _("WIZARD - OK");
    }

    return string_view_utf8::MakeNULLSTR(); //to avoid warning
}

ScreenWizard::StateArray ScreenWizard::states = StateInitializer();

uint64_t ScreenWizard::run_mask = WizardMaskAll();
WizardState_t ScreenWizard::start_state = WizardState_t::START_first;

bool ScreenWizard::is_config_invalid = true;

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
    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

//consumes loop, is it OK?
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

    bool changed = false;
    while (results[size_t(state)] == WizardTestState_t::DISABLED) { // check for disabled result == skip state
        state = WizardState_t(int(state) + 1);                      // skip disabled states
        changed = true;
    }
    if (changed) {
        header.SetText(WizardGetCaption(state)); // change caption
    }

    StateFnc stateFnc = states[size_t(state)];                                 // actual state function (action)
    StateFncData data = stateFnc(StateFncData(state, results[size_t(state)])); // perform state action

    results[size_t(state)] = data.GetResult(); // store result of actual state
    state = data.GetState();                   // update state
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
    static const char en_text[] = N_("The status bar is at "
                                     "the bottom of the "
                                     "screen. It contains "
                                     "information about:\n"
                                     "- Nozzle temp.\n"
                                     "- Heatbed temp.\n"
                                     "- Printing speed\n"
                                     "- Z-axis height\n"
                                     "- Selected filament");
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
    //todo rewrite .. template/macro ?
    ret[static_cast<size_t>(WizardState_t::START)] = StateFnc_START;
    ret[static_cast<size_t>(WizardState_t::INIT)] = StateFnc_INIT;
    ret[static_cast<size_t>(WizardState_t::INFO)] = StateFnc_INFO;
    ret[static_cast<size_t>(WizardState_t::FIRST)] = StateFnc_FIRST;

    ret[static_cast<size_t>(WizardState_t::SELFTEST_FAN)] = StateFnc_SELFTEST_FAN;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_X)] = StateFnc_SELFTEST_X;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_Y)] = StateFnc_SELFTEST_Y;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_Z)] = StateFnc_SELFTEST_Z;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_XYZ)] = StateFnc_SELFTEST_XYZ;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_TEMP)] = StateFnc_SELFTEST_TEMP;
    ret[static_cast<size_t>(WizardState_t::SELFTEST_RESULT)] = StateFnc_SELFTEST_RESULT;

    ret[static_cast<size_t>(WizardState_t::SELFTEST_AND_XYZCALIB)] = StateFnc_SELFTEST_AND_XYZCALIB;

    ret[static_cast<size_t>(WizardState_t::XYZCALIB_INIT)] = StateFnc_XYZCALIB_INIT;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_HOME)] = StateFnc_XYZCALIB_HOME;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_Z)] = StateFnc_XYZCALIB_Z;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MSG_CLEAN_NOZZLE)] = StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MSG_IS_SHEET)] = StateFnc_XYZCALIB_XY_MSG_IS_SHEET;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MSG_REMOVE_SHEET)] = StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MSG_PLACE_PAPER)] = StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_SEARCH)] = StateFnc_XYZCALIB_XY_SEARCH;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MSG_PLACE_SHEET)] = StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_XY_MEASURE)] = StateFnc_XYZCALIB_XY_MEASURE;
    ret[static_cast<size_t>(WizardState_t::XYZCALIB_RESULT)] = StateFnc_XYZCALIB_RESULT;

    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_FILAMENT_ASK)] = StateFnc_FIRSTLAY_FILAMENT_ASK;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_FILAMENT_ASK_PREHEAT)] = StateFnc_FIRSTLAY_FILAMENT_ASK_PREHEAT;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_FILAMENT_LOAD)] = StateFnc_FIRSTLAY_FILAMENT_LOAD;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_FILAMENT_UNLOAD)] = StateFnc_FIRSTLAY_FILAMENT_UNLOAD;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_MSBX_CALIB)] = StateFnc_FIRSTLAY_MSBX_CALIB;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_MSBX_USEVAL)] = StateFnc_FIRSTLAY_MSBX_USEVAL;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_MSBX_START_PRINT)] = StateFnc_FIRSTLAY_MSBX_START_PRINT;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_PRINT)] = StateFnc_FIRSTLAY_PRINT;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_MSBX_REPEAT_PRINT)] = StateFnc_FIRSTLAY_MSBX_REPEAT_PRINT;
    ret[static_cast<size_t>(WizardState_t::FIRSTLAY_RESULT)] = StateFnc_FIRSTLAY_RESULT;

    ret[static_cast<size_t>(WizardState_t::FINISH)] = StateFnc_FINISH;
    ret[static_cast<size_t>(WizardState_t::EXIT)] = StateFnc_EXIT;

    is_config_invalid = false;
    //check if all states are assigned
    for (size_t i = size_t(WizardState_t::START_first); i <= size_t(WizardState_t::last); ++i) {
        if (ret[i] == nullptr) {
            is_config_invalid = true;
        }
    }
    return ret;
}
