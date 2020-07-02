#include "DialogG162.hpp"
#include "DialogHandler.hpp"
#include "window_dlg_calib_z.hpp"

dlg_result_t gui_dlg_calib_z(void) {

    // create blocking dialog
    DialogHandler::WaitUntilClosed(ClientFSM::G162, 0);
    return DLG_OK;
}
