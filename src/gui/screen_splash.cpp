//screen_splash.cpp
#include "screen_splash.hpp"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"

#include "config.h"
#include "version.h"
#include "eeprom.h"
#include "png_resources.hpp"
#include "marlin_client.h"

#include "i18n.h"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "bsod.h"
#include "option/has_selftest.h"
#include "configuration_store/configuration_store.hpp"

#include <option/bootloader.h>

#ifdef _EXTUI
    #include "marlin_client.h"
#endif

#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
    #include "ScreenSelftest.hpp"
#endif // HAS_SELFTEST

screen_splash_data_t::screen_splash_data_t()
    : AddSuperWindow<screen_t>()
    , logo_prusa_mini(this, Rect16(0, 84, 240, 62), &png::prusa_mini_splash_207x47)
    , text_progress(this, Rect16(10, 171, 220, 29), is_multiline::no)
    , progress(this, Rect16(10, 200, 220, 15), 15, COLOR_ORANGE, COLOR_GRAY)
    , text_version(this, Rect16(0, 295, 240, 22), is_multiline::no)
    , icon_logo_buddy(this, Rect16(), nullptr)  //unused?
    , icon_logo_marlin(this, Rect16(), nullptr) //unused?
    , icon_debug(this, Rect16(80, 215, png::marlin_logo_79x61.w, png::marlin_logo_79x61.h), &png::marlin_logo_79x61) {
    super::ClrMenuTimeoutClose();

#if defined(USE_ST7789)
    text_progress.font = resource_font(IDR_FNT_NORMAL);
    text_progress.SetAlignment(Align_t::Center());

    progress.SetFont(resource_font(IDR_FNT_BIG));
    text_version.SetAlignment(Align_t::Center());
#endif // USE_ST7789

    snprintf(text_version_buffer, sizeof(text_version_buffer), "%s%s",
        project_version, project_version_suffix_short);
    // this MakeRAM is safe - text_version_buffer is globally allocated
    text_version.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_version_buffer));

#if HAS_SELFTEST()
    const bool run_selftest = config_store().run_selftest.get();
    const bool run_xyzcalib = config_store().run_xyz_calib.get();
    const bool run_firstlay = config_store().run_firstlay.get();
    const bool run_wizard = (run_selftest && run_xyzcalib && run_firstlay);
#endif
    const bool run_lang = !LangEEPROM::getInstance().IsValid();

    const screen_node screens[] {
        { run_lang ? GetScreenMenuLanguagesNoRet : nullptr }, // lang

#if HAS_SELFTEST()
        {
            run_wizard ? screen_node(ScreenFactory::Screen<ScreenSelftest>, stmWizard) : screen_node()
        } // wizard
#endif
    };
    Screens::Access()->PushBeforeCurrent(screens, screens + (sizeof(screens) / sizeof(screens[0])));
}

void screen_splash_data_t::draw() {
    super::draw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    #if defined(USE_ST7789)
    display::DrawText(Rect16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
    #endif // USE_ST7789
#endif     //_DEBUG
}

/**
 * @brief this callback must be called in GUI thread
 * also it must be called manually before main gui loop
 * no events can be fired during that period and gui_redraw() must be called manually
 *
 * @param percent value for progressbar
 * @param str string to show instead loading
 */
void screen_splash_data_t::bootstrap_cb(unsigned percent, std::optional<const char *> str) {
    GUIStartupProgress progr = { percent, str };
    event_conversion_union un;
    un.pGUIStartupProgress = &progr;
    Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);
}

void screen_splash_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
#ifdef _EXTUI
    if (event == GUI_event_t::GUI_STARTUP) { //without clear it could run multiple times before screen is closed
        if (!param)
            return;

        event_conversion_union un;
        un.pvoid = param;
        if (!un.pGUIStartupProgress)
            return;

        int percent = un.pGUIStartupProgress->percent_done;

        if (un.pGUIStartupProgress->bootstrap_description.has_value()) {
            strlcpy(text_progress_buffer, un.pGUIStartupProgress->bootstrap_description.value(), sizeof(text_progress_buffer));
            text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
            text_progress.Invalidate();
        }

    #if BOOTLOADER()
        // when running under bootloader, we take over the progress bar at 50 %
        percent = 50 + percent / 2;
    #endif

    #if defined(USE_ST7789)
        progress.SetValue(std::clamp(percent, 0, 99));
    #endif // USE_ST7789

        if (percent > 99) {
            Screens::Access()->Close();
        }
#else  // _EXTUI
    if (HAL_GetTick() > 3000) {
        Screens::Access()->Close();
#endif // _EXTUI
    }
}
