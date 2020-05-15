// menu_tune.cpp

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "filament.h"
#include "menu_vars.h"
#include "screens.h"
#include "eeprom.h"
/*
enum {
    MI_RETURN,
    MI_SPEED,
    MI_NOZZLE,
    MI_HEATBED,
    MI_PRINTFAN,
    MI_FLOWFACT,
    MI_BABYSTEP,
    MI_FILAMENT,
    MI_LAN_SETTINGS,
    MI_VERSION_INFO,
#ifdef _DEBUG
    MI_TEST,
#endif //_DEBUG
    MI_MESSAGES,
    MI_COUNT
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

WI_RETURN_t ret;

using this_screen_data_t = screen_menu_data_t<
    true,
    false,
    false,
    MI_RETURN,
    WI_SPIN_t,
    WI_SPIN_t,
    WI_SPIN_t,
    WI_SPIN_t,
    WI_SPIN_t,
    WI_SPIN_FL_t,
    WI_LABEL_t,
    WI_LABEL_t,
    WI_LABEL_t>;

void screen_menu_tune_timer(screen_t *screen, uint32_t mseconds);
void screen_menu_tune_chanege_filament(screen_t *screen);

void screen_menu_tune_init(screen_t *screen) {
    marlin_vars_t *vars = marlin_update_vars(
        MARLIN_VAR_MSK_TEMP_TARG | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT));

    screen_menu_init(screen, "TUNE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);

    psmd->items[MI_RETURN] = menu_item_return;
    psmd->items[MI_SPEED] = { WI_SPIN_t(int32_t(vars->print_speed * 1000), feedrate_range, "Speed"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_NOZZLE] = { WI_SPIN_t(int32_t(vars->target_nozzle * 1000), nozzle_range, "Nozzle"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_HEATBED] = { WI_SPIN_t(int32_t(vars->target_bed * 1000), heatbed_range, "HeatBed"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_PRINTFAN] = { WI_SPIN_t(int32_t(vars->fan_speed * 1000), printfan_range, "Fan Speed"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_FLOWFACT] = { WI_SPIN_t(int32_t(vars->flow_factor * 1000), flowfact_range, "Flow Factor"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_BABYSTEP] = { WI_SPIN_FL_t(vars->z_offset, zoffset_fl_range, zoffset_fl_format, "Live Adjust Z"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_FILAMENT] = { WI_LABEL_t("Change Filament"), SCREEN_MENU_NO_SCREEN };
    psmd->items[MI_LAN_SETTINGS] = { WI_LABEL_t("LAN Setings"), get_scr_lan_settings() };
    psmd->items[MI_VERSION_INFO] = { WI_LABEL_t("Version Info"), get_scr_version_info() };
#ifdef _DEBUG
    psmd->items[MI_TEST] = { WI_LABEL_t("Test"), get_scr_test() };
#endif //_DEBUG
    psmd->items[MI_MESSAGES] = { WI_LABEL_t("Messages"), get_scr_messages() };
}

int screen_menu_tune_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    static float z_offs = 0;

    if (
        marlin_all_axes_homed() && marlin_all_axes_known() && (marlin_command() != MARLIN_CMD_G28) && (marlin_command() != MARLIN_CMD_G29) && (marlin_command() != MARLIN_CMD_M109) && (marlin_command() != MARLIN_CMD_M190)) {
        psmd->items[MI_FILAMENT].item.Enable();
    } else {
        psmd->items[MI_FILAMENT].item.Disable();
    }

    if (screen_menu_event(screen, window, event, param))
        return 1;

    if (event == WINDOW_EVENT_CHANGING) {
        switch ((int)param) {
        case MI_BABYSTEP:
            marlin_do_babysteps_Z(CAST_WI_SPIN_FL(MI_BABYSTEP).value - z_offs); //psmd->items[MI_BABYSTEP].item.data.wi_spin_fl.value
            z_offs = CAST_WI_SPIN_FL(MI_BABYSTEP).value;
            break;
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case MI_SPEED:
            marlin_set_print_speed((uint16_t)(CAST_WI_SPIN(MI_SPEED).value / 1000));
            break;
        case MI_NOZZLE:
            marlin_set_target_nozzle((float)(CAST_WI_SPIN(MI_NOZZLE).value) / 1000);
            break;
        case MI_HEATBED:
            marlin_set_target_bed((float)(CAST_WI_SPIN(MI_HEATBED).value) / 1000);
            break;
        case MI_PRINTFAN:
            marlin_set_fan_speed((uint8_t)(CAST_WI_SPIN(MI_PRINTFAN).value / 1000));
            break;
        case MI_FLOWFACT:
            marlin_set_flow_factor((uint16_t)(CAST_WI_SPIN(MI_FLOWFACT).value / 1000));
            break;
        case MI_BABYSTEP:
            marlin_set_z_offset(CAST_WI_SPIN_FL(MI_BABYSTEP).value);
            eeprom_set_var(EEVAR_ZOFFSET, marlin_get_var(MARLIN_VAR_Z_OFFSET));
            break;
        }
    } else if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_FILAMENT:
            //screen_menu_tune_chanege_filament(screen);
            //todo this should not be here
            if ((psmd->items[MI_FILAMENT].item.IsEnabled())) {
                marlin_gcode_push_front("M600");
            }
            break;
        case MI_BABYSTEP:
            z_offs = CAST_WI_SPIN_FL(MI_BABYSTEP).value;
            break;
        }
    }

    return 0;
}

void screen_menu_tune_timer(screen_t *screen, uint32_t mseconds) {
    static uint32_t last_timer_repaint = 0;
    // if (psmd->pfooter->last_timer == mseconds) return; // WTF is footer timer doing here?
    //if (mseconds % 500 == 0) // is this really necessary? That's why the tune menu behaves like a Ferrari off-road (i.e. sitting stuck :) )
    if ((mseconds - last_timer_repaint) >= 500) {
        marlin_vars_t *vars = marlin_vars();
        bool editing = psmd->menu.mode > 0;
        int index = psmd->menu.index;
        if (!editing || index != MI_SPEED) {
            CAST_WI_SPIN(MI_SPEED).value = vars->print_speed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_NOZZLE) {
            CAST_WI_SPIN(MI_NOZZLE).value = vars->target_nozzle * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_HEATBED) {
            CAST_WI_SPIN(MI_HEATBED).value = vars->target_bed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_PRINTFAN) {
            CAST_WI_SPIN(MI_PRINTFAN).value = vars->fan_speed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_FLOWFACT) {
            CAST_WI_SPIN(MI_FLOWFACT).value = vars->flow_factor * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        last_timer_repaint = mseconds;
    }
}

screen_t screen_menu_tune = {
    0,
    0,
    screen_menu_tune_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_tune_event,
    sizeof(this_screen_data_t), //data_size
    0,                          //pdata
};
*/

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

using Screen = screen_menu_data_t<false, true, false, MI_RETURN>;

static void init(screen_t *screen) {
    Screen::CInit(screen);
}

screen_t screen_menu_tune = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};

extern "C" screen_t *const get_scr_menu_tune() { return &screen_menu_tune; }
