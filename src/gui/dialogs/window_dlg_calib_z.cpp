#include "DialogG162.hpp"
#include "DialogHandler.hpp"
#include "window_dlg_calib_z.hpp"
#include "window_dlg_wait.hpp"
#include "ScreenHandler.hpp"

void gui_marlin_G28_or_G29_in_progress() {
    uint32_t cmd = marlin_command();
    if ((cmd != MARLIN_CMD_G28) && (cmd != MARLIN_CMD_G29))
        Screens::Access()->Close();
}

dlg_result_t gui_dlg_calib_z(void) {
    marlin_event_clr(MARLIN_EVT_CommandBegin);
    marlin_gcode("G28");
    while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
        marlin_client_loop();
    gui_dlg_wait(gui_marlin_G28_or_G29_in_progress); // from beginning of the scope to here it's deprecated

    marlin_gcode("G162 Z");
    // create blocking dialog
    DialogHandler::Access().WaitUntilClosed(ClientFSM::G162, 0);
    return dlg_result_t::ok;
}
