// screen_temperror.cpp

#include "screen_temperror.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "dump.h"
#include "sound.hpp"

screen_temperror_data_t::screen_temperror_data_t()
    : window_frame_t()
    , text(this, Rect16(0, 0, 0, 0), is_multiline::yes)
    , exit(this, Rect16(0, 0, 0, 0), is_multiline::no, is_closed_on_click_t::no) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

void screen_temperror_data_t::draw() {
    //window_frame_t::draw();
    temp_error_code(dump_in_xflash_get_code());
    /// Play after draw not to collide with beep at printer start
    Sound_Play(eSOUND_TYPE::CriticalAlert);
}
