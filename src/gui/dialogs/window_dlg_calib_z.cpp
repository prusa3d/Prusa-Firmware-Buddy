#include "window_dlg_calib_z.hpp"
#include "window_dlg_wait.hpp"
#include "ScreenHandler.hpp"
#include <marlin_client.hpp>

void gui_marlin_G28_or_G29_in_progress() {
    marlin_server::Cmd cmd = marlin_client::get_command();
    if ((cmd != marlin_server::Cmd::G28) && (cmd != marlin_server::Cmd::G29)) {
        Screens::Access()->Close();
    }
}
