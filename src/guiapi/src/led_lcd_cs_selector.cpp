/**
 * @file led_lcd_cs_selector.cpp
 * @author Radek Vana
 * @date 2021-08-23
 */

#include "led_lcd_cs_selector.hpp"
#include <buddy/main.h>
#include "hwio_pindef.h"
#include "gui_time.hpp" //gui::GetTick()
#include <buddy/ccm_thread.hpp>
#include "option/has_side_leds.h"
#include <common/spi_baud_rate_prescaler_guard.hpp>
#include <option/has_burst_stepping.h>

using namespace buddy::hw;

// target selection matrix on boards with board_bom_id >= 9
// SideLed_LcdSelector is marked as A77 in the schematic
// LED strips only use MOSI signal (CLK is ignored) so make sure MOSI is 0 when
// switching to them
//
// | CS  | A77 |  LCD  | FRONT |  SIDE |
// |     |     |       |  LED  |  LED  |
// | --- | --- | ----- | ----- | ----- |
// |  0  |  0  |   X   |       |       |
// |  0  |  1  |   X   |       |   X   | <- don't use
// |  1  |  0  |       |   X   |       |
// |  1  |  1  |       |       |   X   |

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

#if BOARD_IS_XLBUDDY() || PRINTER_IS_PRUSA_iX()
void SideStripWriter::write(uint8_t *pb, uint16_t size) {
    SPI_HandleTypeDef *hspi = hw_get_spi_side_strip();

    /// true when SPI to control LEDs is shared with LCD
    const bool spi_shared_with_lcd = (hspi == &SPI_HANDLE_FOR(lcd));

    if (spi_shared_with_lcd) {
        // LEDs use MHz10_5, reconfigure prescaler accordingly
        SPIBaudRatePrescalerGuard guard { hspi, SPI_BAUDRATEPRESCALER_8 };

        // This nop is sent because it ends the transmission with the MO pin on LOW.
        // If we chip select the neopixel with the data set to high, it registers it as an input and screws up the communication.
        // The correct solution would possibly be to pull the pin to LOW after the chip select for at least the prescribed RESET duration.¨
        // BFW-5067
        ili9488_cmd_nop();

        // switch multiplex to send data to side led strip
        displayCs.set();

    #if PRINTER_IS_PRUSA_XL()
        // On XL the sideleds are optional (depending on BOM), ensure the SideLed_LcdSelector is
        // actually available on the printer before using it
        if (SideLed_LcdSelector) {
            SideLed_LcdSelector->set();
        }
    #endif

        // send data
        ili9488_spi_wr_bytes(pb, size);

        // switch multiplex back
        displayCs.reset();

    #if PRINTER_IS_PRUSA_XL()
        if (SideLed_LcdSelector) {
            SideLed_LcdSelector->reset();
        }
    #endif
    } else {
        HAL_SPI_Abort(hspi);
        assert(can_be_used_by_dma(pb));
    #if HAS_BURST_STEPPING()
        HAL_SPI_Transmit_IT(hspi, pb, size);
    #else
        HAL_SPI_Transmit_DMA(hspi, pb, size);
    #endif
        // wait for transmission complete
        while (HAL_SPI_GetState(hspi) == HAL_SPI_STATE_BUSY_TX) {
            osDelay(1);
        }
    }
}
#endif
