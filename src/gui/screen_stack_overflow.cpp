// screen_stack_overflow.cpp

#include "screen_stack_overflow.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod_gui.hpp"
#include "sound.hpp"
#include <crash_dump/dump.hpp>

using namespace bsod_details;

ScreenStackOverflow::ScreenStackOverflow()
    : AddSuperWindow<ScreenBlueError>() {
    ///@note No translations on blue screens.

    static const char txt_header[] = "STACK OVERFLOW";
    header.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)txt_header));

    char *buffer;

    // Get BSOD title, it has the offending task name
    if (crash_dump::message_get_type() == crash_dump::MsgType::STACK_OVF) {
        if (crash_dump::load_message(nullptr, 0, txt_err_title, std::size(txt_err_title))) {
            title.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_title)));
        }
    }

    // Show core registers and stack as description
    buffer = txt_err_description;
    get_stack(buffer, get_regs(buffer, std::size(txt_err_description)));
    description.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(txt_err_description)));
}
