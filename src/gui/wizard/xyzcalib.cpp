// xyzcalib.cpp
#include "i18n.h"
#include "gui.hpp"
#include "xyzcalib.hpp"

StateFncData StateFnc_XYZCALIB_INIT(StateFncData last_run) {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_INIT\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_HOME(StateFncData last_run) {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_HOME\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_Z(StateFncData last_run) {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_Z\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE(StateFncData last_run) {
    static const char en_text[] = N_("Please clean the nozzle for calibration. Click NEXT when done.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MSG_IS_SHEET(StateFncData last_run) {
    static const char en_text[] = N_("Is steel sheet on heatbed?");
    string_view_utf8 translatedText = _(en_text);

    if (MsgBox(translatedText, Responses_YesNo) == Response::Yes) {
        return StateFncData(WizardState_t::XYZCALIB_XY_MSG_REMOVE_SHEET, WizardTestState_t::PASSED);
    } else {
        return StateFncData(WizardState_t::XYZCALIB_XY_MSG_PLACE_PAPER, WizardTestState_t::PASSED);
    }
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET(StateFncData last_run) {
    static const char en_text[] = N_("Please remove steel sheet from heatbed.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER(StateFncData last_run) {
    static const char en_text[] = N_("Place a sheet of paper under the nozzle during the calibration of first "
                                     "4 points. If the nozzle catches the paper, power off printer immediately!");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_SEARCH(StateFncData last_run) {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_XY_SEARCH\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET(StateFncData last_run) {
    static const char en_text[] = N_("Please place steel sheet on heatbed.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_XY_MEASURE(StateFncData last_run) {
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_PASS(StateFncData last_run) {
    return last_run.PassToNext();
}

StateFncData StateFnc_XYZCALIB_FAIL(StateFncData last_run) {
    return last_run.PassToNext();
}

#if 0

    #include "gui.hpp"
    #include "dbg.h"
    #include "config.h"
    #include "stm32f4xx_hal.h"
    #include "marlin_client.h"
    #include "wizard_config.hpp"
    #include "screen_wizard.hpp"
    #include "window_dlg_calib_z.hpp"

struct xyzcalib_screen_t {
    window_progress_t progress;
    window_text_t text_state;
    window_text_t text_search;
    window_icon_t icon;
    uint32_t timer0;
};

struct xyzcalib_data_t {
    _TEST_STATE_t state_home;
    _TEST_STATE_t state_z;
    _TEST_STATE_t state_xy;
    _TEST_STATE_t state_xy_search;
    _TEST_STATE_t state_xy_measure;
};

void wizard_init_screen_xyzcalib(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    /*
    //window_destroy_children(id_body);
    window_t *pWin = window_ptr(id_body);
    if (pWin != 0) {
        pWin->Invalidate();
        pWin->Show();
    }
    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, Rect16(x, y, WIZARD_X_SPACE, 22), &(p_screen->text_state));
    p_screen->text_state.SetText(_("Auto home"));

    y += 22;

    window_create_ptr(WINDOW_CLS_PROGRESS, id_body, Rect16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));

    y += 12;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, Rect16(x, y, WIZARD_X_SPACE, 66), &(p_screen->text_search));
    p_screen->text_search.SetText(string_view_utf8::MakeNULLSTR());

    y += 66;

    window_create_ptr(WINDOW_CLS_ICON, id_body, Rect16((240 - 100) / 2, y, 100, 100), &(p_screen->icon));
*/
}

int xyzcalib_home(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_home == _TEST_START) {
        wizard_init_screen_xyzcalib(id_body, p_screen, p_data);
        p_screen->icon.SetIdRes(IDR_PNG_wizard_icon_autohome);
        marlin_gcode("G28");
        marlin_event_clr(MARLIN_EVT_CommandEnd);
    } else if (marlin_event_clr(MARLIN_EVT_CommandEnd))
        p_data->state_home = _TEST_PASSED;
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_home), _WIZ_TIMER);
    p_screen->progress.SetValue(progress);
    return progress;
}

int xyzcalib_z(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_z == _TEST_START) {
        p_screen->text_state.SetText(_("Calibrating Z"));
        gui_dlg_calib_z();
        p_data->state_home = _TEST_PASSED;
    }
    return 100;
}

int xyzcalib_xy_search(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_xy_search == _TEST_START) {
        p_screen->text_search.SetText(_(
            "Searching bed\n"
            "calibration points"));
        p_screen->icon.SetIdRes(IDR_PNG_wizard_icon_search);
    }
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_xy_search), _WIZ_TIMER_AUTOPASS);
    p_screen->progress.SetValue(progress);
    return progress;
}

int xyzcalib_xy_measure(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    if (p_data->state_xy_measure == _TEST_START) {
        p_screen->text_search.SetText(_(
            "Measuring reference\n"
            "height of calib.\n"
            "points"));
        p_screen->icon.SetIdRes(IDR_PNG_wizard_icon_measure);
        marlin_gcode("G29");
        marlin_event_clr(MARLIN_EVT_CommandEnd);
    } else if (marlin_event_clr(MARLIN_EVT_CommandEnd))
        p_data->state_xy_measure = _TEST_PASSED;
    int progress = wizard_timer(&p_screen->timer0, 5000, &(p_data->state_xy_measure), _WIZ_TIMER);
    p_screen->progress.SetValue(progress);
    return progress;
}

int xyzcalib_is_ok(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data) {
    int ok = 1;
    ok &= (p_data->state_home == _TEST_PASSED);
    ok &= (p_data->state_z == _TEST_PASSED);
    ok &= (p_data->state_xy_search == _TEST_PASSED);
    ok &= (p_data->state_xy_measure == _TEST_PASSED);
    return ok;
}

#endif //0
