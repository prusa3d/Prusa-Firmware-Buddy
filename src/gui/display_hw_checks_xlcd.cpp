/**
 * @file display_hw_checks_xlcd.cpp
 */
#include "display.h"
#include "display_hw_checks.hpp"
#include "ScreenHandler.hpp"
#include <option/has_touch.h>
#include <device/peripherals.h>

LOG_COMPONENT_REF(GUI);

#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif

namespace {
void reinit_lcd_and_redraw() {
    display::CompleteReinitLCD();
    display::Init();
    Screens::Access()->SetDisplayReinitialized();
}

void check_lcd() {
    // Determine if we should reset the LCD
    {
        bool do_reset = display::IsResetRequired();

#if HAS_TOUCH()
        if (touchscreen.is_enabled()) {
            touchscreen.perform_check();
            do_reset |= (touchscreen.required_recovery_action() == Touchscreen_GT911::RecoveryAction::restart_display);
        }
#endif

        if (do_reset) {
            reinit_lcd_and_redraw();
        }
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
