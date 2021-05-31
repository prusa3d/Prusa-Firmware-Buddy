// screen_wizard.cpp

#include "screen_wizard.hpp"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.hpp"
#include "filament.hpp"
#include "eeprom.h"
#include "filament_sensor.hpp"
#include "i18n.h"
#include "RAII.hpp"
#include "ScreenHandler.hpp"

#include "selftest.hpp"
#include "firstlay.hpp"
#include "xyzcalib.hpp"

void ScreenWizard::Run(wizard_run_type_t type) {
    run_mask = WizardMask(type);
    caption_type = type;
    Screens::Access()->Open(ScreenFactory::Screen<ScreenWizard>);
}

string_view_utf8 ScreenWizard::WizardGetCaption(WizardState_t st, wizard_run_type_t type) {
    static constexpr const char *en_wizard = N_("WIZARD");
    static constexpr const char *en_wizard_ok = N_("WIZARD - OK");
    static constexpr const char *en_selftest = N_("SELFTEST");
    static constexpr const char *en_xyz = N_("XYZ CALIBRATION");
    static constexpr const char *en_firstlay = N_("FIRST LAYER CALIBRATION");

    switch (type) {
    case wizard_run_type_t::firstlay:
        return _(en_firstlay);
    case wizard_run_type_t::selftest:
        return _(en_selftest);
    case wizard_run_type_t::xyz:
        return _(en_xyz);
    default:
        if (IsStateInWizardMask(st, WizardMaskStart())) {
            return _(en_wizard);
        }

        if (IsStateInWizardMask(st, WizardMaskRange(WizardState_t::SELFTEST_first, WizardState_t::SELFTEST_last))) {
            return _(en_selftest);
        }

        if (IsStateInWizardMask(st, WizardMaskXYZCalib())) {
            return _(en_xyz);
        }

        if (IsStateInWizardMask(st, WizardMaskFirstLay())) {
            return _(en_firstlay);
        }

        if (st == WizardState_t::EXIT) {
            return _(en_wizard_ok);
        }
    }

    return string_view_utf8::MakeNULLSTR(); //to avoid warning
}

ScreenWizard::StateArray ScreenWizard::states = StateInitializer();

uint64_t ScreenWizard::run_mask = WizardMaskAll();
WizardState_t ScreenWizard::start_state = WizardState_t::START_first;
wizard_run_type_t ScreenWizard::caption_type = wizard_run_type_t::all;
bool ScreenWizard::is_config_invalid = true;

ScreenWizard::ScreenWizard()
    : AddSuperWindow<screen_t>()
    , header(this, WizardGetCaption(WizardState_t::START_first, caption_type))
    , footer(this)
    , state(start_state)
    , loopInProgress(false) {
    marlin_set_print_speed(100);
    start_state = WizardState_t::START_first;
    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

//consumes loop, is it OK?
void ScreenWizard::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    static bool repaint_caption = false;

    if (event != GUI_event_t::LOOP) {
        SuperWindowEvent(sender, event, param);
        return;
    }

    //loop might be blocking
    if (loopInProgress)
        return;
    AutoRestore<bool> AR(loopInProgress);
    loopInProgress = true;

    while (!IsStateInWizardMask(state, run_mask) && state != WizardState_t::last) { // skip disabled states
        state = WizardState_t(int(state) + 1);                                      // skip disabled states
        repaint_caption = true;
    }
    if (repaint_caption) {
        header.SetText(WizardGetCaption(state, caption_type)); // change caption
    }

    StateFnc stateFnc = states[size_t(state)]; // actual state function (action)
    WizardState_t data = stateFnc();           // perform state action
    switch (data) {                            // update state
    case WizardState_t::repeat:
        //do nothing - leave state as it is
        break;
    case WizardState_t::next:
        state = (state != WizardState_t::last) ? WizardState_t(int(state) + 1) : WizardState_t::last;
        break;
    default:
        state = data;
        break;
    }

    repaint_caption = data != state;
}

const PhaseResponses Responses_IgnoreYesNo = { Response::Ignore, Response::Yes, Response::No, Response::_none };

WizardState_t StateFnc_START() {
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
        return WizardState_t::EXIT;
#endif //_DEBUG
    case Response::Yes:
        return WizardState_t::next;
    case Response::No:
    default:
        return WizardState_t::EXIT;
    }
}

WizardState_t StateFnc_INIT() {
    //wizard_init(_START_TEMP_NOZ, _START_TEMP_BED);
    if (FS_instance().Get() == fsensor_t::Disabled) {
        FS_instance().Enable();
        if (FS_instance().WaitInitialized() == fsensor_t::NotConnected)
            FS_instance().Disable();
    }

    //preheat for SELFTEST_TEMP, so selftest is quicker
    if (IsStateInWizardMask(WizardState_t::SELFTEST_TEMP, ScreenWizard::GetMask())) {
        marlin_gcode("M104 S35"); // Set nozzle temperature 35
        marlin_gcode("M140 S35"); // Set bed temperature 35
    }
    return WizardState_t::next;
}

//todo both is_multiline::no and is_multiline::yes does not work with \n
WizardState_t StateFnc_INFO() {
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
    return WizardState_t::next;
}

WizardState_t StateFnc_FIRST() {
    static const char en_text[] = N_("Press NEXT to run the Selftest, which checks for potential issues related to the assembly.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_FINISH() {
    static const char en_text[] = N_("Calibration successful! Happy printing!");
    string_view_utf8 translatedText = _(en_text);
    MsgBoxPepa(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_EXIT() {
    Screens::Access()->Close();
    return WizardState_t::repeat;
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
