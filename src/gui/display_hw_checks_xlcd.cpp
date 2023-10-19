/**
 * @file display_hw_checks_xlcd.cpp
 */
#include "display.h"
#include "display_hw_checks.hpp"
#include "touch_get.hpp"
#include "touch_dependency.hpp"
#include "screen_home.hpp"
#include "ScreenHandler.hpp"
#include <option/has_touch.h>
#include <device/peripherals.h>
#include <option/has_leds.h>

LOG_COMPONENT_REF(GUI);

static int touch_read_err_total = 0;
static constexpr uint32_t touch_err_cnt_to_be_reset = 4; // how many times must touch read fail to be considered an error
static constexpr uint32_t disable_touch_after_n_resets_in_sequence = 4; // if 4 resets did not help, just disable it, home screen will show msgbox
static uint32_t touch_read_err = 0; // errors in row

namespace {
void reinit_lcd_and_redraw() {
    display::CompleteReinitLCD();
    display::Init();
    Screens::Access()->SetDisplayReinitialized();
}

void check_lcd() {
    bool do_reset = false;
#if HAS_LEDS()
    leds::ForceRefresh(4);
#endif

    uint8_t data_buff[ILI9488_MAX_COMMAND_READ_LENGHT] = { 0x00 };

    display::ReadMADCTL(data_buff);

    if ((data_buff[1] != 0xE0 && data_buff[1] != 0xF0 && data_buff[1] != 0xF8)) {
        do_reset = true;
    }

    if (option::has_touch && touch::is_enabled() && screen_home_data_t::EverBeenOpened() && (!touch::does_read_work())) {
        ++touch_read_err;
        ++touch_read_err_total;
    } else {
        touch_read_err = 0;
    }

    if (touch_read_err) {
        if (touch_read_err > (disable_touch_after_n_resets_in_sequence * touch_err_cnt_to_be_reset)) {
            // reached touch disable limit
            touch::disable();
            log_error(GUI, "Cannot communicate with touch driver");
            touch_read_err = 0;
            screen_home_data_t::SetTouchBrokenDuringRun();
        } else if ((touch_read_err % touch_err_cnt_to_be_reset) == 0) {
            // try to reset the touch
            do_reset = true;
        } else {
            // we did have a touch error, let us try to reconfigure I2C
            log_warning(GUI, "Applying workaround to fix I2C of touch");
            I2C_INIT(touch);
        }
    }

    if (do_reset) {
        reinit_lcd_and_redraw();
    }
}
} // anonymous namespace

void lcd::communication_check() {
    const uint32_t min_check_period_ms = 2048; // both touch and display
    static uint32_t last_touch_check_ms = gui::GetTick_IgnoreTickLoop(); // sync with loop time would be unwanted

    uint32_t now = gui::GetTick_IgnoreTickLoop();
    if ((now - last_touch_check_ms) >= min_check_period_ms) {
        last_touch_check_ms = now;
        check_lcd();
    }
}

int touch::get_touch_read_err_total() {
    return touch_read_err_total;
}
