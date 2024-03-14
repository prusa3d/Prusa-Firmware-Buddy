/**
 * @file led_lcd_cs_selector.cpp
 * @author Radek Vana
 * @date 2021-08-23
 */

#include "led_lcd_cs_selector.hpp"
#include "main.h"
#include "hwio_pindef.h"
#include "gui_time.hpp" //gui::GetTick()
#include <ccm_thread.hpp>
#include "option/has_side_leds.h"

using namespace buddy::hw;

// this function is not in header, i prefer one extern over adding it to
// ili9488.hpp and including it
extern void ili9488_spi_wr_bytes(const uint8_t *pb, uint16_t size);
extern void ili9488_cmd_nop(void);

void LED_LCD_SPI_switcher::SelectLeds() {
    saved_prescaler = spi->Init.BaudRatePrescaler;
    // LEDs use MHz10_5, reconfigure prescaler accordingly
    const auto new_prescaler = SPI_BAUDRATEPRESCALER_8;

    // prescaler is already OK, do not do anyhting
    if (saved_prescaler == new_prescaler) {
        return;
    }

    if (HAL_SPI_DeInit(spi) != HAL_OK) {
        Error_Handler();
    }

    spi->Init.BaudRatePrescaler = new_prescaler;
    if (HAL_SPI_Init(spi) != HAL_OK) {
        Error_Handler();
    }
}

void LED_LCD_SPI_switcher::Restore() {
    // prescaler is already OK, do not do anyhting
    if (spi->Init.BaudRatePrescaler == saved_prescaler) {
        return;
    }

    if (HAL_SPI_DeInit(spi) != HAL_OK) {
        Error_Handler();
    }
    spi->Init.BaudRatePrescaler = saved_prescaler;
    if (HAL_SPI_Init(spi) != HAL_OK) {
        Error_Handler();
    }
}

void LED_LCD_SPI_switcher::WrBytes(uint8_t *pb, uint16_t size) {
    if (spi == &SPI_HANDLE_FOR(lcd)) {
        ili9488_spi_wr_bytes(pb, size);
    } else {
        HAL_SPI_Abort(spi);
        assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(pb)));
        HAL_SPI_Transmit_DMA(spi, pb, size);
        // wait for transmission complete
        while (HAL_SPI_GetState(spi) == HAL_SPI_STATE_BUSY_TX) {
            osDelay(1);
        }
    }
}

void GuiLedsWriter::write(uint8_t *pb, uint16_t size) {
    LED_LCD_SPI_switcher writer(&SPI_HANDLE_FOR(lcd));

    writer.SelectLeds();
    ili9488_cmd_nop();

    // switch multiplex to send data to side led strip
    displayCs.set();

    writer.WrBytes(pb, size);
    writer.Restore();

    // switch multiplex back
    displayCs.reset();
}

#if HAS_SIDE_LEDS()
void SideStripWriter::write(uint8_t *pb, uint16_t size) {
    LED_LCD_SPI_switcher writer(hw_get_spi_side_strip());

    /// true when SPI to control LEDs is shared with LCD
    const bool spi_shared_with_lcd = (writer.spi == &SPI_HANDLE_FOR(lcd));

    writer.SelectLeds();
    ili9488_cmd_nop();

    // switch multiplex to send data to side led strip
    if (spi_shared_with_lcd) {
        displayCs.reset();
        SideLed_LcdSelector->set();
    }

    writer.WrBytes(pb, size);
    writer.Restore();

    // switch multiplex back
    if (spi_shared_with_lcd) {
        displayCs.reset();
        SideLed_LcdSelector->reset();
    }
}
#endif
