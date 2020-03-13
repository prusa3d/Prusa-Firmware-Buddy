// window_dlg_change.c

#include "window_dlg_change.h"
#include "gui.h"
#include "window_dlg_statemachine.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_msgbox.h"
#include "window_dlg_preheat.h"
#include "button_draw.h"
#include "filament_sensor.h"

static dlg_result_t _gui_dlg_change(void) {
    uint8_t st = fs_get__send_M600_on__and_disable(); //remember fs state
    marlin_gcode("M600 Z0");
    dlg_result_t ret;             //todo get info from marlin thread
    fs_restore__send_M600_on(st); //restore fs state
    return ret;
}

dlg_result_t gui_dlg_change(void) {
    return _gui_dlg_change();
}
