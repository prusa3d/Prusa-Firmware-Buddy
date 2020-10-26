// screen_temperror.cpp

#include "screen_temperror.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "dump.h"
#include "sound.hpp"

screen_temperror_data_t::screen_temperror_data_t()
    : screen_reset_error_data_t() {}

void screen_temperror_data_t::draw() {
    temp_error_code(dump_in_xflash_get_code());
    /// Play after draw not to collide with beep at printer start
    start_sound();
}
