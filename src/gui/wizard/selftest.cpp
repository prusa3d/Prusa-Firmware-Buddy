// selftest.cpp
#include "selftest.hpp"
#include "i18n.h"
#include "gui.hpp"
#include "selftest_MINI.h"
#include "marlin_client.h"
#include "DialogHandler.hpp"
#include "eeprom.h"
#include "screen_wizard.hpp"
#include "DialogSelftestResult.hpp"

WizardState_t StateFnc_SELFTEST_FAN() {
    marlin_test_start(stmFans);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestFans, 0);
    return WizardState_t::next;
}

WizardState_t StateFnc_SELFTEST_X() {
    marlin_test_start(stmXAxis);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestAxis, 0);
    return WizardState_t::next;
}

WizardState_t StateFnc_SELFTEST_Y() {
    marlin_test_start(stmYAxis);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestAxis, 0);
    return WizardState_t::next;
}

WizardState_t StateFnc_SELFTEST_Z() {
    marlin_test_start(stmZAxis);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestAxis, 0);
    return WizardState_t::next;
}

WizardState_t StateFnc_SELFTEST_XYZ() {
    marlin_test_start(stmXYZAxis);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestAxis, 0);
    return WizardState_t::next;
}

WizardState_t StateFnc_SELFTEST_RESULT() {
    static const char en_text_ok[] = N_("All tests finished successfully!");
    static const char en_text_nok[] = N_("The selftest failed to finish. Double-check the printer's wiring and axes. Then restart the Selftest.");
    static const char en_text_xyz[] = N_("Everything is alright. I will run XYZ calibration now. It will take approximately 12 minutes.");
    string_view_utf8 translated_ok = _(en_text_ok);
    string_view_utf8 translated_nok = _(en_text_nok);
    string_view_utf8 translated_xyz = _(en_text_xyz);

    SelftestResultEEprom_t result;
    result.ui32 = eeprom_get_ui32(EEVAR_SELFTEST_RESULT);

    if (result.printFan == SelftestResult_Passed && result.heatBreakFan == SelftestResult_Passed && result.xaxis == SelftestResult_Passed && result.yaxis == SelftestResult_Passed && result.zaxis == SelftestResult_Passed && result.nozzle == SelftestResult_Passed && result.bed == SelftestResult_Passed) {
        eeprom_set_bool(EEVAR_RUN_SELFTEST, false); // clear selftest flag
        MsgBoxPepa(IsStateInWizardMask(WizardState_t::XYZCALIB_first, ScreenWizard::GetMask()) ? translated_xyz : translated_ok, Responses_Next);
        return WizardState_t::next;
    } else {
        MsgBox(translated_nok, Responses_Next);
        DialogSelftestResult::Show(result);
        return WizardState_t::EXIT;
    }
}

WizardState_t StateFnc_SELFTEST_TEMP() {
    marlin_test_start(stmHeaters);
    DialogHandler::Access().WaitUntilClosed(ClientFSM::SelftestHeat, 0);
    return WizardState_t::next;
}
