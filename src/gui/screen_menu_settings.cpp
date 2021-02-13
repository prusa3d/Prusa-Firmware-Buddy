// screen_menu_settings.cpp

#include "screen_menus.hpp"
#include "gui.hpp"
#include "config.h"
#include "app.h"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "cmsis_os.h"
#include "sys.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "filament_sensor.hpp"
#include "dump.h"
#include "sound.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "i18n.h"
#include "Marlin/src/core/serial.h"

/*****************************************************************************/
//MI_FILAMENT_SENSOR
class MI_FILAMENT_SENSOR : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Filament Sensor");
    static bool fs_not_connected;
    static bool consumeNotConnected();
    void no_sensor_msg() const;

    bool init_index() const;

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_OFF_ON_t(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    void CheckDisconnected();

protected:
    virtual void OnChange(size_t old_index) override;
};

void MI_FILAMENT_SENSOR::no_sensor_msg() const {
    MsgBoxQuestion(_("No filament sensor detected. Verify that the sensor is connected and try again."));
}

bool MI_FILAMENT_SENSOR::init_index() const {
    fsensor_t fs = FS_instance().WaitInitialized();
    fs_not_connected = fs == fsensor_t::NotConnected;
    if (fs_not_connected) //tried to enable but there is no sensor
    {
        FS_instance().Disable();
        fs_not_connected = true;
        fs = fsensor_t::Disabled;
    }
    return fs == fsensor_t::Disabled ? 0 : 1;
}

void MI_FILAMENT_SENSOR::CheckDisconnected() {
    if (consumeNotConnected() || FS_instance().WaitInitialized() == fsensor_t::NotConnected) {
        FS_instance().Disable();
        index = 0;
        no_sensor_msg();
    }
}

bool MI_FILAMENT_SENSOR::consumeNotConnected() {
    bool ret = fs_not_connected;
    fs_not_connected = false;
    return ret;
}

void MI_FILAMENT_SENSOR::OnChange(size_t old_index) {
    old_index == 1 ? FS_instance().Disable() : FS_instance().Enable();

    fsensor_t fs = FS_instance().WaitInitialized();
    if (fs == fsensor_t::NotConnected) //tried to enable but there is no sensor
    {
        FS_instance().Disable();
        index = old_index;
        fs_not_connected = true;
    }
}

bool MI_FILAMENT_SENSOR::fs_not_connected = false;

#ifdef _DEBUG
using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_SERVICE, MI_HW_SETUP, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif // BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME,
    MI_DEVHASH_IN_QR, MI_LANGUAGE, MI_SORT_FILES,
    MI_SOUND_TYPE, MI_XFLASH_RESET, MI_XFLASH_DELETE, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EEPROM>;
#else
using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_HW_SETUP, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_FS_AUTOLOAD, MI_TIMEOUT, MI_FAN_CHECK,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_DEVHASH_IN_QR, MI_LANGUAGE>;
#endif

class ScreenMenuSettings : public Screen {
public:
    constexpr static const char *label = N_("SETTINGS");
    ScreenMenuSettings();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

ScreenFactory::UniquePtr GetScreenMenuSettings() {
    return ScreenFactory::Screen<ScreenMenuSettings>();
}

ScreenMenuSettings::ScreenMenuSettings()
    : Screen(_(label)) {
    if (sheet_number_of_calibrated() > 1) {
        Item<MI_CURRENT_PROFILE>().UpdateLabel();
        Item<MI_CURRENT_PROFILE>().Show();
    }
}

void ScreenMenuSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        Item<MI_FILAMENT_SENSOR>().CheckDisconnected();
    }

    SuperWindowEvent(sender, event, param);
}
