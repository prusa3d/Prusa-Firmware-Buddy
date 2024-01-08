/**
 * @file display_hw_checks_xlcd.cpp
 */
#include "display.h"
#include "display_hw_checks.hpp"
#include "ScreenHandler.hpp"
#include <option/has_touch.h>
#include <device/peripherals.h>
#include <option/has_leds.h>

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
#if HAS_LEDS()
    leds::ForceRefresh(4);
#endif

    // Determine if we should reset the LCD
    {
        bool do_reset = false;

        {
            uint8_t data_buff[ILI9488_MAX_COMMAND_READ_LENGHT] = { 0x00 };
            display::ReadMADCTL(data_buff);

            if ((data_buff[1] != 0xE0 && data_buff[1] != 0xF0 && data_buff[1] != 0xF8)) {
                do_reset = true;
            }
        }

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
