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

    void no_sensor_msg() const {
        MsgBoxQuestion(_("No filament sensor detected. Verify that the sensor is connected and try again."));
    }

    size_t init_index() const {
        fsensor_t fs = fs_wait_initialized();
        if (fs == fsensor_t::NotConnected) //tried to enable but there is no sensor
        {
            fs_disable();
            no_sensor_msg();
            fs = fsensor_t::Disabled;
        }
        return fs == fsensor_t::Disabled ? 0 : 1;
    }
    // bool fs_not_connected;

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_OFF_ON_t(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    void CheckDisconnected() {
        fsensor_t fs = fs_wait_initialized();
        if (fs == fsensor_t::NotConnected) { //only way to have this state is that fs just disconnected
            fs_disable();
            index = 0;
            no_sensor_msg();
        }
    }

protected:
    virtual void OnChange(size_t old_index) {
        old_index == 1 ? fs_disable() : fs_enable();
        fsensor_t fs = fs_wait_initialized();
        if (fs == fsensor_t::NotConnected) //tried to enable but there is no sensor
        {
            fs_disable();
            index = old_index;
            no_sensor_msg();
        }
    }
};

#ifdef _DEBUG
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_SERVICE, MI_HW_SETUP, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif // BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME,
    MI_QR_PRIVACY, MI_LANGUAGE, MI_SORT_FILES,
    MI_SOUND_TYPE, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EEPROM>;
#else
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_TEMPERATURE, MI_CURRENT_PROFILE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_HW_SETUP, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_QR_PRIVACY, MI_LANGUAGE>;
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
