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
#include <common/spi_baud_rate_prescaler_guard.hpp>

using namespace buddy::hw;

// this function is not in header, i prefer one extern over adding it to
// ili9488.hpp and including it
extern void ili9488_spi_wr_bytes(const uint8_t *pb, uint16_t size);
extern void ili9488_cmd_nop(void);

void GuiLedsWriter::write(uint8_t *pb, uint16_t size) {
    SPI_HandleTypeDef *hspi = &SPI_HANDLE_FOR(lcd);

    // LEDs use MHz10_5, reconfigure prescaler accordingly
    SPIBaudRatePrescalerGuard guard { hspi, SPI_BAUDRATEPRESCALER_8 };

    ili9488_cmd_nop();

    // switch multiplex to send data to side led strip
    displayCs.set();

    ili9488_spi_wr_bytes(pb, size);

    // switch multiplex back
    displayCs.reset();
}

#if HAS_SIDE_LEDS()
void SideStripWriter::write(uint8_t *pb, uint16_t size) {
    SPI_HandleTypeDef *hspi = hw_get_spi_side_strip();

    // LEDs use MHz10_5, reconfigure prescaler accordingly
    SPIBaudRatePrescalerGuard guard { hspi, SPI_BAUDRATEPRESCALER_8 };

    /// true when SPI to control LEDs is shared with LCD
    const bool spi_shared_with_lcd = (hspi == &SPI_HANDLE_FOR(lcd));

    ili9488_cmd_nop();

    if (spi_shared_with_lcd) {
        // switch multiplex to send data to side led strip
        displayCs.set();
        SideLed_LcdSelector->set();

        // send data
        ili9488_spi_wr_bytes(pb, size);

        // switch multiplex back
        displayCs.reset();
        SideLed_LcdSelector->reset();
    } else {
        HAL_SPI_Abort(hspi);
        assert(can_be_used_by_dma(pb));
        HAL_SPI_Transmit_DMA(hspi, pb, size);
        // wait for transmission complete
        while (HAL_SPI_GetState(hspi) == HAL_SPI_STATE_BUSY_TX) {
            osDelay(1);
        }
    }
}
#endif
