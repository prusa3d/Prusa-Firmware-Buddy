// menu_tune.cpp

#include "gui.h"
#include "screen_menu.h"
#include "marlin_client.h"
#include "filament.h"
#include "menu_vars.h"
#include "eeprom.h"

#ifdef _DEBUG
extern screen_t screen_test;
#endif //_DEBUG
extern screen_t screen_menu_info;
extern screen_t screen_messages;

enum {
    MI_RETURN,
    MI_SPEED,
    MI_NOZZLE,
    MI_HEATBED,
    MI_PRINTFAN,
    MI_FLOWFACT,
    MI_BABYSTEP,
    MI_FILAMENT,
    MI_INFO,
#ifdef _DEBUG
    MI_TEST,
#endif //_DEBUG
    MI_MESSAGES,
    MI_COUNT
};

//cannot use .wi_spin = { 0, feedrate_range } ...
//sorry, unimplemented: non-trivial designated initializers not supported
const menu_item_t _menu_tune_items[] = {
    { { "Speed", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN },            //set later
    { { "Nozzle", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN },           //set later
    { { "HeatBed", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN },          //set later
    { { "Fan Speed", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN },        //set later
    { { "Flow Factor", 0, WI_SPIN }, SCREEN_MENU_NO_SCREEN },      //set later
    { { "Live Adjust Z", 0, WI_SPIN_FL }, SCREEN_MENU_NO_SCREEN }, //set later
    { { "Change Filament", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Info", 0, WI_LABEL | WI_DISABLED }, &screen_menu_info },
#ifdef _DEBUG
    { { "Test", 0, WI_LABEL }, &screen_test },
#endif //_DEBUG
    { { "Messages", 0, WI_LABEL }, &screen_messages },
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

void screen_menu_tune_timer(screen_t *screen, uint32_t mseconds);
void screen_menu_tune_chanege_filament(screen_t *screen);

void screen_menu_tune_init(screen_t *screen) {
    marlin_vars_t *vars;
    screen_menu_init(screen, "TUNE", ((this_screen_data_t *)screen->pdata)->items, MI_COUNT, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_tune_items, (MI_COUNT - 1) * sizeof(menu_item_t));

    vars = marlin_update_vars(
        MARLIN_VAR_MSK_TEMP_TARG | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT));

    psmd->items[MI_SPEED].item.wi_spin.value = (int32_t)(vars->print_speed * 1000);
    psmd->items[MI_SPEED].item.wi_spin.range = feedrate_range;

    psmd->items[MI_NOZZLE].item.wi_spin.value = (int32_t)(vars->target_nozzle * 1000);
    psmd->items[MI_NOZZLE].item.wi_spin.range = nozzle_range;

    psmd->items[MI_HEATBED].item.wi_spin.value = (int32_t)(vars->target_bed * 1000);
    psmd->items[MI_HEATBED].item.wi_spin.range = heatbed_range;

    psmd->items[MI_PRINTFAN].item.wi_spin.value = (int32_t)(vars->fan_speed * 1000);
    psmd->items[MI_PRINTFAN].item.wi_spin.range = printfan_range;

    psmd->items[MI_FLOWFACT].item.wi_spin.value = (int32_t)(vars->flow_factor * 1000);
    psmd->items[MI_FLOWFACT].item.wi_spin.range = flowfact_range;

    psmd->items[MI_BABYSTEP].item.wi_spin_fl.value = vars->z_offset;
    psmd->items[MI_BABYSTEP].item.wi_spin_fl.range = zoffset_fl_range;
    psmd->items[MI_BABYSTEP].item.wi_spin_fl.prt_format = zoffset_fl_format;
}

int screen_menu_tune_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    static float z_offs = 0;

    if (
        marlin_all_axes_homed() && marlin_all_axes_known() && (marlin_command() != MARLIN_CMD_G28) && (marlin_command() != MARLIN_CMD_G29) && (marlin_command() != MARLIN_CMD_M109) && (marlin_command() != MARLIN_CMD_M190)) {
        psmd->items[MI_FILAMENT].item.type &= ~WI_DISABLED;
    } else {
        psmd->items[MI_FILAMENT].item.type |= WI_DISABLED;
    }

    if (screen_menu_event(screen, window, event, param))
        return 1;

    if (event == WINDOW_EVENT_CHANGING) {
        switch ((int)param) {
        /*case MI_SPEED:
			marlin_set_print_speed((uint8_t)(psmd->items[MI_SPEED].item.value / 1000));
			break;
		case MI_NOZZLE:
			marlin_set_target_nozzle((float)(psmd->items[MI_NOZZLE].item.value) / 1000);
			break;
		case MI_HEATBED:
			marlin_set_target_bed((float)(psmd->items[MI_HEATBED].item.value) / 1000);
			break;
		case MI_PRINTFAN:
			marlin_set_fan_speed((uint8_t)(psmd->items[MI_PRINTFAN].item.value / 1000));
			break;
		case MI_FLOWFACT:
			marlin_set_flow_factor((uint16_t)(psmd->items[MI_FLOWFACT].item.value / 1000));
			break;*/
        case MI_BABYSTEP:
            marlin_do_babysteps_Z(psmd->items[MI_BABYSTEP].item.wi_spin_fl.value - z_offs);
            z_offs = psmd->items[MI_BABYSTEP].item.wi_spin_fl.value;
            break;
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case MI_SPEED:
            marlin_set_print_speed((uint16_t)(psmd->items[MI_SPEED].item.wi_spin.value / 1000));
            break;
        case MI_NOZZLE:
            marlin_set_target_nozzle((float)(psmd->items[MI_NOZZLE].item.wi_spin.value) / 1000);
            break;
        case MI_HEATBED:
            marlin_set_target_bed((float)(psmd->items[MI_HEATBED].item.wi_spin.value) / 1000);
            break;
        case MI_PRINTFAN:
            marlin_set_fan_speed((uint8_t)(psmd->items[MI_PRINTFAN].item.wi_spin.value / 1000));
            break;
        case MI_FLOWFACT:
            marlin_set_flow_factor((uint16_t)(psmd->items[MI_FLOWFACT].item.wi_spin.value / 1000));
            break;
        case MI_BABYSTEP:
            marlin_set_z_offset(psmd->items[MI_BABYSTEP].item.wi_spin_fl.value);
            eeprom_set_var(EEVAR_ZOFFSET, marlin_get_var(MARLIN_VAR_Z_OFFSET));
            break;
        }
    } else if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_FILAMENT:
            //screen_menu_tune_chanege_filament(screen);
            //todo this should not be here
            if (!(psmd->items[MI_FILAMENT].item.type & WI_DISABLED)) {
                marlin_gcode_push_front("M600");
            }
            break;
        case MI_BABYSTEP:
            z_offs = psmd->items[MI_BABYSTEP].item.wi_spin_fl.value;
            break;
        case MI_MESSAGES:
            screen_open(psmd->items[(int)param].screen->id);
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
            psmd->items[MI_SPEED].item.wi_spin.value = vars->print_speed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_NOZZLE) {
            psmd->items[MI_NOZZLE].item.wi_spin.value = vars->target_nozzle * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_HEATBED) {
            psmd->items[MI_HEATBED].item.wi_spin.value = vars->target_bed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_PRINTFAN) {
            psmd->items[MI_PRINTFAN].item.wi_spin.value = vars->fan_speed * 1000;
            psmd->menu.win.flg |= WINDOW_FLG_INVALID;
        }
        if (!editing || index != MI_FLOWFACT) {
            psmd->items[MI_FLOWFACT].item.wi_spin.value = vars->flow_factor * 1000;
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

extern "C" {
const screen_t *pscreen_menu_tune = &screen_menu_tune;
}
