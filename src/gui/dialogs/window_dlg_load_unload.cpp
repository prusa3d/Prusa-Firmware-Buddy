// window_dlg_load_unload.c

#include "window_dlg_load_unload.h"
#include "marlin_client.h"
#include "window_dlg_preheat.h"
#include "filament.h"
#include "../lang/i18n.h"

dlg_result_t gui_dlg_load(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    FILAMENT_t filament = gui_dlg_preheat(NULL);
    if (filament == FILAMENT_NONE) {
        return DLG_ABORTED;
    }
    marlin_gcode_printf("M701 S\"%s\"", filaments[filament].name);
    return DLG_OK;
}

dlg_result_t gui_dlg_purge(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    FILAMENT_t filament = gui_dlg_preheat_autoselect_if_able(NULL);
    if (filament == FILAMENT_NONE)
        return DLG_ABORTED;

    marlin_gcode("M701 L0");
    return DLG_OK;
}

dlg_result_t gui_dlg_load_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    FILAMENT_t filament = gui_dlg_preheat_forced(_("PREHEAT for LOAD"));
    if (filament == FILAMENT_NONE) {
        return DLG_ABORTED; //DLG_ABORTED should not happen
    }
    marlin_gcode_printf("M701 S\"%s\"", filaments[filament].name);
    return DLG_OK;
}

dlg_result_t gui_dlg_unload(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able(NULL) == FILAMENT_NONE)
        return DLG_ABORTED; //user can choose "RETURN"
    marlin_gcode("M702");
    return DLG_OK;
}

dlg_result_t gui_dlg_unload_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able_forced(_("PREHEAT for UNLOAD")) == FILAMENT_NONE)
        return DLG_ABORTED; //LD_ABORTED should not happen
    marlin_gcode("M702");
    return DLG_OK;
}
