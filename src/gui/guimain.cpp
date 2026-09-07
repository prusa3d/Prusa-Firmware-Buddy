#include "DialogHandler.hpp"
#include "display.hpp"
#include "gui_time.hpp"
#include "gui.hpp"
#include "Jogwheel.hpp"
#include "knob_event.hpp"
#include "language_eeprom.hpp"
#include "marlin_client.hpp"
#include <screen_error.hpp>
#include "screen_home.hpp"
#include "screen_move_z.hpp"
#include "ScreenFactory.hpp"
#include "ScreenHandler.hpp"
#include "ScreenShot.hpp"
#include "sound.hpp"
#include "tasks.hpp"
#include <config_store/store_instance.hpp>
#include <crash_dump/dump.hpp>
#include <screen_splash.hpp>
#include <wdt.hpp>
#include <feature/factory_reset/factory_reset.hpp>

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_handler.hpp>
#endif

#include <option/has_leds.h>
#if HAS_LEDS()
    #include <leds/led_manager.hpp>
#endif

#include <gui/screen/initial/screen_crash_dump.hpp>
#include <gui/screen/initial/screen_print_readiness.hpp>
#include <gui/screen/initial/screen_initial_network_setup.hpp>
#include <gui/screen_printer_setup.hpp>
#include <gui/screen/initial/screen_welcome.hpp>
#include <gui/screen/screen_printer_type_changed.hpp>

#include <option/developer_mode.h>

#include <option/has_power_panic.h>
#if HAS_POWER_PANIC()
    #include "power_panic.hpp"
#endif

#include <option/has_emergency_stop.h>
#if HAS_EMERGENCY_STOP()
    #include <gui/screen/initial/screen_emergency_stop_consent.hpp>
#endif

#include <option/has_ht_hotend.h>
#if HAS_HT_HOTEND()
    #include <gui/screen/initial/screen_hotend_type_changed.hpp>
#endif

#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include <screen_menu_selftest_snake.hpp>
#endif

#include <option/has_heatbed_screws_during_transport.h>
#if HAS_HEATBED_SCREWS_DURING_TRANSPORT()
    #include <gui/screen/initial/screen_remove_heatbed_screws.hpp>
#endif

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <gui/screen/initial/screen_touch_driver_failed.hpp>
#endif

#include <option/has_translations.h>
#if HAS_TRANSLATIONS()
    #include <gui/screen_menu_languages.hpp>
#endif

Jogwheel jogwheel;

void gui_error_run(void) {
    gui_init();

    // This is not safe, because resource file could be corrupted
    // gui_error_run executes before bootstrap so resources may not be up to date resulting in artefects
    display::enable_resource_file();

    LangEEPROM::getInstance(); // Initialize language EEPROM value

    // Handle factory reset before setting up the error screen
    switch (crash_dump::message_get_type()) {

    case crash_dump::MsgType::FACTORY_RESET:
        FactoryReset::perform_internal();
        break;

    case crash_dump::MsgType::RSOD:
        // Don't show the "crash detected. Save to USB?" screen.
        // RSODs are "expected" errors. The dump can still be exported through the menu item if needed
        crash_dump::dump_set_exported();
        break;

    default:
        break;
    }

    screen_node screen_initializer { ScreenFactory::Screen<ScreenError> };
    Screens::Init(screen_initializer);

    // Mark everything as displayed
    crash_dump::message_set_displayed();
    crash_dump::dump_set_displayed();

#if HAS_LEDS()
    leds::LEDManager::instance().init();
#endif

    while (true) {
        gui::TickLoop();

#if HAS_LEDS()
        leds::LEDManager::instance().update();
#endif

        Screens::Access()->Loop();
        gui_bare_loop();
    }
}

namespace {

void init_screens() {
    Screens::Init(ScreenFactory::Screen<ScreenSplash>);

    Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<screen_home_data_t>);

#if HAS_POWER_PANIC()
    // don't present any screen or wizard if there is a powerpanic pending
    if (power_panic::state_stored()) {
        return;
    }
#endif

#if DEVELOPER_MODE()
    // #error dead code found by automatic analyses (see BFW-5461)
    // don't present any screen or wizard
    return;
#endif

    if (ScreenPrintReadiness::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenPrintReadiness>);
    }

    if (ScreenCrashDump::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenCrashDump>);
    }

#if HAS_EMERGENCY_STOP()
    if (ScreenEmergencyStopConsent::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenEmergencyStopConsent>);
    }
#endif

    bool should_show_welcome_screen = false;

#if HAS_SELFTEST()
    if (ScreenMenuSTSWizard::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenMenuSTSWizard>);
        should_show_welcome_screen = true;
    }
#endif

#if HAS_HEATBED_SCREWS_DURING_TRANSPORT()
    if (ScreenRemoveHeatbedScrews::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenRemoveHeatbedScrews>);
    }
#endif

    if (ScreenInitialNetworkSetup::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenInitialNetworkSetup>);
        should_show_welcome_screen = true;
    }

#if HAS_HT_HOTEND()
    if (ScreenHotendTypeChanged::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenHotendTypeChanged>);
    }
#endif

    if (ScreenPrinterSetup::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenPrinterSetup>);
        should_show_welcome_screen = true;
    }

    if (should_show_welcome_screen) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenWelcome>);
    }

    if (ScreenPrinterTypeChanged::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenPrinterTypeChanged>);
    }

#if HAS_TOUCH()
    if (ScreenTouchDriverFailed::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenTouchDriverFailed>);
    }
#endif

#if HAS_TRANSLATIONS()
    if (ScreenInitialLanguageSelection::should_show()) {
        Screens::Access()->PushBeforeCurrent(ScreenFactory::Screen<ScreenInitialLanguageSelection>);
    }
#endif
}

} // namespace

void gui_run(void) {
    gui_init();

    gui::knob::RegisterHeldLeftAction(TakeAScreenshot);
    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMoveZ>); });

    init_screens();

    // TIMEOUT variable getting value from EEPROM when EEPROM interface is initialized
    if (config_store().menu_timeout.get()) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }

    Screens::Access()->Loop();
#if HAS_LEDS()
    leds::LEDManager::instance().init();
#endif
    // Show bootstrap screen untill firmware initializes
    TaskDeps::provide(TaskDeps::Dependency::gui_display_ready);
    while (!TaskDeps::check(TaskDeps::Tasks::bootstrap_done)) {
        gui_bare_loop();
    }

    marlin_client::init();

    DialogHandler::Access(); // to create class NOW, not at first call of one of callback

    marlin_client::set_event_notify(marlin_server::EVENT_MSK_DEF);

    // Close bootstrap screen, open home screen
    Screens::Access()->Close();

    sound::play(SoundType::start);

#if HAS_SIDE_LEDS()
    leds::SideStripHandler::instance().activity_ping();
#endif

    TaskDeps::provide(TaskDeps::Dependency::gui_ready);

    // Do one initial screen loop to close the screen_splash_t and open the screen_home_t
    // Otherwise, some FSM dialogs might possibly open over the splash screen in  DialogHandler::Access().Loop();
    // and then be immediately closed.
    // BFW-6193
    Screens::Access()->Loop();

    // TODO make some kind of registration
    while (1) {
        gui::TickLoop();

        Screens::Access()->Loop();
        DialogHandler::Access().Loop();

        gui_loop();
    }
}
