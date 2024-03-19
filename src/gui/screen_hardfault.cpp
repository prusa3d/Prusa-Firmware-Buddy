#include "screen_hardfault.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod_gui.hpp"
#include <find_error.hpp>
#include "sound.hpp"

using namespace bsod_details;

ScreenHardfault::ScreenHardfault()
    : AddSuperWindow<ScreenBlueError>() {
    ///@note No translations on blue screens.

    static const char txt_header[] = "HARDFAULT";
    header.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)txt_header));

    // Show reason of hardfault as title
    title.SetText(string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(get_hardfault_reason())));

    char *buffer;

    // Show last known task, core registers and stack as description
    buffer = txt_err_description;
    get_stack(buffer, get_regs(buffer, get_task_name(buffer, std::size(txt_err_description))));
    description.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_description)));
}
