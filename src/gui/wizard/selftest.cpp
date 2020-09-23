// selftest.cpp
#include "selftest_temp.hpp"
#include "i18n.h"
#include "gui.hpp"

StateFncData StateFnc_SELFTEST_INIT(StateFncData last_run) {
    static const char en_text[] = N_(
        "State\n"
        "SELFTEST_INIT\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_NEXT, 0, GuiDefaults::RectScreenBody, is_multiline::no);
    return last_run.PassToNext();
}

StateFncData StateFnc_SELFTEST_PASS(StateFncData last_run) {
    const char en_text[] = N_("All tests finished successfully!");
    string_view_utf8 translatedText = _(en_text);
    MsgBoxPepa(translatedText, Responses_NEXT);
    return last_run.PassToNext().PassToNext(); // 2x PassToNext() to skip fail
}

StateFncData StateFnc_SELFTEST_FAIL(StateFncData last_run) {
    static const char en_text[] = N_("The selftest failed to finish. Double-check the printer's wiring and axes. Then restart the Selftest.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_NEXT);
    return StateFncData(WizardState_t::EXIT, WizardTestState_t::PASSED);
}

StateFncData StateFnc_SELFTEST_AND_XYZCALIB(StateFncData last_run) {
    static const char en_text[] = N_("Everything is alright. I will run XYZ calibration now. It will take approximately 12 minutes.");
    string_view_utf8 translatedText = _(en_text);
    MsgBoxPepa(translatedText, Responses_NEXT);
    return last_run.PassToNext();
}

#if 0

    #include "selftest.hpp"
    #include "stm32f4xx_hal.h"

    #ifndef _DEBUG
        #define LAST_SELFTEST_TIMEOUT (30 * 60) // [s]
    #else
        #define LAST_SELFTEST_TIMEOUT 30 // [s]
    #endif                               //_DEBUG

struct selftest_data_t {
    selftest_cool_data_t cool_data;
    selftest_temp_data_t temp_data;
    selftest_fans_axis_data_t fans_axis_data;
};

uint32_t last_selftest_result;
uint32_t last_selftest_time = 0;

// !!!!!
// !!!!! if it is possible, insert new flags at the beginning (i.e. before the first one)
// !!!!! all changes are best consulted with Content-Department
// !!!!!
static uint32_t get_and_store_selftest_result(int16_t id_body, selftest_data_t *p_data) {
    uint32_t mask = 0;
    mask = (mask << 1) | (p_data->fans_axis_data.state_fan0 != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_fan1 != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_x != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_y != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_z != _TEST_PASSED);
    mask = (mask << 1) | (p_data->cool_data.state_cool != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_temp_nozzle != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_temp_bed != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_preheat_bed != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_preheat_nozzle != _TEST_PASSED);
    last_selftest_result = mask;
    last_selftest_time = HAL_GetTick() / 1000; // ->[s]
    return mask;
}

int wizard_selftest_is_ok(int16_t id_body, selftest_data_t *p_data) {
    return (get_and_store_selftest_result(id_body, p_data) == 0);
}

#endif //0
