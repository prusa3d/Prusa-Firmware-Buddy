// xyzcalib.c

#include "xyzcalib.h"
#include "gui.hpp"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "screen_wizard.h"
#include "wizard_ui.h"
#include "window_dlg_calib_z.hpp"

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
