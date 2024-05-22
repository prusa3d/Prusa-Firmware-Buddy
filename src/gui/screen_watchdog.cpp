// screen_watchdog.cpp

#include "screen_watchdog.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod_gui.hpp"
#include "sound.hpp"

using namespace bsod_details;

ScreenWatchdog::ScreenWatchdog()
    : AddSuperWindow<ScreenBlueError>() {
    ///@note No translations on blue screens.

    static const char txt_header[] = "WATCHDOG RESET";
    header.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)txt_header));

    char *buffer;

    // Show last known task name as title
    buffer = txt_err_title;
    get_task_name(buffer, std::size(txt_err_title));
    title.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_title)));

    // Show core registers and stack as description
    buffer = txt_err_description;
    get_stack(buffer, get_regs(buffer, std::size(txt_err_description)));
    description.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_description)));
}
