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
#include "filament_sensor.h"
#include "dump.h"
#include "sound.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "i18n.h"

/*****************************************************************************/
//MI_FILAMENT_SENSOR
class MI_FILAMENT_SENSOR : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Fil. sens.");

    size_t init_index() const {
        fsensor_t fs = fs_wait_initialized();
        if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
        {
            fs_disable();
            MsgBoxQuestion(_("No filament sensor detected. Verify that the sensor is connected and try again."));
            fs = FS_DISABLED;
        }
        return fs == FS_DISABLED ? 0 : 1;
    }
    // bool fs_not_connected;

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_OFF_ON_t(init_index(), label, 0, true, false) {}
    void CheckDisconnected() {
        fsensor_t fs = fs_wait_initialized();
        if (fs == FS_NOT_CONNECTED) { //only way to have this state is that fs just disconnected
            fs_disable();
            index = 0;
            MsgBoxQuestion(_("No filament sensor detected. Verify that the sensor is connected and try again."));
        }
    }

protected:
    virtual void OnChange(size_t old_index) {
        old_index == 1 ? fs_disable() : fs_enable();
        fsensor_t fs = fs_wait_initialized();
        if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
        {
            fs_disable();
            index = old_index;
            MsgBoxQuestion(_("No filament sensor detected. Verify that the sensor is connected and try again."));
        }
    }
};

#ifdef _DEBUG
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_SERVICE, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif // BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME,
    MI_LANGUAGE, MI_SORT_FILES,
    MI_SOUND_TYPE, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EE_LOAD_400, MI_EE_LOAD_401, MI_EE_LOAD_402, MI_EE_LOAD_403RC1, MI_EE_LOAD_403,
    MI_EE_LOAD, MI_EE_SAVE, MI_EE_SAVEXML>;
#else
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    MI_TIMEZONE,
    #endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_LANGUAGE>;
#endif

class ScreenMenuSettings : public Screen {
public:
    constexpr static const char *label = N_("SETTINGS");
    ScreenMenuSettings()
        : Screen(_(label)) {}
    virtual void windowEvent(window_t *sender, uint8_t ev, void *param) override;
};

ScreenFactory::UniquePtr GetScreenMenuSettings() {
    return ScreenFactory::Screen<ScreenMenuSettings>();
}

void ScreenMenuSettings::windowEvent(window_t *sender, uint8_t ev, void *param) {
    if (ev == WINDOW_EVENT_LOOP) {
        Item<MI_FILAMENT_SENSOR>().CheckDisconnected();
    }

    Screen::windowEvent(sender, ev, param);
}
