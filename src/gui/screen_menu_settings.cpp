// screen_menu_settings.cpp

#include "config.h"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "ScreenHandler.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "knob_event.hpp"
#include "netdev.h"
#include "wui.h"
#include "SteelSheets.hpp"
#include "MItem_crash.hpp"
#include "Configuration_MINI_adv.h"
#ifdef BUDDY_ENABLE_CONNECT
    #include <connect/marlin_printer.hpp>
#endif
/*****************************************************************************/

class MI_LOAD_SETTINGS : public WI_LABEL_t {
    constexpr static const char *const label = N_("Load Settings from file");

public:
    MI_LOAD_SETTINGS()
        : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        // FIXME: Some error handling/reporting
        // TODO: Loading other things than just network
        if (netdev_load_ini_to_eeprom()) {
            notify_reconfigure();
        }

        // FIXME: Error handling
#ifdef BUDDY_ENABLE_CONNECT
        connect_client::MarlinPrinter::load_cfg_from_ini();
#endif
    }
};

#ifdef _DEBUG
using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_SERVICE, MI_HW_SETUP, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_NETWORK,
    MI_TIMEZONE,
    MI_LOAD_SETTINGS,
    #endif // BUDDY_ENABLE_ETHERNET
    #ifdef BUDDY_ENABLE_DFU_ENTRY
    MI_ENTER_DFU,
    #endif
    MI_USB_MSC_ENABLE,
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME,
    MI_DEVHASH_IN_QR, MI_LANGUAGE, MI_LANGUAGUE_USB, MI_LANGUAGUE_XFLASH, MI_LOAD_LANG, MI_SORT_FILES,
    MI_SOUND_TYPE, MI_XFLASH_RESET, MI_XFLASH_DELETE, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EEPROM, MI_EXPERIMENTAL_SETTINGS, MI_FACTORY_DEFAULTS>;
#else
using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FOOTER_SETTINGS, MI_HW_SETUP, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
    #ifdef BUDDY_ENABLE_DFU_ENTRY
    MI_ENTER_DFU,
    #endif
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_USB_MSC_ENABLE,
    MI_NETWORK,
    MI_TIMEZONE,
    MI_LOAD_SETTINGS,
    #endif // BUDDY_ENABLE_ETHERNET
    #if ENABLED(CRASH_RECOVERY)
    MI_CRASH_DETECTION,
    #endif // ENABLED(CRASH_RECOVERY)
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_DEVHASH_IN_QR,
    MI_LANGUAGE, MI_FACTORY_DEFAULTS>;
#endif

class ScreenMenuSettings : public Screen {
    gui::knob::screen_action_cb old_action;

public:
    constexpr static const char *label = N_("SETTINGS");
    ScreenMenuSettings();
    ~ScreenMenuSettings();
};

ScreenFactory::UniquePtr GetScreenMenuSettings() {
    return ScreenFactory::Screen<ScreenMenuSettings>();
}

ScreenMenuSettings::ScreenMenuSettings()
    : Screen(_(label))
    , old_action(gui::knob::GetLongPressScreenAction()) { // backup hold action

    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(GetScreenMenuExperimentalSettings); }); // new hold action
    EnableLongHoldScreenAction();
}

ScreenMenuSettings::~ScreenMenuSettings() {
    gui::knob::RegisterLongPressScreenAction(old_action); // restore hold action
}
