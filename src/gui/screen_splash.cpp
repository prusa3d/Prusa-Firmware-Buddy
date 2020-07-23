//screen_splash.c

#include "gui.hpp"
#include "config.h"
#include "version.h"
#include "wizard/wizard.h"
#include "eeprom.h"

#include "stm32f4xx_hal.h"
#include "screens.h"
#include "../lang/i18n.h"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"

#ifdef _EXTUI
    #include "marlin_client.h"
#endif

struct screen_splash_data_t {
    window_frame_t frame;
    window_icon_t logo_prusa_mini;
    window_text_t text_progress;
    window_progress_t progress;
    window_text_t text_version;
    char text_version_buffer[16];
    window_icon_t icon_logo_buddy;
    window_icon_t icon_logo_marlin;

    window_icon_t icon_debug;

    uint32_t last_timer;
    uint8_t logo_invalid;
};

#define _psd ((screen_splash_data_t *)screen->pdata)

void screen_splash_timer(screen_t *screen, uint32_t mseconds);

void screen_splash_init(screen_t *screen) {
    int16_t id0;

    id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0),
        &(_psd->frame));

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(0, 84, 240, 62),
        &(_psd->logo_prusa_mini));
    _psd->logo_prusa_mini.SetIdRes(IDR_PNG_splash_logo_prusa_prn);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(10, 171, 220, 20),
        &(_psd->text_progress));
    _psd->text_progress.font = resource_font(IDR_FNT_NORMAL);
    _psd->text_progress.SetAlignment(ALIGN_CENTER_BOTTOM);
    static const char loading[] = N_("Loading ...");
    _psd->text_progress.SetText(_(loading));

    window_create_ptr(WINDOW_CLS_PROGRESS, id0, rect_ui16(10, 200, 220, 15),
        &(_psd->progress));
    _psd->progress.color_back = COLOR_GRAY;
    _psd->progress.color_progress = COLOR_ORANGE;
    _psd->progress.font = resource_font(IDR_FNT_BIG);
    _psd->progress.height_progress = 15;

    window_create_ptr(WINDOW_CLS_ICON, id0, rect_ui16(80, 240, 80, 80),
        &(_psd->icon_logo_marlin));
    _psd->icon_logo_marlin.SetIdRes(IDR_PNG_splash_logo_marlin);

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(00, 295, 240, 22),
        &(_psd->text_version));
    _psd->text_version.SetAlignment(ALIGN_CENTER);
    snprintf(_psd->text_version_buffer, sizeof(_psd->text_version_buffer), "%s%s",
        project_version, project_version_suffix_short);
    // this MakeRAM is safe - text_version_buffer is globally allocated
    _psd->text_version.SetText(string_view_utf8::MakeRAM((const uint8_t *)_psd->text_version_buffer));

    _psd->logo_invalid = 0;
}

void screen_splash_done(screen_t *screen) {
    window_destroy(_psd->frame.id);
}

void screen_splash_draw(screen_t *screen) {
    if (_psd->logo_prusa_mini.f_invalid)
        _psd->logo_invalid = 1;
}

int screen_splash_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    screen_splash_timer(screen, HAL_GetTick());
    if ((event == WINDOW_EVENT_LOOP) && _psd->logo_invalid) {
#ifdef _DEBUG
        static const char dbg[] = "DEBUG";
        display::DrawText(rect_ui16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
        _psd->logo_invalid = 0;
    }
#ifdef _EXTUI
    if (marlin_event(MARLIN_EVT_StartProcessing)) {
        // Originally these lines should be immediately after marlin_client_init, but because the functions are blocking
        // and we want the gui thread alive, we moved the lines here.
        marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF);
        marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF);
        screen_close();
        uint8_t run_selftest = eeprom_get_var(EEVAR_RUN_SELFTEST).ui8;
        uint8_t run_xyzcalib = eeprom_get_var(EEVAR_RUN_XYZCALIB).ui8;
        uint8_t run_firstlay = eeprom_get_var(EEVAR_RUN_FIRSTLAY).ui8;
        uint8_t run_wizard = (run_selftest && run_xyzcalib && run_firstlay) ? 1 : 0;
        // uint8_t run_language = eeprom_get_var(EEVAR_LANGUAGE).ui16 == static_cast<uint16_t>(0xffff) ? 1 : 0;
        // if (run_language) {
        //     // screen_stack_push(get_scr_home()->id);
        //     screen_open(get_scr_menu_languages()->id);
        // }

        const bool lang_valid = LangEEPROM::getInstance().IsValid();

        if ((run_wizard || run_firstlay)) {
            if (run_wizard) {
                screen_stack_push(get_scr_home()->id);
                if (lang_valid) {
                    wizard_run_complete();
                } else {
                    wizard_stack_push_complete();
                    screen_open(get_scr_menu_languages_noret()->id);
                }
            } else if (run_firstlay) {
                if (gui_msgbox(_("The printer is not calibrated. Start First Layer Calibration?"), MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING) == MSGBOX_RES_YES) {
                    screen_stack_push(get_scr_home()->id);
                    if (lang_valid) {
                        wizard_run_firstlay();
                    } else {
                        wizard_stack_push_firstlay();
                        screen_open(get_scr_menu_languages_noret()->id);
                    }
                } else if (lang_valid) {
                    screen_open(get_scr_home()->id);
                } else {
                    screen_stack_push(get_scr_home()->id);
                    screen_open(get_scr_menu_languages_noret()->id);
                }
            }
        } else if (lang_valid) {
            screen_open(get_scr_home()->id);
        } else {
            screen_stack_push(get_scr_home()->id);
            screen_open(get_scr_menu_languages_noret()->id);
        }

#else
    if (HAL_GetTick() > 3000) {
        screen_close();
        screen_open(get_scr_test()->id);
#endif
        return 1;
    }
    return 0;
}

void screen_splash_timer(screen_t *screen, uint32_t mseconds) {
    float percent = mseconds / 3000.0 * 100;
    _psd->progress.SetValue((percent < 95) ? percent : 95);
}

screen_t screen_splash = {
    0,
    0,
    screen_splash_init,
    screen_splash_done,
    screen_splash_draw,
    screen_splash_event,
    sizeof(screen_splash_data_t), //data_size
    0,                            //pdata
};

screen_t *const get_scr_splash() { return &screen_splash; }
