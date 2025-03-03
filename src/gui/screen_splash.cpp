#include "screen_splash.hpp"
#include "ScreenHandler.hpp"

#include "config.h"
#include "config_features.h"
#include <version/version.hpp>
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include <config_store/store_instance.hpp>

#include "i18n.h"
#include "../lang/translator.hpp"
#include "language_eeprom.hpp"
#include "screen_menu_languages.hpp"
#include <pseudo_screen_callback.hpp>
#include "bsod.h"
#include <guiconfig/guiconfig.h>
#include <st25dv64k.h>
#include <sys.h>

#include <option/bootloader.h>
#include <option/developer_mode.h>
#include <option/has_translations.h>
#include <gui/screen_printer_setup.hpp>

#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include "printer_selftest.hpp"
    #include "screen_menu_selftest_snake.hpp"
#endif // HAS_SELFTEST

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif // HAS_TOUCH

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include "display.hpp"
#include <option/has_switched_fan_test.h>

#if HAS_MINI_DISPLAY()
    #define SPLASHSCREEN_PROGRESSBAR_X 16
    #define SPLASHSCREEN_PROGRESSBAR_Y 148
    #define SPLASHSCREEN_PROGRESSBAR_W 206
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     165

#elif HAS_LARGE_DISPLAY()
    #define SPLASHSCREEN_PROGRESSBAR_X 100
    #define SPLASHSCREEN_PROGRESSBAR_Y 165
    #define SPLASHSCREEN_PROGRESSBAR_W 280
    #define SPLASHSCREEN_PROGRESSBAR_H 12
    #define SPLASHSCREEN_VERSION_Y     185
#endif

screen_splash_data_t::screen_splash_data_t()
    : screen_t()
#if HAS_MINI_DISPLAY()
    , img_printer("/internal/res/printer_logo.qoi") // dimensions are printer dependent
    , img_marlin("/internal/res/marlin_logo_79x61.qoi")
    , icon_logo_printer(this, &img_printer, point_i16_t(0, 84), window_icon_t::Center::x, GuiDefaults::ScreenWidth)
    , icon_logo_marlin(this, &img_marlin, point_i16_t(80, 225))
#endif
    , text_progress(this, Rect16(0, SPLASHSCREEN_VERSION_Y, GuiDefaults::ScreenWidth, 18), is_multiline::no)
    , progress(this, Rect16(SPLASHSCREEN_PROGRESSBAR_X, SPLASHSCREEN_PROGRESSBAR_Y, SPLASHSCREEN_PROGRESSBAR_W, SPLASHSCREEN_PROGRESSBAR_H), COLOR_ORANGE, COLOR_GRAY, 6)
    , version_displayed(false) {
    ClrMenuTimeoutClose();

    text_progress.set_font(Font::small);
    text_progress.SetAlignment(Align_t::Center());
    text_progress.SetTextColor(COLOR_GRAY);

    snprintf(text_progress_buffer, sizeof(text_progress_buffer), "Firmware %s", version::project_version_full);
    text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
    progress.SetProgressPercent(0);

#if !HAS_SELFTEST()
    // Nothing

#elif DEVELOPER_MODE()
    const bool run_wizard = false;

#elif !PRINTER_IS_PRUSA_iX()
    const bool run_wizard =
        []() {
            SelftestResult sr = config_store().selftest_result.get();

            auto any_passed = [](std::same_as<TestResult> auto... results) -> bool {
                static_assert(sizeof...(results) > 0, "Pass at least one result");

                return ((results == TestResult_Passed) || ...);
            };

            if (any_passed(sr.xaxis, sr.yaxis, sr.zaxis, sr.bed
    #if PRINTER_IS_PRUSA_XL()
                    ,
                    config_store().selftest_result_phase_stepping.get()

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
    #if HAS_SWITCHED_FAN_TEST()
                        sr.tools[e].fansSwitched,
    #endif /* HAS_SWITCHED_FAN_TEST() */
                        sr.tools[e].nozzle, sr.tools[e].fsensor, sr.tools[e].loadcell, sr.tools[e].dockoffset, sr.tools[e].tooloffset)) {
                    return false;
                }
            }

            return true;
        }();
#else
    const bool run_wizard = false;
#endif

    constexpr auto pepa_callback = +[] {
        const char *txt =
#if PRINTER_IS_PRUSA_XL()
            N_("Hi, this is your\nOriginal Prusa XL printer.\n"
               "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MK4()
            // The MK4 is left out intentionally - it could be MK4, MK4S or MK3.9, we don't know yet
            N_("Hi, this is your\nOriginal Prusa printer.\n"
               "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MK3_5()
            N_("Hi, this is your\nOriginal Prusa MK3.5 printer.\n"
               "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MINI()
            N_("Hi, this is your\nOriginal Prusa MINI printer.\n"
               "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_iX()
            N_("Hi, this is your\nOriginal Prusa iX printer.\n"
               "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_COREONE()
            N_("Hi, this is your\nPrusa CORE One printer.\n"
               "I would like to guide you\nthrough the setup process.");
#else
    #error unknown config
#endif
        MsgBoxPepaCentered(_(txt), Responses_Ok);
    };

#if HAS_TOUCH()
    constexpr auto touch_error_callback = +[] {
        touchscreen.set_enabled(false);
        MsgBoxWarning(_("Touch driver failed to initialize, touch functionality disabled"), Responses_Ok);
    };
#endif

    constexpr auto network_callback = +[] {
        // Calls network_initial_setup_wizard
        marlin_client::gcode("M1703 A");
    };

    // Check for FW type change
    {
        auto &model_var = config_store().last_boot_base_printer_model;
        const auto model = model_var.get();
        const auto current_base_model = PrinterModelInfo::firmware_base().model;
        if (model == model_var.default_val) {
            // Not initialized - assume correct printer
            model_var.set(current_base_model);

        } else if (model != current_base_model) {
            constexpr auto callback = +[] {
                StringViewUtf8Parameters<16> params;
                MsgBoxError(
                    _("Printer type changed from %s to %s.\nFactory reset has to be performed.\n")
                        .formatted(params, PrinterModelInfo::get(config_store().last_boot_base_printer_model.get()).id_str, PrinterModelInfo::firmware_base().id_str),
                    { Response::Continue });

                auto msg = MsgBoxBase(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, _("Erasing everything,\nit will take some time..."));
                msg.Draw(); // Non-blocking info

                static constexpr uint32_t empty = 0xffffffff;
                for (uint16_t address = 0; address <= (8096 - 4); address += 4) {
                    st25dv64k_user_write_bytes(address, &empty, sizeof(empty));
                }

                MsgBoxInfo(_("Reset complete. The system will now restart."), Responses_Ok);
                sys_reset();
            };
            Screens::Access()->PushBeforeCurrent(screen_node(ScreenFactory::Screen<PseudoScreenCallback, callback>));
            return;
        }
    }

    const screen_node screens[] {
#if HAS_TRANSLATIONS()
        { !LangEEPROM::getInstance().IsValid() ? ScreenFactory::Screen<ScreenMenuLanguages, ScreenMenuLanguages::Context::initial_language_selection> : nullptr },
#endif
#if HAS_TOUCH()
            { touchscreen.is_enabled() && !touchscreen.is_hw_ok() ? ScreenFactory::Screen<PseudoScreenCallback, touch_error_callback> : nullptr },
#endif // HAS_TOUCH

            { !config_store().printer_setup_done.get() ? ScreenFactory::Screen<PseudoScreenCallback, pepa_callback> : nullptr },
            { !config_store().printer_setup_done.get() ? ScreenFactory::Screen<ScreenPrinterSetup> : nullptr },
            { !config_store().printer_setup_done.get() ? ScreenFactory::Screen<PseudoScreenCallback, network_callback> : nullptr },

#if HAS_SELFTEST()
        {
            run_wizard ? ScreenFactory::Screen<ScreenMenuSTSWizard> : nullptr
        }
#endif
    };

#if ENABLED(POWER_PANIC)
    // present none of the screens above if there is a powerpanic pending
    if (!power_panic::state_stored()) {
#endif
        Screens::Access()->PushBeforeCurrent(screens, screens + std::size(screens));
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
    screen_t::draw(); // We want to draw over bootloader's screen without flickering/redrawing
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    #if HAS_MINI_DISPLAY()
    display::draw_text(Rect16(180, 91, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(Font::small), COLOR_BLACK, COLOR_RED);
    #endif
    #if HAS_LARGE_DISPLAY()
    display::draw_text(Rect16(340, 130, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(Font::small), COLOR_BLACK, COLOR_RED);
    #endif
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

void screen_splash_data_t::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
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
                snprintf(text_progress_buffer, sizeof(text_progress_buffer), "Firmware %s", version::project_version_full);
                text_progress.SetText(string_view_utf8::MakeRAM((uint8_t *)text_progress_buffer));
                text_progress.Invalidate();
                version_displayed = true;
            }
        }

        progress.SetProgressPercent(std::clamp(percent, 0, 100));
    }
}
