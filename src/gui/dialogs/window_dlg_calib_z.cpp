#include "DialogG162.hpp"
#include "DialogHandler.hpp"
#include "window_dlg_calib_z.hpp"

dlg_result_t gui_dlg_calib_z(void) {

    marlin_gcode("G162 Z");
    DialogHandler::WaitUntilClosed(ClientFSM::G162, 0);
    return DLG_OK;
}
