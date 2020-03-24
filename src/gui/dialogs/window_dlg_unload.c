// window_dlg_unload.c

#include "window_dlg_unload.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "filament.h"

dlg_result_t gui_dlg_unload(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able(NULL) < 1)
        return DLG_ABORTED; //user can choose "RETURN"
    marlin_gcode("M702");
    set_filament(FILAMENT_NONE); //todo this must be in marlin
    return DLG_OK;
}

dlg_result_t gui_dlg_unload_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able_forced("PREHEAT for UNLOAD") < 0)
        return DLG_ABORTED; //LD_ABORTED should not happen
    marlin_gcode("M702");
    set_filament(FILAMENT_NONE); //todo this must be in marlin
    return DLG_OK;
}
