// screen_menu_calibration.c

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "wizard/wizard.h"
#include "window_dlg_wait.h"
#include "screens.h"

#include "menu_vars.h"
#include "eeprom.h"

typedef enum {
    MI_RETURN,
    MI_WIZARD,
    MI_Z_OFFSET,
    MI_AUTO_HOME,
    MI_MESH_BED,
    MI_SELFTEST,
    MI_CALIB_FIRST,
    MI_COUNT
} MI_t;

const menu_item_t _menu_calibration_items[] = {
    { { "Wizard", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Z-offset", 0, WI_SPIN_FL }, SCREEN_MENU_NO_SCREEN }, //set later
    { { "Auto Home", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Mesh Bed Level.", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "SelfTest", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "First Layer Cal.", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

//"C inheritance" of screen_menu_data_t with data items
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    screen_menu_data_t base;
    menu_item_t items[MI_COUNT];

} this_screen_data_t;

#pragma pack(pop)

void screen_menu_calibration_init(screen_t *screen) {
    marlin_vars_t *vars;
    screen_menu_init(screen, "CALIBRATION", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_calibration_items, (MI_COUNT - 1) * sizeof(menu_item_t));

    vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
    psmd->items[MI_Z_OFFSET].item.wi_spin_fl.value = vars->z_offset;
    psmd->items[MI_Z_OFFSET].item.wi_spin_fl.prt_format = zoffset_fl_format;
    psmd->items[MI_Z_OFFSET].item.wi_spin_fl.range = zoffset_fl_range;
}

int8_t gui_marlin_G28_or_G29_in_progress() {
    uint32_t cmd = marlin_command();
    if ((cmd == MARLIN_CMD_G28) || (cmd == MARLIN_CMD_G29))
        return -1;
    else
        return 0;
}

int screen_menu_calibration_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (screen_menu_event(screen, window, event, param))
        return 1;
    if ((event == WINDOW_EVENT_CHANGING) && ((int)param == MI_Z_OFFSET))
        marlin_set_z_offset(psmd->items[MI_Z_OFFSET].item.wi_spin_fl.value);
    else if ((event == WINDOW_EVENT_CHANGE) && ((int)param == MI_Z_OFFSET))
        eeprom_set_var(EEVAR_ZOFFSET, marlin_get_var(MARLIN_VAR_Z_OFFSET));
    else if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_WIZARD:
            wizard_run_complete();
            break;
        case MI_AUTO_HOME:
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_gcode("G28");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
            break;
        case MI_MESH_BED:
            if (!marlin_all_axes_homed()) {
                marlin_event_clr(MARLIN_EVT_CommandBegin);
                marlin_gcode("G28");
                while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                    marlin_client_loop();
                gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
            }
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_gcode("G29");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            gui_dlg_wait(gui_marlin_G28_or_G29_in_progress);
            break;
        case MI_SELFTEST:
            wizard_run_selftest();
            break;
        case MI_CALIB_FIRST:
            wizard_run_firstlay();
            break;
        }
    }
    return 0;
}

screen_t screen_menu_calibration = {
    0,
    0,
    screen_menu_calibration_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_calibration_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};

extern "C" screen_t *const get_scr_menu_calibration() { return &screen_menu_calibration; }
