/**
 * @file gui_leds.cpp
 * @author Radek Vana
 * @date 2021-08-01
 */

#include "gui_leds.hpp"
#include "led_lcd_cs_selector.hpp"
#include <algorithm>
#include "neopixel.hpp"
#include <option/has_side_leds.h>
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
    bool ena = config_store().side_leds_enabled.get();
    leds::side_strip_control.SetEnable(ena);
#endif
}
void leds::ForceRefresh(size_t cnt) {
    getNeopixels().ForceRefresh(cnt);
}

void leds::TickLoop() {
    if (getNeopixels().LedsToRewrite() > 0 || getNeopixels().GetForceRefresh()) {
        getNeopixels().Send();
    }
#if HAS_SIDE_LEDS()
    leds::side_strip_control.Tick();
#endif
}

void leds::SetNth(Color clr, leds::index n) {
    if (n == index::count_) {
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

    SetNth(Color(0, percent, 0), index::backlight);
}
