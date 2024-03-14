/**
 * @file display_hw_checks_MINI.cpp
 */
#include "display.h"
#include "display_hw_checks.hpp"
#include "touch_dependency.hpp"
#include "ScreenHandler.hpp"

namespace {
void reinit_lcd_and_redraw() {
    display::CompleteReinitLCD();
    display::Init();
    Screens::Access()->SetDisplayReinitialized();
}

void check_lcd() {
    uint8_t data_buff[ST7789V_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    display::ReadMADCTL(data_buff);

    if ((data_buff[1] != 0xE0 && data_buff[1] != 0xF0 && data_buff[1] != 0xF8)) {
        reinit_lcd_and_redraw();
    }
}
} // anonymous namespace

void lcd::communication_check() {
    const uint32_t min_check_period_ms = 2048;
    static uint32_t last_touch_check_ms = gui::GetTick_IgnoreTickLoop(); // sync with loop time would be unwanted

    uint32_t now = gui::GetTick_IgnoreTickLoop();
    if ((now - last_touch_check_ms) >= min_check_period_ms) {
        last_touch_check_ms = now;
        check_lcd();
    }
}

int touch::get_touch_read_err_total() {
    return 0;
}
