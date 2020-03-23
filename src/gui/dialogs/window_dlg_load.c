// window_dlg_load.c
// does not have header
// window_dlg_loadunload.h used instead

#include "window_dlg_load.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "gui.h" //gui_defaults
#include "button_draw.h"
#include "filament.h"
#include "filament_sensor.h"

static dlg_result_t _gui_dlg_load(void) {
    marlin_gcode("M701");
    dlg_result_t ret; //todo get info from marlin thread
    if (ret != DLG_OK)
        set_filament(FILAMENT_NONE); //todo this must be in marlin
    return ret;
}

dlg_result_t gui_dlg_load(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat(NULL) < 1)
        return DLG_ABORTED; //0 is return
    return _gui_dlg_load();
}

dlg_result_t gui_dlg_load_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_forced("PREHEAT for LOAD") < 0)
        return DLG_ABORTED; //DLG_ABORTED should not happen
    return _gui_dlg_load();
}
