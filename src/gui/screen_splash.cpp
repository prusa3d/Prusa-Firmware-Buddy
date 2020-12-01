//screen_splash.cpp
#include "screen_splash.hpp"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"

#include "config.h"
#include "version.h"
#include "eeprom.h"

#include "stm32f4xx_hal.h"
#include "i18n.h"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "screen_wizard.hpp"
#include "bsod.h"

#ifdef _EXTUI
    #include "marlin_client.h"
#endif

void screen_splash_data_t::timer(uint32_t mseconds) {
    float percent = mseconds / 3000.0 * 100;
    progress.SetValue((percent < 95) ? percent : 95);
}

screen_splash_data_t::screen_splash_data_t()
    : AddSuperWindow<screen_t>()
    , logo_prusa_mini(this, Rect16(0, 84, 240, 62), IDR_PNG_prusa_printer_splash)
    , text_progress(this, Rect16(10, 171, 220, 20), is_multiline::no)
    , progress(this, Rect16(10, 200, 220, 15), 15, COLOR_ORANGE, COLOR_GRAY)
    , text_version(this, Rect16(0, 295, 240, 22), is_multiline::no)
    , icon_logo_buddy(this, Rect16(), 0)  //unused?
    , icon_logo_marlin(this, Rect16(), 0) //unused?
    , icon_debug(this, Rect16(80, 240, 80, 80), IDR_PNG_marlin_logo) {

    if (ScreenWizard::IsConfigInvalid()) {
        static const char en_text[] = "Wizard states invalid"; // intentionally not translated
        bsod(en_text);
    }

    text_progress.font = resource_font(IDR_FNT_NORMAL);
    text_progress.SetAlignment(ALIGN_CENTER_BOTTOM);
    static const char loading[] = N_("Loading ...");
    text_progress.SetText(_(loading));
    progress.SetFont(resource_font(IDR_FNT_BIG));
    text_version.SetAlignment(ALIGN_CENTER);
    snprintf(text_version_buffer, sizeof(text_version_buffer), "%s%s",
        project_version, project_version_suffix_short);
    // this MakeRAM is safe - text_version_buffer is globally allocated
    text_version.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_version_buffer));
}

void screen_splash_data_t::draw() {
    super::draw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    display::DrawText(Rect16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
}

void screen_splash_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    timer(HAL_GetTick());

#ifdef _EXTUI

    if (marlin_event_clr(MARLIN_EVT_Startup)) { //without clear it could run multiple times before screen is closed

        /*if (marlin_event(MARLIN_EVT_StartProcessing)) {
        // Originally these lines should be immediately after marlin_client_init, but because the functions are blocking
        // and we want the gui thread alive, we moved the lines here.
        marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF);
        marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF);
        Screens::Access()->Close();
        */

        const bool run_selftest = variant_get_ui8(eeprom_get_var(EEVAR_RUN_SELFTEST)) ? 1 : 0;
        const bool run_xyzcalib = variant_get_ui8(eeprom_get_var(EEVAR_RUN_XYZCALIB)) ? 1 : 0;
        const bool run_firstlay = variant_get_ui8(eeprom_get_var(EEVAR_RUN_FIRSTLAY)) ? 1 : 0;
        const bool run_wizard = (run_selftest && run_xyzcalib && run_firstlay);
        const bool run_lang = !LangEEPROM::getInstance().IsValid();

        const ScreenFactory::Creator screens[] {
            run_lang ? GetScreenMenuLanguagesNoRet : nullptr,          // lang
            run_wizard ? ScreenFactory::Screen<ScreenWizard> : nullptr // wizard
        };
        Screens::Access()->PushBeforeCurrent(screens, screens + (sizeof(screens) / sizeof(screens[0])));
        Screens::Access()->Close();
#else
    if (HAL_GetTick() > 3000) {
        Screens::Access()->Close();
#endif
    }
}
