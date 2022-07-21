#include "DialogHandler.hpp"
#include "window_dlg_calib_z.hpp"
#include "window_dlg_wait.hpp"
#include "ScreenHandler.hpp"

void gui_marlin_G28_or_G29_in_progress() {
    uint32_t cmd = marlin_command();
    if ((cmd != MARLIN_CMD_G28) && (cmd != MARLIN_CMD_G29))
        Screens::Access()->Close();
}
