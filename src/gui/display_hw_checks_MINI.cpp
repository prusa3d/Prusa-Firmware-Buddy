/**
 * @file display_hw_checks_MINI.cpp
 */
#include "display.hpp"
#include "display_hw_checks.hpp"
#include "ScreenHandler.hpp"
#include <common/timing.h>

namespace {
void reinit_lcd_and_redraw() {
    display::complete_reinit();
    display::init();
    if (auto *screen = Screens::Access()->Get()) {
        screen->Invalidate();
    }
}

void check_lcd() {
    if (display::is_reset_required()) {
        reinit_lcd_and_redraw();
    }
}
} // anonymous namespace

void lcd::communication_check() {
    const uint32_t min_check_period_ms = 2048;
    static uint32_t last_touch_check_ms = ticks_ms();

    uint32_t now = ticks_ms();
    if ((now - last_touch_check_ms) >= min_check_period_ms) {
        last_touch_check_ms = now;
        check_lcd();
    }
}
