// screen_menu_settings.c

#include "gui.h"
#include "app.h"
#include "marlin_client.h"
#include "screen_menu.h"
#include "cmsis_os.h"
#include "sys.h"
#include "eeprom.h"
#include "screen_lan_settings.h"
#include "screen_menu_fw_update.h"
#include "filament_sensor.h"
#include "dump.h"

//hardfault test
#define HF_TEST

extern screen_t screen_menu_temperature;
extern screen_t screen_menu_move;
#ifdef _DEBUG
extern screen_t screen_menu_service;
extern screen_t screen_test;
#endif //_DEBUG
extern osThreadId webServerTaskHandle;

const char *settings_opt_enable_disable[] = { "Off", "On", NULL };

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
    MI_LAN_SETTINGS,
    MI_SAVE_DUMP,
#ifdef HF_TEST
	MI_HF_TEST_0,
	MI_HF_TEST_1,
#endif //HF_TEST
} MI_t;

const menu_item_t _menu_settings_items[] = {
    { { "Temperature", 0, WI_LABEL }, &screen_menu_temperature },
    { { "Move Axis", 0, WI_LABEL }, &screen_menu_move },
    { { "Disable Steppers", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Factory Reset", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#ifdef _DEBUG
    { { "Service", 0, WI_LABEL }, &screen_menu_service },
    { { "Test", 0, WI_LABEL }, &screen_test },
#endif //_DEBUG
    { { "FW Update", 0, WI_LABEL }, &screen_menu_fw_update },
    { { "Fil. sens.", 0, WI_SWITCH, .wi_switch_select = { 0, settings_opt_enable_disable } }, SCREEN_MENU_NO_SCREEN },
    { { "Timeout", 0, WI_SWITCH, .wi_switch_select = { 0, settings_opt_enable_disable } }, SCREEN_MENU_NO_SCREEN },
    { { "LAN Settings", 0, WI_LABEL }, &screen_lan_settings },
    { { "Save CrashDUMP", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#ifdef HF_TEST
    { { "HF0 test", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "HF1 test", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#endif //HF_TEST
};

void screen_menu_settings_init(screen_t *screen) {
    int count = sizeof(_menu_settings_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "SETTINGS", count + 1, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_settings_items, count * sizeof(menu_item_t));

    fsensor_t fs = fs_wait_inicialized();
    if (fs == FS_NOT_CONNECTED) {
        fs_disable();
        fs = FS_DISABLED;
    }
    psmd->items[MI_FILAMENT_SENSOR].item.wi_switch_select.index = (fs != FS_DISABLED);
    psmd->items[MI_TIMEOUT].item.wi_switch_select.index = menu_timeout_enabled; //st25dv64k_user_read(MENU_TIMEOUT_FLAG_ADDRESS)
}

int screen_menu_settings_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param))
        return 1;
    if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_SAVE_DUMP:
            if (dump_save_xflash_to_usb("dump.bin"))
                gui_msgbox("Crash dump report 'dump.bin' saved to usb flash", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
            else
                gui_msgbox("Error saving crash dump report to usb flash", MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
            break;
#ifdef HF_TEST
        case MI_HF_TEST_0:
        	dump_hardfault_test_0();
            break;
        case MI_HF_TEST_1:
        	dump_hardfault_test_1();
            break;
#endif //HF_TEST
        case MI_DISABLE_STEP:
            marlin_gcode("M18");
            break;
        case MI_FACTORY_DEFAULTS:
            if (gui_msgbox("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?", MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES) {
                marlin_event_clr(MARLIN_EVT_FactoryReset);
                marlin_gcode("M502");
                while (!marlin_event_clr(MARLIN_EVT_FactoryReset)) {
                    gui_loop();
                }
                marlin_event_clr(MARLIN_EVT_StoreSettings);
                marlin_gcode("M500");
                while (!marlin_event_clr(MARLIN_EVT_StoreSettings)) {
                    gui_loop();
                }
                eeprom_defaults();
                gui_msgbox("Factory defaults loaded. The system will now restart.", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
                sys_reset();
            }
            break;
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
                psmd->items[MI_FILAMENT_SENSOR].item.wi_switch_select.index = 0;
                //todo need to invalidate that stupid item, but I cannot grrrr
                gui_msgbox("No filament sensor detected. Verify that the sensor is connected and try again.", MSGBOX_ICO_QUESTION);
            }
        } break;
        }
    }
    return 0;
}

screen_t screen_menu_settings = {
    0,
    0,
    screen_menu_settings_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_settings_event,
    sizeof(screen_menu_data_t), //data_size
    0, //pdata
};

const screen_t *pscreen_menu_settings = &screen_menu_settings;
