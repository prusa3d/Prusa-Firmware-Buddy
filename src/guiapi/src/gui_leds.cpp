/**
 * @file gui_leds.cpp
 * @author Radek Vana
 * @date 2021-08-01
 */

#include "gui_leds.hpp"
#include "led_lcd_cs_selector.hpp"
#include <algorithm>
#include "neopixel.hpp"

using namespace leds;

static size_t loop_index = 0;
static size_t write_count = 0;

using Neopixels = neopixel::SPI_10M5Hz<4, LED_LCD_CS_selector::WrBytes>;

Neopixels &getNeopixels() {
    static Neopixels ret;
    return ret;
}

class Writer : public LED_LCD_CS_selector {

public:
    Writer()
        : LED_LCD_CS_selector(speed::MHz10_5) {}
};

void leds::ForceRefresh(size_t cnt) {
    getNeopixels().ForceRefresh(cnt);
}
void leds::TickLoop() {
    if (getNeopixels().LedsToRewrite() > 0 || getNeopixels().GetForceRefresh()) {
        Writer wr;
        getNeopixels().Send();
        ++write_count;
    }
    ++loop_index;
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

//backlight is on WS2811 (RGB)
//but color structure is meant for WS2812 (GRB)
//so to set RED channel on WS2811 have to set GREEN
void leds::SetBrightness(unsigned percent) {
    percent = std::min(100u, percent);
    percent *= 255;
    percent /= 100;

    SetNth(Color(0, percent, 0), index::backlight);
}
