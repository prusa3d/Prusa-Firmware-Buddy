#include "screen_splash.hpp"
#include "ScreenHandler.hpp"

#include "config.h"
#include "config_features.h"
#include "version.h"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include <config_store/store_instance.hpp>

#include "i18n.h"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "screen_menu_languages.hpp"
#include "screen_touch_error.hpp"
#include "bsod.h"
#include <guiconfig/guiconfig.h>

#include <option/bootloader.h>
#include <option/developer_mode.h>
#include <option/has_translations.h>

#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
    #include "ScreenSelftest.hpp"
#endif // HAS_SELFTEST

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif // HAS_TOUCH

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include <option/has_selftest_snake.h>
#if HAS_SELFTEST_SNAKE()
    #include "screen_menu_selftest_snake.hpp"
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#if defined(USE_ST7789)
    #define SPLASHSCREEN_PROGRESSBAR_X 16
    #define SPLASHSCREEN_PROGRESSBAR_Y 148
    #define SPLASHSCREEN_PROGRESSBAR_W 206
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     165
#elif defined(USE_ILI9488)
    #define SPLASHSCREEN_PROGRESSBAR_X 100
    #define SPLASHSCREEN_PROGRESSBAR_Y 165
    #define SPLASHSCREEN_PROGRESSBAR_W 280
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     185
#endif

screen_splash_data_t::screen_splash_data_t()
    : AddSuperWindow<screen_t>()
#if defined(USE_ST7789)
    , img_printer("/internal/res/printer_logo.qoi") // dimensions are printer dependent
    , img_marlin("/internal/res/marlin_logo_79x61.qoi")
    , icon_logo_printer(this, &img_printer, point_i16_t(0, 84), window_icon_t::Center::x, GuiDefaults::ScreenWidth)
    , icon_logo_marlin(this, &img_marlin, point_i16_t(80, 225))
#endif // USE_ST7789
    , text_progress(this, Rect16(0, SPLASHSCREEN_VERSION_Y, GuiDefaults::ScreenWidth, 18), is_multiline::no)
    , progress(this, Rect16(SPLASHSCREEN_PROGRESSBAR_X, SPLASHSCREEN_PROGRESSBAR_Y, SPLASHSCREEN_PROGRESSBAR_W, SPLASHSCREEN_PROGRESSBAR_H), COLOR_ORANGE, COLOR_GRAY, 6)
    , version_displayed(false) {
    super::ClrMenuTimeoutClose();

    text_progress.set_font(Font::small);
    text_progress.SetAlignment(Align_t::Center());
    text_progress.SetTextColor(COLOR_GRAY);

    snprintf(text_progress_buffer, sizeof(text_progress_buffer), "Firmware %s", project_version_full);
    text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
    progress.SetProgressPercent(0);
#if HAS_SELFTEST()
    #if DEVELOPER_MODE()
    const bool run_wizard = false;
    #elif !HAS_SELFTEST_SNAKE()
        #if PRINTER_IS_PRUSA_MK4
    const bool run_selftest = !SelftestResult_Passed_Mandatory(config_store().selftest_result.get());
        #else
    const bool run_selftest = config_store().run_selftest.get();
        #endif
    const bool run_xyzcalib = config_store().run_xyz_calib.get();
    const bool run_firstlay = config_store().run_first_layer.get();
    const bool run_wizard = (run_selftest && run_xyzcalib && run_firstlay);
    #else
    const bool run_wizard {
        []() {
            SelftestResult sr = config_store().selftest_result.get();

            auto any_passed = [](std::same_as<TestResult> auto... results) -> bool {
                static_assert(sizeof...(results) > 0, "Pass at least one result");

                return ((results == TestResult_Passed) || ...);
            };

            if (any_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed
        #if PRINTER_IS_PRUSA_XL
                    ,
                    config_store().selftest_result_nozzle_diameter.get(), config_store().selftest_result_phase_stepping.get()

        #endif
                        )) {
                return false;
            }
            for (size_t e = 0; e < config_store_ns::max_tool_count; e++) {
        #if HAS_TOOLCHANGER()
                if (!prusa_toolchanger.is_tool_enabled(e)) {
                    continue;
                }
        #endif
                if (any_passed(sr.tools[e].printFan, sr.tools[e].heatBreakFan,
        #if not PRINTER_IS_PRUSA_MINI
                        sr.tools[e].fansSwitched,
        #endif
                        sr.tools[e].nozzle, sr.tools[e].fsensor, sr.tools[e].loadcell, sr.tools[e].dockoffset, sr.tools[e].tooloffset)) {
                    return false;
                }
            }

            return true;
        }()
    };
    #endif
#endif

#if HAS_TRANSLATIONS()
    const bool run_lang = !LangEEPROM::getInstance().IsValid();
#endif
    const screen_node screens[] {
#if HAS_TRANSLATIONS()
        { run_lang ? ScreenFactory::Screen<ScreenMenuLanguagesNoRet> : nullptr }, // lang
#endif
#if HAS_TOUCH()
            { touchscreen.is_enabled() && !touchscreen.is_hw_ok() ? ScreenFactory::Screen<ScreenTouchError> : nullptr }, // touch error will show after language
#endif // HAS_TOUCH

#if HAS_SELFTEST()
    #if HAS_SELFTEST_SNAKE()
            {
                run_wizard ? screen_node(ScreenFactory::Screen<ScreenMenuSTSWizard>) : screen_node()
            } // xl wizard
    #else
            {
                run_wizard ? screen_node(ScreenFactory::Screen<ScreenSelftest>, stmWizard) : screen_node()
            } // wizard
    #endif
#else
        {
            screen_node()
        }
#endif
    };

#if ENABLED(POWER_PANIC)
    // present none of the screens above if there is a powerpanic pending
    if (!power_panic::state_stored()) {
#endif
        Screens::Access()->PushBeforeCurrent(screens, screens + (sizeof(screens) / sizeof(screens[0])));
#if ENABLED(POWER_PANIC)
    }
#endif
}

screen_splash_data_t::~screen_splash_data_t() {
    img::enable_resource_file(); // now it is safe to use resources from xFlash
}

void screen_splash_data_t::draw() {
    Validate();
    progress.Invalidate();
    text_progress.Invalidate();
    super::draw(); // We want to draw over bootloader's screen without flickering/redrawing
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    #if defined(USE_ST7789)
    display::DrawText(Rect16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(Font::small), COLOR_BLACK, COLOR_RED);
    #endif // USE_ST7789
    #if defined(USE_ILI9488)
    display::DrawText(Rect16(340, 130, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(Font::small), COLOR_BLACK, COLOR_RED);
    #endif // USE_ILI9488
#endif //_DEBUG
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

void screen_splash_data_t::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
#ifdef _EXTUI
    if (event == GUI_event_t::GUI_STARTUP) { // without clear it could run multiple times before screen is closed
        if (!param) {
            return;
        }

        event_conversion_union un;
        un.pvoid = param;
        if (!un.pGUIStartupProgress) {
            return;
        }
        int percent = un.pGUIStartupProgress->percent_done;

        // Bootstrap & FW version are displayed in the same space - we want to display what process is happening during bootstrap
        // If such a process description is not available (e.g: fw_gui_splash_progress()) - draw FW version (only once to avoid flickering)
        if (un.pGUIStartupProgress->bootstrap_description.has_value()) {
            strlcpy(text_progress_buffer, un.pGUIStartupProgress->bootstrap_description.value(), sizeof(text_progress_buffer));
            text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
            text_progress.Invalidate();
            version_displayed = false;
        } else {
            if (!version_displayed) {
                snprintf(text_progress_buffer, sizeof(text_progress_buffer), "Firmware %s", project_version_full);
                text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
                text_progress.Invalidate();
                version_displayed = true;
            }
        }

        progress.SetProgressPercent(std::clamp(percent, 0, 100));

#else // _EXTUI
    if (HAL_GetTick() > 3000) {
        Screens::Access()->Close();
#endif // _EXTUI
    }
}
