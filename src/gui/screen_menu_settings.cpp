// screen_menu_settings.c

#include "gui.h"
#include "config.h"
#include "app.h"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "cmsis_os.h"
#include "sys.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#ifdef BUDDY_ENABLE_ETHERNET
    #include "screen_lan_settings.h"
#endif //BUDDY_ENABLE_ETHERNET
#include "screen_menu_fw_update.h"
#include "filament_sensor.h"
#include "screens.h"
#include "dump.h"
#include "sound_C_wrapper.h"
#include "WindowMenuItems.hpp"

#pragma pack(push, 1)

/*****************************************************************************/
//MI_FILAMENT_SENSOR
class MI_FILAMENT_SENSOR : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "Fil. sens.";

    size_t init_index() const {
        fsensor_t fs = fs_wait_inicialized();
        if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
        {
            fs_disable();
            gui_msgbox("No filament sensor detected. Verify that the sensor is connected and try again.", MSGBOX_ICO_QUESTION);
            fs = FS_DISABLED;
        }
        return fs == FS_DISABLED ? 1 : 0;
    }

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_OFF_ON_t(init_index(), label, 0, true, false) {}

protected:
    virtual void OnChange(size_t old_index) {
        old_index == 0 ? fs_disable() : fs_enable();
        fsensor_t fs = fs_wait_inicialized();
        if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
        {
            fs_disable();
            index = old_index;
            gui_msgbox("No filament sensor detected. Verify that the sensor is connected and try again.", MSGBOX_ICO_QUESTION);
        }
    }
};

/*****************************************************************************/
//MI_TIMEOUT
//if needed to remeber after poweroff
//use st25dv64k_user_read(MENU_TIMEOUT_FLAG_ADDRESS) st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)1 or 0);
class MI_TIMEOUT : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "Timeout";
    static bool timeout_enabled;

public:
    MI_TIMEOUT()
        : WI_SWITCH_OFF_ON_t(timeout_enabled ? 0 : 1, label, 0, true, false) {}
    virtual void OnChange(size_t old_index) {
        if (timeout_enabled) {
            gui_timer_delete(gui_get_menu_timeout_id());
        }
        timeout_enabled = !timeout_enabled;
    }
};
bool MI_TIMEOUT::timeout_enabled = true;

/*****************************************************************************/
//MI_SOUND_MODE
class MI_SOUND_MODE : public WI_SWITCH_t<4> {
    constexpr static const char *const label = "Sound Mode";

    constexpr static const char *str_Once = "Once";
    constexpr static const char *str_Loud = "Loud";
    constexpr static const char *str_Silent = "Silent";
    constexpr static const char *str_Assist = "Assist";
    size_t init_index() const {
        size_t sound_mode = Sound_GetMode();
        return sound_mode > 4 ? eSOUND_MODE_DEFAULT : sound_mode;
    }

public:
    MI_SOUND_MODE()
        : WI_SWITCH_t<4>(init_index(), label, 0, true, false, str_Once, str_Loud, str_Silent, str_Assist) {}
    virtual void OnChange(size_t old_index) {
        Sound_SetMode(static_cast<eSOUND_MODE>(index));
    }
};

/*****************************************************************************/
//MI_SOUND_TYPE
class MI_SOUND_TYPE : public WI_SWITCH_t<5> {
    constexpr static const char *const label = "Sound Type";

    constexpr static const char *str_ButtonEcho = "ButtonEcho";
    constexpr static const char *str_StandardPrompt = "StandardPrompt";
    constexpr static const char *str_StandardAlert = "StandardAlert";
    constexpr static const char *str_EncoderMove = "EncoderMove";
    constexpr static const char *str_BlindAlert = "BlindAlert";

public:
    MI_SOUND_TYPE()
        : WI_SWITCH_t<5>(0, label, 0, true, false, str_ButtonEcho, str_StandardPrompt, str_StandardAlert, str_EncoderMove, str_BlindAlert) {}
    virtual void OnChange(size_t old_index) {
        if (old_index == eSOUND_TYPE_StandardPrompt) {
            gui_msgbox_prompt("eSOUND_TYPE_StandardPrompt - test", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        } else {
            Sound_Play(static_cast<eSOUND_TYPE>(old_index));
        }
    }
};

#pragma pack(pop)

#ifdef _DEBUG
enum { FsensorPos = 8 };
using parent = screen_menu_data_t<false, true, false, MI_RETURN, MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_SERVICE, MI_TEST, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    #endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE, MI_SOUND_TYPE, MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EE_LOAD_400, MI_EE_LOAD_401, MI_EE_LOAD_402, MI_EE_LOAD_403RC1, MI_EE_LOAD_403,
    MI_EE_LOAD, MI_EE_SAVE, MI_EE_SAVEXML>;
#else
enum { FsensorPos = 6 };
using parent = screen_menu_data_t<false, true, false, MI_RETURN, MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_FW_UPDATE, MI_FILAMENT_SENSOR, MI_TIMEOUT,
    #ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
    #endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP, MI_SOUND_MODE>;
#endif

#pragma pack(push, 1)
class ScreenMenuSettings : public parent {
public:
    constexpr static const char *label = "Settings";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};
#pragma pack(pop)

/*****************************************************************************/
//static member method definition
void ScreenMenuSettings::Init(screen_t *screen) {
    Create(screen, label);
}

int ScreenMenuSettings::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuSettings *const ths = reinterpret_cast<ScreenMenuSettings *>(screen->pdata);
    if (event == WINDOW_EVENT_LOOP) {
        //todo handle if FS disconnects
    }

    return ths->Event(window, event, param);
}

screen_t screen_menu_settings = {
    0,
    0,
    ScreenMenuSettings::Init,
    ScreenMenuSettings::CDone,
    ScreenMenuSettings::CDraw,
    ScreenMenuSettings::CEvent,
    sizeof(ScreenMenuSettings), //data_size
    0,                          //pdata
};
extern "C" screen_t *const get_scr_menu_settings() { return &screen_menu_settings; }
