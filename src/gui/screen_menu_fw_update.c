/*
 * screen_menu_fw_update.c
 *
 *  Created on: Dec 18, 2019
 *      Author: Migi
 */

#include "screen_menu_fw_update.h"
#include "sys.h"

const char *opt_on_off[] = { "On", "Off", NULL };

typedef enum {
    MI_RETURN,
    MI_ALWAYS,
    MI_ON_RESTART,
    MI_COUNT,
} MI_t;

//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_fw_update_init(screen_t *screen) {
    screen_menu_init(screen, "FW UPDATE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 1);
    psmd->items[MI_RETURN] = menu_item_return;
    psmd->items[MI_ALWAYS] = (menu_item_t) { { "Always", 0, WI_SWITCH, .wi_switch_select = { 0, opt_on_off } }, SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_ON_RESTART] = (menu_item_t) { { "On restart", 0, WI_SWITCH, .wi_switch_select = { 0, opt_on_off } }, SCREEN_MENU_NO_SCREEN };
    if (sys_fw_update_is_enabled()) {
        psmd->items[MI_ALWAYS].item.wi_switch_select.index = 0;
        psmd->items[MI_ON_RESTART].item.wi_switch_select.index = 0;
        psmd->items[MI_ON_RESTART].item.type |= WI_DISABLED;
    } else {
        psmd->items[MI_ALWAYS].item.wi_switch_select.index = 1;
        psmd->items[MI_ON_RESTART].item.wi_switch_select.index = sys_fw_update_on_restart_is_enabled() ? 0 : 1;
    }
    psmd->help.font = resource_font(IDR_FNT_SPECIAL);
    window_set_text(psmd->help.win.id, "Select when you want\nto automatically flash\nupdated firmware\nfrom USB flash disk.");
}

int screen_menu_fw_update_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param))
        return 1;

    if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_ALWAYS:
            if (sys_fw_update_is_enabled()) {
                sys_fw_update_disable();
                psmd->items[MI_ON_RESTART].item.wi_switch_select.index = sys_fw_update_on_restart_is_enabled() ? 0 : 1;
                psmd->items[MI_ON_RESTART].item.type &= ~WI_DISABLED;
            } else {
                sys_fw_update_enable();
                psmd->items[MI_ON_RESTART].item.type |= WI_DISABLED;
                psmd->items[MI_ON_RESTART].item.wi_switch_select.index = 0;
            }
            break;
        case MI_ON_RESTART:
            if (!(psmd->items[MI_ON_RESTART].item.type & WI_DISABLED)) {
                sys_fw_update_on_restart_is_enabled() ? sys_fw_update_on_restart_disable() : sys_fw_update_on_restart_enable();
            }
            break;
        }
    }
    return 0;
}

screen_t screen_menu_fw_update = {
    0,
    0,
    screen_menu_fw_update_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_fw_update_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

screen_t *const pscreen_menu_fw_update = &screen_menu_fw_update;
