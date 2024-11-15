/**
 * @file gui_leds.cpp
 * @author Radek Vana
 * @date 2021-08-01
 */

#include "gui_leds.hpp"
#include "display.hpp"
#include "led_lcd_cs_selector.hpp"
#include <algorithm>
#include "neopixel.hpp"
#include <option/has_side_leds.h>
#include "ili9488.hpp"
#include "led_animations/animator.hpp"
#include <device/peripherals.h>
#include <config_store/store_instance.hpp>

#if HAS_SIDE_LEDS()
    #include "leds/side_strip_control.hpp"
#endif

using namespace leds;

using Neopixels = neopixel::SPI_10M5Hz<4, GuiLedsWriter::write>;

Neopixels &getNeopixels() {
    static Neopixels ret;
    return ret;
}

void leds::Init() {
    // Turn on LCD backlight
    // TODO move SetBrightness to display
    leds::SetBrightness(100);
    leds::TickLoop();

#if HAS_SIDE_LEDS()
    leds::side_strip_control.SetEnable(config_store().side_leds_enabled.get());
    leds::side_strip_control.set_dimming_enabled(config_store().side_leds_dimming_enabled.get());
#endif
}
void leds::ForceRefresh(size_t cnt) {
    getNeopixels().ForceRefresh(cnt);
}

void leds::TickLoop() {
    getNeopixels().Tick();

#if HAS_SIDE_LEDS()
    leds::side_strip_control.Tick();
#endif
}

void leds::SetNth(ColorRGBW clr, leds::index n) {
    if (n >= index::count_) {
        for (size_t i = 0; i < Count; ++i) {
            SetNth(clr, leds::index(i));
        }
    } else {
        getNeopixels().Set(clr.data, size_t(n));
    }
}

// backlight is on WS2811 (RGB)
// but color structure is meant for WS2812 (GRB)
// so to set RED channel on WS2811 have to set GREEN
void leds::SetBrightness(unsigned percent) {
    percent = std::min(100u, percent);
    percent *= 255;
    percent /= 100;

    SetNth(ColorRGBW(0, percent, 0), index::backlight);
}

extern osThreadId displayTaskHandle;

void leds::enter_power_panic() {
    // normally, GUI is accessing LCD & LEDs SPIs, but this is called from task handling power panic, and we need to turn leds off quickly. So we'll steal display's its SPI in a hacky way.

    // 1. configure led animations to off, in case gui would want to write them again, this lock mutex, so has to be done before suspending display task
#if HAS_SIDE_LEDS()
    leds::side_strip_control.PanicOff();
#endif
    Animator_LCD_leds().panic_off();

    // 2. Temporary suspend display task, so that it doesn't interfere with turning off leds
    osThreadSuspend(displayTaskHandle);

    // 3. Safe mode for display SPI is enabled (that disables DMA transfers and writes directly to SPI)
    display::enable_safe_mode();

    // 4. Reinitialize SPI, so that we terminate any ongoing transfers to display or leds
    // 5. turn off actual leds
#if HAS_SIDE_LEDS()
    hw_init_spi_side_leds();
    side_strip.SetColor(ColorRGBW());
    side_strip.Update();
#endif
    SPI_INIT(lcd);
    leds::SetNth(ColorRGBW(), leds::index::count_);
    leds::ForceRefresh(size_t(leds::index::count_));
    getNeopixels().Tick();

    // 5. reenable display task
    osThreadResume(displayTaskHandle);
}
