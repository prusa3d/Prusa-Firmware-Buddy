#pragma once

#include <cstdint>

/**
 * @brief Multiplexor, that switches one SPI from LCD to leds and back
 *
 */
class LED_LCD_SPI_switcher {
public:
    LED_LCD_SPI_switcher(SPI_HandleTypeDef *spi)
        : spi(spi) {}

    void SelectLeds();
    void Restore();
    void WrBytes(uint8_t *pb, uint16_t size);

    SPI_HandleTypeDef *const spi;

private:
    uint32_t saved_prescaler;
};

/**
 * @brief Writer to push data to Side LCD strip
 */
class SideStripWriter {
public:
    static void write(uint8_t *pb, uint16_t size);
};

/**
 * @brief Writer to push data to GUi LCD strip
 */
class GuiLedsWriter {
public:
    static void write(uint8_t *pb, uint16_t size);
};
