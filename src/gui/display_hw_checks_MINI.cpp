/**
 * @file display_hw_checks_MINI.cpp
 */
#include "display.h"
#include "display_hw_checks.hpp"
#include "ScreenHandler.hpp"

namespace {
void reinit_lcd_and_redraw() {
    display::CompleteReinitLCD();
    display::Init();
    Screens::Access()->SetDisplayReinitialized();
}

void check_lcd() {
    if (display::IsResetRequired()) {
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
