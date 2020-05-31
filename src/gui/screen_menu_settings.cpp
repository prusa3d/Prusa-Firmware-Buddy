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
/*
extern osThreadId webServerTaskHandle;

const char *settings_opt_enable_disable[] = { "Off", "On", NULL };
const char *sound_opt_modes[] = { "Once", "Loud", "Silent", "Assist", NULL };
const eSOUND_MODE e_sound_modes[] = { eSOUND_MODE_ONCE, eSOUND_MODE_LOUD, eSOUND_MODE_SILENT, eSOUND_MODE_ASSIST };
#ifdef _DEBUG
const char *sound_opt_types[] = { "ButtonEcho", "StandardPrompt", "StandardAlert", "EncoderMove", "BlindAlert", NULL };
const eSOUND_TYPE e_sound_types[] = { eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_EncoderMove, eSOUND_TYPE_BlindAlert };
#endif // _DEBUG

typedef enum {
    MI_RETURN,
    MI_TEMPERATURE,
    MI_MOVE_AXIS,
    MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS,
#ifdef _DEBUG
    MI_SERVICE,
    MI_TEST,
#endif //_DEBUG
    MI_FW_UPDATE,
    MI_FILAMENT_SENSOR,
    MI_TIMEOUT,
#ifdef BUDDY_ENABLE_ETHERNET
    MI_LAN_SETTINGS,
#endif //BUDDY_ENABLE_ETHERNET
    MI_SAVE_DUMP,
    MI_SOUND_MODE,
#ifdef _DEBUG
    MI_SOUND_TYPE,
    MI_HF_TEST_0,
    MI_HF_TEST_1,
    MI_EE_LOAD_400,
    MI_EE_LOAD_401,
    MI_EE_LOAD_402,
    MI_EE_LOAD_403RC1,
    MI_EE_LOAD_403,
    MI_EE_LOAD,
    MI_EE_SAVE,
    MI_EE_SAVEXML,
#endif //_DEBUG
    MI_COUNT
} MI_t;



    // memcpy(psmd->items + 1, _menu_settings_items, (MI_COUNT - 1) * sizeof(menu_item_t));

    fsensor_t fs = fs_wait_inicialized();
    if (fs == FS_NOT_CONNECTED) {
        fs_disable();
        fs = FS_DISABLED;
    }
    psmd->items[MI_FILAMENT_SENSOR].item.data.wi_switch.index = (fs != FS_DISABLED);
    psmd->items[MI_TIMEOUT].item.data.wi_switch.index = menu_timeout_enabled; //st25dv64k_user_read(MENU_TIMEOUT_FLAG_ADDRESS)

    for (size_t i = 0; i < sizeof(e_sound_modes); i++) {
        if (e_sound_modes[i] == Sound_GetMode()) {
            psmd->items[MI_SOUND_MODE].item.data.wi_switch.index = i;
            break;
        }
    }
}





        case MI_TIMEOUT:
            if (menu_timeout_enabled == 1) {
                menu_timeout_enabled = 0;
                gui_timer_delete(gui_get_menu_timeout_id());
                //st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)0);
            } else {
                menu_timeout_enabled = 1;
                //st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)1);
            }
            break;
        case MI_FILAMENT_SENSOR: {
            fsensor_t fs = fs_get_state();
            fs == FS_DISABLED ? fs_enable() : fs_disable();
            fs = fs_wait_inicialized();
            if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
            {
                fs_disable();
                psmd->items[MI_FILAMENT_SENSOR].item.data.wi_switch.index = 0;
                //todo need to invalidate that stupid item, but I cannot grrrr
                gui_msgbox("No filament sensor detected. Verify that the sensor is connected and try again.", MSGBOX_ICO_QUESTION);
            }
        } break;
        case MI_SOUND_MODE:
            Sound_SetMode(e_sound_modes[psmd->items[MI_SOUND_MODE].item.data.wi_switch.index]);
            break;
#ifdef _DEBUG
        case MI_SOUND_TYPE:
            if (e_sound_types[psmd->items[MI_SOUND_TYPE].item.data.wi_switch.index] == eSOUND_TYPE_StandardPrompt) {
                gui_msgbox_prompt("eSOUND_TYPE_StandardPrompt - test", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
            } else {
                Sound_Play(e_sound_types[psmd->items[MI_SOUND_TYPE].item.data.wi_switch.index]);
            }



*/

//psmd->items[MI_FILAMENT_SENSOR] = (menu_item_t) { { "Fil. sens.", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
//psmd->items[MI_TIMEOUT] = (menu_item_t) { { "Timeout", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
//psmd->items[MI_SOUND_MODE] = (menu_item_t) { { "Sound Mode", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
//psmd->items[MI_SOUND_TYPE] = (menu_item_t) { { "Sound Type", 0, WI_SWITCH }, SCREEN_MENU_NO_SCREEN };
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

constexpr const char *str_off = "Off";
constexpr const char *str_on = "On";
constexpr const std::array<const char *, 2> off_on = { str_off, str_on };

class MI_FILAMENT_SENSOR : public WI_SWITCH_t<off_on.size()> {
    constexpr static const char *const label = "Fil. sens.";

    size_t get_index() {
        fsensor_t fs = fs_wait_inicialized();
        if (fs == FS_NOT_CONNECTED) {
            fs_disable();
            fs = FS_DISABLED;
        }
        return fs == FS_DISABLED ? 1 : 0;
    }

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_t<off_on.size()>(get_index(), off_on, label, 0, true, false) {}
    virtual void OnClick() {
        //index did not change yet, chage fsensor
        index == 0 ? fs_disable() : fs_enable();
        fsensor_t fs = fs_wait_inicialized();
        if (fs == FS_NOT_CONNECTED) //tried to enable but there is no sensor
        {
            fs_disable();
            ClrIndex(); //set index 0
            //todo need to invalidate
            gui_msgbox("No filament sensor detected. Verify that the sensor is connected and try again.", MSGBOX_ICO_QUESTION);
        }
    }
};

#ifdef _DEBUG
using Screen = screen_menu_data_t<false, true, false, MI_RETURN, MI_TEMPERATURE, MI_MOVE_AXIS, MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS, MI_SERVICE, MI_TEST, MI_FW_UPDATE,
    MI_FILAMENT_SENSOR,
    //MI_TIMEOUT,

    MI_LAN_SETTINGS,

    MI_SAVE_DUMP,
    //MI_SOUND_MODE,

    //MI_SOUND_TYPE,
    MI_HF_TEST_0, MI_HF_TEST_1,
    MI_EE_LOAD_400, MI_EE_LOAD_401, MI_EE_LOAD_402, MI_EE_LOAD_403RC1, MI_EE_LOAD_403,
    MI_EE_LOAD, MI_EE_SAVE, MI_EE_SAVEXML>;
#else
using Screen = screen_menu_data_t<false, true, false, MI_RETURN>;
#endif
static void init(screen_t *screen) {
    constexpr static const char *label = "Settings";
    Screen::Create(screen, label);
}

screen_t screen_menu_settings = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};
extern "C" screen_t *const get_scr_menu_settings() { return &screen_menu_settings; }
