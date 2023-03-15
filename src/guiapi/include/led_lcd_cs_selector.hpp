/**
 * @file led_lcd_cs_selector.hpp
 * @file gui_leds.hpp
 * @author Radek Vana
 * @brief inherit folowing object to write on LEDs
 * @date 2021-08-01
 */
#pragma once

#include <cstdint>
class LED_LCD_CS_selector {
public:
    // add more speed
    enum class speed { MHz42,
        MHz21,
        MHz10_5 };
    LED_LCD_CS_selector(speed spd);
    ~LED_LCD_CS_selector();

    static void WrBytes(uint8_t *pb, uint16_t size);

private:
    static uint32_t last_tick;
    uint32_t saved_prescaler;
};
