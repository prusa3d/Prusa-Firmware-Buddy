// xyzcalib.cpp
#include "i18n.h"
#include "gui.hpp"
#include "xyzcalib.hpp"

WizardState_t StateFnc_XYZCALIB_INIT() {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_INIT\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_HOME() {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_HOME\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_Z() {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_Z\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE() {
    static const char en_text[] = N_("Please clean the nozzle for calibration. Click NEXT when done.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MSG_IS_SHEET() {
    static const char en_text[] = N_("Is steel sheet on heatbed?");
    string_view_utf8 translatedText = _(en_text);

    if (MsgBox(translatedText, Responses_YesNo) == Response::Yes) {
        return WizardState_t::XYZCALIB_XY_MSG_REMOVE_SHEET;
    } else {
        return WizardState_t::XYZCALIB_XY_MSG_PLACE_PAPER;
    }
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET() {
    static const char en_text[] = N_("Please remove steel sheet from heatbed.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER() {
    static const char en_text[] = N_("Place a sheet of paper under the nozzle during the calibration of first "
                                     "4 points. If the nozzle catches the paper, power off printer immediately!");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_SEARCH() {
    static const char en_text[] = ( // for now intentionally not translated
        "State\n"
        "XYZCALIB_XY_SEARCH\n"
        "not implemented");
    const string_view_utf8 notTranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)(en_text));

    MsgBox(notTranslatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET() {
    static const char en_text[] = N_("Please place steel sheet on heatbed.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_XY_MEASURE() {
    return WizardState_t::next;
}

WizardState_t StateFnc_XYZCALIB_RESULT() {
    return WizardState_t::next;
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
    uint16_t x = WizardDefaults::MarginLeft;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, Rect16(x, y, WizardDefaults::X_space, 22), &(p_screen->text_state));
    p_screen->text_state.SetText(_("Auto home"));

    y += 22;

    window_create_ptr(WINDOW_CLS_PROGRESS, id_body, Rect16(x, y, WizardDefaults::X_space, 8), &(p_screen->progress));

    y += 12;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, Rect16(x, y, WizardDefaults::X_space, 66), &(p_screen->text_search));
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

.........................................................................................

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
                        ? _STATE_XYZCALIB_RESULT
                        : _STATE_XYZCALIB_FAIL;
                    wizard_done_screen(screen);
                }
                break;
            case _STATE_XYZCALIB_RESULT:
                eeprom_set_bool(EEVAR_RUN_XYZCALIB, false); // clear XYZ calib flag
                wizard_msgbox(_(
                                  "Congratulations! "
                                  "XYZ calibration is ok. "
                                  "XY axes are "
                                  "perpendicular."),
                    MSGBOX_BTN_NEXT, IDR_PNG_pepa_64px);
                pd->state = _STATE_FIRSTLAY_INIT;
                break;
            case _STATE_XYZCALIB_FAIL:
                wizard_msgbox(_(
                                  "The XYZ calibration failed to finish. "
                                  "Double-check the printer's wiring and axes, then restart the XYZ calibration."),
                    MSGBOX_BTN_DONE, 0);
                Screens::Access()->Close();
                break;

#endif //0
