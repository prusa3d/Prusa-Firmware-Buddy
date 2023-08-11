#include "screen_bsod.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod_gui.hpp"
#include <crash_dump/dump.hpp>
#include "sound.hpp"

using namespace bsod_details;

ScreenBsod::ScreenBsod()
    : AddSuperWindow<ScreenBlueError>() {
    ///@note No translations on blue screens.

    static const char txt_header[] = "BSOD";
    header.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)txt_header));

    char *buffer = txt_err_description;
    size_t buffer_remaining = std::size(txt_err_description);

    // Get BSOD message
    if (crash_dump::message_get_type() == crash_dump::MsgType::BSOD) {
        if (crash_dump::load_message(buffer, buffer_remaining, txt_err_title, std::size(txt_err_title))) {
            title.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_title)));

            // Update buffer
            const size_t len = std::strlen(buffer);
            if (len < buffer_remaining - 1) { // Will fit buffer with -1 for newline
                buffer_remaining -= len;
                buffer += len;

                // Add newline
                *buffer = '\n';
                ++buffer;
                --buffer_remaining;
                *buffer = '\0';
            } else {
                buffer_remaining = 0;
                buffer = nullptr;
            }
        }
    }

    // Show last known task, core registers and stack as description
    get_stack(buffer, get_regs(buffer, get_task_name(buffer, buffer_remaining)));
    description.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_description)));
}
