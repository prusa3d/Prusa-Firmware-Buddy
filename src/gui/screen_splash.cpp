//screen_splash.cpp
#include "screen_splash.hpp"
#include "ScreenHandler.hpp"

#include "config.h"
#include "version.h"
#include "wizard/wizard.h"
#include "eeprom.h"

#include "stm32f4xx_hal.h"
#include "screens.h"
#include "../lang/i18n.h"

#ifdef _EXTUI
    #include "marlin_client.h"
#endif

void screen_splash_data_t::timer(uint32_t mseconds) {
    float percent = mseconds / 3000.0 * 100;
    progress.SetValue((percent < 95) ? percent : 95);
}

screen_splash_data_t::screen_splash_data_t()
    : window_frame_t(&logo_prusa_mini)
    , logo_prusa_mini(this, nullptr, rect_ui16(0, 84, 240, 62), IDR_PNG_splash_logo_prusa_prn)
    , text_progress(this, &logo_prusa_mini, rect_ui16(10, 171, 220, 20))
    , progress(this, &text_progress, rect_ui16(10, 200, 220, 15), 15, COLOR_GRAY, COLOR_ORANGE)
    , text_version(this, &progress, rect_ui16(00, 295, 240, 22))
    , icon_logo_buddy(this, &text_version)
    , icon_logo_marlin(this, &icon_logo_buddy)
    , icon_debug(this, &icon_logo_marlin) {
    //window_create_ptr(WINDOW_CLS_ICON, id, rect_ui16(0, 84, 240, 62),
    //    &(logo_prusa_mini));
    //logo_prusa_mini.SetIdRes(IDR_PNG_splash_logo_prusa_prn);

    //window_create_ptr(WINDOW_CLS_TEXT, id, rect_ui16(10, 171, 220, 20),
    //    &(text_progress));
    text_progress.font = resource_font(IDR_FNT_NORMAL);
    text_progress.SetAlignment(ALIGN_CENTER_BOTTOM);
    static const char loading[] = N_("Loading ...");
    text_progress.SetText(_(loading));

    //window_create_ptr(WINDOW_CLS_PROGRESS, id, rect_ui16(10, 200, 220, 15),
    //    &(progress));
    //progress.color_back = COLOR_GRAY;
    //progress.color_progress = COLOR_ORANGE;
    progress.SetFont(resource_font(IDR_FNT_BIG));
    //progress.height_progress = 15;

    window_create_ptr(WINDOW_CLS_ICON, id, rect_ui16(80, 240, 80, 80),
        &(icon_logo_marlin));
    icon_logo_marlin.SetIdRes(IDR_PNG_splash_logo_marlin);

    //window_create_ptr(WINDOW_CLS_TEXT, id, rect_ui16(00, 295, 240, 22),
    //    &(text_version));
    text_version.SetAlignment(ALIGN_CENTER);
    snprintf(text_version_buffer, sizeof(text_version_buffer), "%s%s",
        project_version, project_version_suffix_short);
    // this MakeRAM is safe - text_version_buffer is globally allocated
    text_version.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_version_buffer));
}

void screen_splash_data_t::unconditionalDraw() {
    window_frame_t::unconditionalDraw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    display::DrawText(rect_ui16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
}

int screen_splash_data_t::event(window_t *sender, uint8_t event, void *param) {
    timer(HAL_GetTick());

#ifdef _EXTUI

    if (marlin_event(MARLIN_EVT_Startup)) {
        //screen_close();

        /*if (marlin_event(MARLIN_EVT_StartProcessing)) {
        // Originally these lines should be immediately after marlin_client_init, but because the functions are blocking
        // and we want the gui thread alive, we moved the lines here.
        marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF);
        marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF);
        screen_close();
        */

        uint8_t run_selftest = eeprom_get_var(EEVAR_RUN_SELFTEST).ui8;
        uint8_t run_xyzcalib = eeprom_get_var(EEVAR_RUN_XYZCALIB).ui8;
        uint8_t run_firstlay = eeprom_get_var(EEVAR_RUN_FIRSTLAY).ui8;
        uint8_t run_wizard = (run_selftest && run_xyzcalib && run_firstlay) ? 1 : 0;
        /*if ((run_wizard || run_firstlay)) {
            if (run_wizard) {
                screen_stack_push(get_scr_home()->id);
                wizard_run_complete();
            } else if (run_firstlay) {
                if (gui_msgbox(_("The printer is not calibrated. Start First Layer Calibration?"), MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING) == MSGBOX_RES_YES) {
                    screen_stack_push(get_scr_home()->id);
                    wizard_run_firstlay();
                } else
                    screen_open(get_scr_home()->id);
            }
        } else*/
        //Screens::Access()->Open(ScreenFactory::ScreenHome);
        Screens::Access()->Open(ScreenFactory::Screen<screen_home_data_t>);
#else
    if (HAL_GetTick() > 3000) {
        screen_close();
        screen_open(get_scr_test()->id);
#endif
        return 1;
    }
    return 0;
}
