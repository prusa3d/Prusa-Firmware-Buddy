#include "window_dlg_calib_z.hpp"
#include "window_dlg_wait.hpp"
#include "ScreenHandler.hpp"

void gui_marlin_G28_or_G29_in_progress() {
    marlin_server::Cmd cmd = marlin_client::get_command();
    if ((cmd != marlin_server::Cmd::G28) && (cmd != marlin_server::Cmd::G29)) {
        Screens::Access()->Close();
    }
}

dlg_result_t gui_dlg_calib_z(void) {
    marlin_client::event_clr(marlin_server::Event::CommandBegin);
    marlin_client::gcode("G162 Z");
    return dlg_result_t::ok;
}
