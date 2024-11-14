#include "ili9488.hpp"

#include <device/board.h>
#include <device/hal.h>
#include <span>
#include <string.h>
#include <stdlib.h>
#include "qoi_decoder.hpp"
#include <ccm_thread.hpp>
#include "cmath_ext.h"
#include <stdint.h>
#include "printers.h"
#include <common/spi_baud_rate_prescaler_guard.hpp>
#include "raster_opfn_c.h"
#include "hwio_pindef.h"
#include "cmsis_os.h"
#include "display_math_helper.h"

#include "hw_configuration.hpp"

#include <option/bootloader.h>
#include <option/has_touch.h>
#include <logging/log.hpp>

#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif

LOG_COMPONENT_REF(GUI);

#define ILI9488_FLG_DMA  0x08 // DMA enabled
#define ILI9488_FLG_SAFE 0x20 // SAFE mode (no DMA and safe delay)

struct ili9488_config_t {
    uint8_t flg; // flags (DMA, MISO)
    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
};

constexpr static uint8_t DEFAULT_MADCTL = 0xE0; // memory data access control (mirror XY)
constexpr static uint8_t DEFAULT_COLMOD = 0x66; // interface pixel format (6-6-6, hi-color)

static ili9488_config_t ili9488_config = {
    .flg = ILI9488_FLG_DMA, // flags (DMA, MISO)
    .gamma = 0, // gamma curve
    .brightness = 0, // brightness
    .is_inverted = 0, // inverted
    .control = 0, // default control reg value
};

// ili9488 commands
#define CMD_SLPIN     0x10
#define CMD_SLPOUT    0x11
#define CMD_INVOFF    0x20 // Display Inversion Off
#define CMD_INVON     0x21 // Display Inversion On
#define CMD_GAMMA_SET 0x26 // gamma set
#define CMD_DISPOFF   0x28
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_RASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_RAMRD     0x2E
#define CMD_MADCTL    0x36
// #define CMD_IDMOFF 		0x38//Idle Mode Off
// #define CMD_IDMON 		0x38//Idle Mode On
#define CMD_COLMOD  0x3A
#define CMD_RAMWRC  0x3C
#define CMD_WRDISBV 0x51 // Write Display Brightness
#define CMD_RDDISBV 0x52 // Read Display Brightness Value
#define CMD_WRCTRLD 0x53 // Write CTRL Display
//-Brightness Control Block - bit 5
//-Display Dimming			- bit 3
//-Backlight Control On/Off - bit 2
#define CMD_RDCTRLD   0x54 // Read CTRL Value Display
#define CMD_CABCCTRL2 0xC8

// ili9488 gamma
#define GAMMA_CURVE0 0x01
#define GAMMA_CURVE1 0x02
#define GAMMA_CURVE2 0x04
#define GAMMA_CURVE3 0x08

// ili9488 CTRL Display
static const uint8_t MASK_CTRLD_BCTRL = (0x01 << 5); // Brightness Control Block
static const uint8_t MASK_CTRLD_DD(0x01 << 3); // Display Dimming
static const uint8_t MASK_CTRLD_BL(0x01 << 2); // Backlight Control

const uint8_t CMD_MADCTLRD = 0x0B;
constexpr static uint8_t CMD_NOP = 0x00;

uint8_t ili9488_flg = 0; // flags

static constexpr uint8_t ILI9488_MAX_COMMAND_READ_LENGHT = 4;

namespace {
bool do_complete_lcd_reinit = false;
}

static bool reduce_display_baudrate = false;

osThreadId ili9488_task_handle = 0;

#define ILI9488_SIG_SPI_TX 0x0008
#define ILI9488_SIG_SPI_RX 0x0008

uint8_t ili9488_buff[ILI9488_COLS * 3 * ILI9488_BUFF_ROWS]; // 3 bytes for pixel color
bool ili9488_buff_borrowed = false; ///< True if buffer is borrowed by someone else

uint8_t *ili9488_borrow_buffer() {
    assert(!ili9488_buff_borrowed && "Already lent");
    assert(ili9488_task_handle == osThreadGetId() && "Must be called only from one task");
    ili9488_buff_borrowed = true;
    return ili9488_buff;
}

void ili9488_return_buffer() {
    assert(ili9488_buff_borrowed);
    ili9488_buff_borrowed = false;
}

size_t ili9488_buffer_size() {
    return sizeof(ili9488_buff);
}

/*some functions are in header - excluded from display_t struct*/
void ili9488_gamma_set_direct(uint8_t gamma_enu);
uint8_t ili9488_read_ctrl(void);
void ili9488_ctrl_set(uint8_t ctrl);

using namespace buddy::hw;

static void ili9488_set_cs(void) {
#if (BOARD_IS_BUDDY())
    displayCs.write(Pin::State::high);
#endif
}

static void ili9488_clr_cs(void) {
#if (BOARD_IS_BUDDY())
    displayCs.write(Pin::State::low);
#endif
}

static void ili9488_set_rs(void) {
    displayRs.write(Pin::State::high);
}

static void ili9488_clr_rs(void) {
    displayRs.write(Pin::State::low);
}

static void ili9488_set_rst(void) {
    displayRst.write(Pin::State::high);
}

static void ili9488_clr_rst(void) {
    displayRst.write(Pin::State::low);
}

static inline void ili9488_fill_ui16(uint16_t *p, uint16_t v, uint16_t c) {
    while (c--) {
        *(p++) = v;
    }
}

static void ili9488_fill_ui24(uint8_t *p, uint32_t v, int c) {
    while (c--) {
        p[0] = 0xFF & v;
        p[1] = 0xFF & (v >> 8);
        p[2] = 0xFF & (v >> 16);
        p += 3;
    }
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void ili9488_delay_ms(uint32_t ms) {
    if (is_interrupt() || (ili9488_flg & ILI9488_FLG_SAFE)) {
        volatile uint32_t temp;
        while (ms--) {
            do {
                temp = SysTick->CTRL;
            } while ((temp & 0x01) && !(temp & (1 << 16)));
        }
    } else {
        osDelay(ms);
    }
}

void ili9488_spi_wr_byte(uint8_t b) {
    HAL_SPI_Transmit(&SPI_HANDLE_FOR(lcd), &b, 1, HAL_MAX_DELAY);
}

void ili9488_spi_wr_bytes(const uint8_t *pb, uint16_t size) {
    if ((ili9488_flg & ILI9488_FLG_DMA) && !(ili9488_flg & ILI9488_FLG_SAFE) && (size > 4)) {
        osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_TX);
        osSignalWait(ILI9488_SIG_SPI_TX, osWaitForever);
        assert(can_be_used_by_dma(pb));
        HAL_SPI_Transmit_DMA(&SPI_HANDLE_FOR(lcd), const_cast<uint8_t *>(pb), size);
        osSignalWait(ILI9488_SIG_SPI_TX, osWaitForever);
    } else {
        HAL_SPI_Transmit(&SPI_HANDLE_FOR(lcd), const_cast<uint8_t *>(pb), size, HAL_MAX_DELAY);
    }
}

void ili9488_spi_rd_bytes(uint8_t *pb, uint16_t size) {
    // reading is more reliable at 20MHz
    SPIBaudRatePrescalerGuard guard { &SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_4 };

    HAL_SPI_Receive(&SPI_HANDLE_FOR(lcd), pb, size, HAL_MAX_DELAY);
}

void ili9488_cmd(uint8_t cmd, const uint8_t *pdata, uint16_t size) {
    // BFW-6328 Some displays possibly problematic with higher baudrate, reduce 40 -> 20 MHz
    SPIBaudRatePrescalerGuard _g(&SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_4, reduce_display_baudrate);

    ili9488_clr_cs(); // CS = L
    ili9488_clr_rs(); // RS = L
    ili9488_spi_wr_byte(cmd); // write command byte
    if (pdata && size) {
        ili9488_set_rs(); // RS = H
        ili9488_spi_wr_bytes(pdata, size); // write data bytes
    }
    ili9488_set_cs(); // CS = H
}

template <size_t SZ>
void ili9488_cmd_array(uint8_t cmd, const std::array<uint8_t, SZ> &arr) {
    ili9488_cmd(cmd, arr.data(), SZ);
}

void ili9488_cmd_no_data(uint8_t cmd) {
    ili9488_cmd(cmd, nullptr, 0);
}

void ili9488_cmd_1_data(uint8_t cmd, uint8_t data) {
    ili9488_cmd(cmd, &data, 1);
}

void ili9488_cmd_rd(uint8_t cmd, uint8_t *pdata) {
    // reading is even more reliable at 10MHz
    SPIBaudRatePrescalerGuard guard { &SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_8 };

    ili9488_clr_cs(); // CS = L
    ili9488_clr_rs(); // RS = L
    uint8_t data_to_write[ILI9488_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    data_to_write[0] = cmd;
    data_to_write[1] = 0x00;
    HAL_SPI_TransmitReceive(&SPI_HANDLE_FOR(lcd), data_to_write, pdata, ILI9488_MAX_COMMAND_READ_LENGHT, HAL_MAX_DELAY);
    ili9488_set_cs();
}

void ili9488_wr(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size)) {
        return; // null or empty data - return
    }

    // BFW-6328 Some displays possibly problematic with higher baudrate, reduce 40 -> 20 MHz
    SPIBaudRatePrescalerGuard _g(&SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_4, reduce_display_baudrate);

    ili9488_clr_cs(); // CS = L
    ili9488_set_rs(); // RS = H
    ili9488_spi_wr_bytes(pdata, size); // write data bytes
    ili9488_set_cs(); // CS = H
}

void ili9488_rd(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size)) {
        return; // null or empty data - return
    }
    // generate little pulse on displayCs, because ILI need change displayCs logic level
    displayCs.write(Pin::State::high);
    ili9488_delay_ms(1);
    displayCs.write(Pin::State::low);

    ili9488_clr_cs(); // CS = L
    ili9488_clr_rs(); // RS = L
    ili9488_spi_wr_byte(CMD_RAMRD); // write command byte
    ili9488_spi_wr_byte(0); // write dummy byte, datasheet p.122

    ili9488_spi_rd_bytes(pdata, size); // read data bytes
    ili9488_set_cs(); // CS = H

    // generate little pulse on displayCs, because ILI need change displayCs logic level
    displayCs.write(Pin::State::high);
    ili9488_delay_ms(1);
    displayCs.write(Pin::State::low);
}

void ili9488_cmd_slpout(void) {
    ili9488_cmd(CMD_SLPOUT, 0, 0);
}

void ili9488_cmd_madctl(uint8_t madctl) {
    ili9488_cmd(CMD_MADCTL, &madctl, 1);
}

void ili9488_cmd_colmod(uint8_t colmod) {
    ili9488_cmd(CMD_COLMOD, &colmod, 1);
}

void ili9488_cmd_dispon(void) {
    ili9488_cmd(CMD_DISPON, 0, 0);
}

void ili9488_cmd_dispoff(void) {
    ili9488_cmd(CMD_DISPOFF, 0, 0);
}

void ili9488_cmd_caset(uint16_t x, uint16_t cx) {
    uint8_t data[4] = { static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x & 0xff), static_cast<uint8_t>(cx >> 8), static_cast<uint8_t>(cx & 0xff) };
    ili9488_cmd(CMD_CASET, data, 4);
}

void ili9488_cmd_raset(uint16_t y, uint16_t cy) {
    uint8_t data[4] = { static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y & 0xff), static_cast<uint8_t>(cy >> 8), static_cast<uint8_t>(cy & 0xff) };
    ili9488_cmd(CMD_RASET, data, 4);
}

void ili9488_cmd_ramwr(uint8_t *pdata, uint16_t size) {
    ili9488_cmd(CMD_RAMWR, pdata, size);
}

void ili9488_cmd_ramrd(uint8_t *pdata, uint16_t size) {
    ili9488_rd(pdata, size);
}

bool ili9488_is_reset_required() {
    // REMOVEME: This is a bit of hack to reduce config_store locks.
    // This function is called in lcd::communication_check every 2 s.
    reduce_display_baudrate = config_store().reduce_display_baudrate.get();

    uint8_t pdata[ILI9488_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    ili9488_cmd_rd(CMD_MADCTLRD, pdata);
    if ((pdata[1] != 0xE0 && pdata[1] != 0xF0 && pdata[1] != 0xF8)) {
        return true;
    }
    return false;
}

/*void ili9488_test_miso(void)
{
//	uint16_t data_out[8] = {CLR565_WHITE, CLR565_WHITE, CLR565_RED, CLR565_RED, CLR565_GREEN, CLR565_GREEN, CLR565_BLUE, CLR565_BLUE};
        uint8_t data_out[16] = {0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t data_in[32];
        memset(data_in, 0, sizeof(data_in));
        ili9488_clr_cs();
        ili9488_cmd_caset(0, ILI9488_COLS - 1);
        ili9488_cmd_raset(0, ILI9488_ROWS - 1);
        ili9488_cmd_ramwr((uint8_t*)data_out, 16);
        ili9488_set_cs();
        ili9488_clr_cs();
        ili9488_cmd_caset(0, ILI9488_COLS - 1);
        ili9488_cmd_raset(0, ILI9488_ROWS - 1);
        ili9488_cmd_ramrd(data_in, 32);
        ili9488_set_cs();
}*/

void ili9488_reset(void) {
    // some extra step based on new manufacturer recommendation
    if (Configuration::Instance().has_display_backlight_control()) {
        ili9488_set_rst();
        ili9488_delay_ms(1);
    }

    ili9488_clr_rst();
    ili9488_delay_ms(15);

#if HAS_TOUCH()
    touchscreen.reset_chip(ili9488_set_rst); // touch will restore reset
#else
    ili9488_set_rst();
#endif
}

void ili9488_power_down() {
    // activate reset pin of display, keep it enabled
    ili9488_clr_rst();
}

void ili9488_set_complete_lcd_reinit() {
    do_complete_lcd_reinit = true;
}

static void startup_old_manufacturer() {
    ili9488_cmd_slpout(); // wakeup
    ili9488_delay_ms(120); // 120ms wait
    ili9488_cmd_madctl(DEFAULT_MADCTL); // interface pixel format
    ili9488_cmd_colmod(DEFAULT_COLMOD); // memory data access control
    ili9488_cmd_dispon(); // display on
    ili9488_delay_ms(10); // 10ms wait
    ili9488_clear(0x000000); // black screen after power on
    ili9488_delay_ms(100); // time to set black color
    ili9488_inversion_on();
}

static void startup_new_manufacturer() {
    // Adjust Control 3
    // DSI write DCS command, use loose packet RGB 666
    ili9488_cmd_array(0xF7, std::to_array<uint8_t>({ 0xA9, 0x51, 0x2C, 0x82 }));

    // Memory Access Control
    // defines read/write scanning direction of the frame memory
    // ili9488_cmd_1_data(CMD_MADCTL, 0x48); - original recommended value, does not work, we use 0xe0
    ili9488_cmd_madctl(DEFAULT_MADCTL);

    ili9488_cmd_colmod(DEFAULT_COLMOD); // Interface Pixel Format 0x66:RGB666

    // Frame Rate Control (In Normal Mode/Full Colors) (this seems to be default)
    ili9488_cmd_array(0xB1, std::to_array<uint8_t>({
                                0xa0, // division ratio for internal clocks - Fosc, frame frequency of full color normal mode
                                0x11 // Clocks per line
                            }));

    // Display Inversion Control (this seems to be default)
    ili9488_cmd_1_data(0xB4, 0x02); // 2 dot inversion

    // Power Control 1
    ili9488_cmd_array(0xC0, std::to_array<uint8_t>({
                                0x0f, // Set the VREG1OUT voltage for positive gamma
                                0x0f // Set the VREG2OUT voltage for negative gammas
                            }));

    // Power Control 2
    ili9488_cmd_1_data(0xC1, 0x41); // Set the factor used in the step-up circuits.

    // Power Control 3 (For Normal Mode)
    ili9488_cmd_1_data(0xC2, 0x22); // Select the operating frequency of the step-up circuit

    // VCOM Control
    ili9488_cmd_array(0xC5, std::to_array<uint8_t>({
                                0x00, // 0: NV memory is not programmed
                                0x53, // VCM_REG [7:0]
                                0x80 // 1: VCOM value from VCM_REG [7:0].
                            }));

    // Entry Mode Set
    //  Deep Standby Mode, Low voltage detection ... format 16bbp (R, G, B) to 18 bbp (R, G, B) stored in the internal GRAM
    ili9488_cmd_1_data(0xB7, 0xc6);

    // PGAMCTRL (Positive Gamma Control)
    static constexpr auto gamma_control_data = std::to_array<uint8_t>({ 0x00, 0x08, 0x0c, 0x02, 0x0e, 0x04, 0x30, 0x45, 0x47, 0x04, 0x0c, 0x0a, 0x2e, 0x34, 0x0F });
    ili9488_cmd_array(0xE0, gamma_control_data);

    // NGAMCTRL (Negative Gamma Control)
    static constexpr auto ngamma_control_data = std::to_array<uint8_t>({ 0x00, 0x11, 0x0d, 0x01, 0x0f, 0x05, 0x39, 0x36, 0x51, 0x06, 0x0f, 0x0d, 0x33, 0x37, 0x0F });
    ili9488_cmd_array(0xE1, ngamma_control_data);

    ili9488_inversion_on(); // Display Inversion ON

    ili9488_cmd_slpout(); // Sleep OUT - turns off the sleep mode
    ili9488_delay_ms(120); // 120ms wait
    ili9488_cmd_dispon(); // display on
    ili9488_clear(0x000000); // black screen after power on
    // ili9488_delay_ms(100);      // time to set black color
}

void ili9488_init(void) {
    displayCs.write(Pin::State::low);
    ili9488_task_handle = osThreadGetId();
    if (ili9488_flg & ILI9488_FLG_SAFE) {
        ili9488_flg &= ~ILI9488_FLG_DMA;
    } else {
        ili9488_flg = ili9488_config.flg;
    }

    if (!option::bootloader || do_complete_lcd_reinit) {
        ili9488_reset(); // 15ms reset pulse
        ili9488_delay_ms(120); // 120ms wait
        if (buddy::hw::Configuration::Instance().has_display_backlight_control()) {
            startup_new_manufacturer();
        } else {
            startup_old_manufacturer();
        }
    } else {
        ili9488_cmd_madctl(DEFAULT_MADCTL); // interface pixel format
        ili9488_cmd_colmod(DEFAULT_COLMOD); // memory data access control
        ili9488_inversion_on();
    }

#if HAS_TOUCH()
    if (touchscreen.is_enabled()) {
        touchscreen.upload_touchscreen_config();
    }
#endif

    if (Configuration::Instance().has_display_backlight_control()) {
        ili9488_brightness_enable();

        // inverted brightness
        uint8_t pwm_inverted = 0b10110001;
        ili9488_cmd(CMD_CABCCTRL2, &pwm_inverted, sizeof(pwm_inverted));
    }

    ili9488_brightness_set(0xFF); // set backlight to maximum

    do_complete_lcd_reinit = false;
}

void ili9488_done(void) {
}

void ili9488_clear(uint32_t clr666) {
    assert(!ili9488_buff_borrowed && "Buffer lent to someone");

    int i;
    uint8_t *p_byte = (uint8_t *)ili9488_buff;

    for (i = 0; i < ILI9488_COLS * ILI9488_BUFF_ROWS - 1; i++) {
        *((uint32_t *)p_byte) = clr666;
        p_byte += 3; // increase the address by 3 because the color has 3 bytes
    }
    uint8_t *clr_ptr = (uint8_t *)&clr666;
    for (int j = 0; j < 3; j++) {
        *(p_byte + j) = *(clr_ptr + j);
    }

    ili9488_clr_cs();
    ili9488_cmd_caset(0, ILI9488_COLS - 1);
    ili9488_cmd_raset(0, ILI9488_ROWS - 1);
    ili9488_cmd_ramwr(0, 0);
    for (i = 0; i < ILI9488_ROWS / ILI9488_BUFF_ROWS; i++) {
        ili9488_wr(ili9488_buff, sizeof(ili9488_buff));
    }
    ili9488_set_cs();
    //	ili9488_test_miso();
}

void ili9488_set_pixel(uint16_t point_x, uint16_t point_y, uint32_t clr666) {
    ili9488_cmd_caset(point_x, point_x + 1);
    ili9488_cmd_raset(point_y, point_y + 1);
    ili9488_cmd_ramwr((uint8_t *)(&clr666), 3);
}

uint8_t *ili9488_get_block(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    assert(!ili9488_buff_borrowed && "Buffer lent to someone");

    if (start_x >= ILI9488_COLS || start_y >= ILI9488_ROWS || end_x >= ILI9488_COLS || end_y >= ILI9488_ROWS) {
        return NULL;
    }
    ili9488_cmd_caset(start_x, end_x);
    ili9488_cmd_raset(start_y, end_y);
    ili9488_cmd_ramrd(ili9488_buff, ILI9488_COLS * 3 * ILI9488_BUFF_ROWS);
    return ili9488_buff;
}

uint32_t ili9488_get_pixel_colorFormat666(uint16_t point_x, uint16_t point_y) {
    enum { buff_sz = 5 };
    uint8_t buff[buff_sz];
    ili9488_cmd_caset(point_x, point_x + 1);
    ili9488_cmd_raset(point_y, point_y + 1);
    ili9488_cmd_ramrd(buff, buff_sz);
    uint32_t ret = ((buff[0] << 16) & 0xFC0000) + ((buff[1] << 8) & 0x00FC00) + (buff[2] & 0xFC);
    return ret; // directColor;
}

void ili9488_fill_rect_colorFormat666(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t clr666) {
    // BFW-6328 Some displays possibly problematic with higher baudrate, reduce 40 -> 20 MHz
    SPIBaudRatePrescalerGuard _g(&SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_4, reduce_display_baudrate);

    assert(!ili9488_buff_borrowed && "Buffer lent to someone");

    int i;
    uint32_t size = (uint32_t)rect_w * rect_h * 3;
    int n = size / sizeof(ili9488_buff);
    int s = size % sizeof(ili9488_buff);
    if (n) {
        ili9488_fill_ui24((uint8_t *)ili9488_buff, clr666, sizeof(ili9488_buff) / 3);
    } else {
        ili9488_fill_ui24((uint8_t *)ili9488_buff, clr666, size / 3);
    }
    ili9488_clr_cs();
    ili9488_cmd_caset(rect_x, rect_x + rect_w - 1);
    ili9488_cmd_raset(rect_y, rect_y + rect_h - 1);
    ili9488_cmd_ramwr(0, 0);
    for (i = 0; i < n; i++) {
        ili9488_wr(ili9488_buff, sizeof(ili9488_buff));
    }
    if (s) {
        ili9488_wr(ili9488_buff, s);
    }
    ili9488_set_cs();
}

void ili9488_draw_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    /// @note This function is used when someone borrowed the buffer and filled it with data, don't check ili9488_buff_borrowed.
    /// @todo Cannot check that the buffer is borrowed because it is returned before calling this. Needs refactoring.

    ili9488_clr_cs();
    ili9488_cmd_caset(x, x + w - 1);
    ili9488_cmd_raset(y, y + h - 1);
    ili9488_cmd_ramwr(ili9488_buff, 3 * w * h);
    ili9488_set_cs();
}

void ili9488_draw_qoi_ex(FILE *pf, uint16_t point_x, uint16_t point_y, Color back_color, uint8_t rop, Rect16 subrect) {
    assert(!ili9488_buff_borrowed && "Buffer lent to someone");
    assert(pf);

    // BFW-6328 Some displays possibly problematic with higher baudrate, reduce 40 -> 20 MHz
    SPIBaudRatePrescalerGuard _g(&SPI_HANDLE_FOR(lcd), SPI_BAUDRATEPRESCALER_4, reduce_display_baudrate);

    // Current pixel position starts top-left where the image is placed
    point_i16_t pos = { static_cast<int16_t>(point_x), static_cast<int16_t>(point_y) };

    // Prepare input buffer
    std::span<uint8_t> i_buf(ili9488_buff, 512); ///< Input file buffer
    std::span<uint8_t> i_data; ///< Span of input data read from file

    // Prepare output buffer
    std::span<uint8_t> p_buf(ili9488_buff + i_buf.size(), std::size(ili9488_buff) - i_buf.size()); ///< Output pixel buffer
    auto o_data = p_buf.begin(); ///< Pointer to output pixel data in buffer

#if 0
    // Measure time it takes to draw QOI image
    #warning "Spamming the log"
    struct ImgMeasure {
        volatile uint32_t start_us;
        ImgMeasure() { start_us = ticks_us(); }
        ~ImgMeasure() { log_debug(GUI, "Img draw took %u us", ticks_us() - start_us); }
    } image_timing;
#endif /*0*/

    // Read header and image size to tweak drawn subrect
    if (size_t nread = fread(i_buf.data(), 1, qoi::Decoder::HEADER_SIZE, pf); nread != qoi::Decoder::HEADER_SIZE) {
        return; // Header couldn't be read
    }
    Rect16 img_rect = Rect16(pos, qoi::Decoder::get_image_size(std::span<uint8_t, qoi::Decoder::HEADER_SIZE>(i_buf)));

    // Recalculate subrect that is going to be drawn
    if (subrect.IsEmpty()) {
        subrect = img_rect;
    } else {
        subrect.Intersection(img_rect);
    }
    subrect.Intersection(Rect16(0, 0, ILI9488_COLS, ILI9488_ROWS)); // Clip drawn subrect to display size

    // Prepare output
    // Set write rectangle
    ili9488_cmd_caset(subrect.Left(), subrect.Right());
    ili9488_cmd_raset(subrect.Top(), subrect.Bottom());
    // Start write of data
    ili9488_cmd_ramwr(0, 0);

    qoi::Decoder qoi_decoder; ///< QOI decoding statemachine
    while (1) {
        // Read more data from file
        if (size_t nread = fread(i_buf.data(), 1, i_buf.size(), pf); nread == 0) {
            break; // Picture ends
        } else {
            i_data = std::span<uint8_t>(i_buf.begin(), nread);
        }

        // Process input data
        for (auto i_byte : i_data) {

            // Push byte to decoder
            qoi_decoder.push_byte((uint8_t)i_byte);

            // Pull pixels from decoder
            while (qoi_decoder.has_pixel()) {
                qoi::Pixel pixel = qoi_decoder.pull_pixel();

                // Keep track of pixel position
                auto orig_pos = pos;
                pos.x++;
                if (pos.x > subrect.Right()) {
                    pos.x = subrect.Left();
                    pos.y++;
                }

                // Skip pixels outside of subrect
                if (subrect.Contain(orig_pos) == false) {
                    if (orig_pos.y > subrect.Bottom()) { // Picture ends
                        // Write remaining pixels to display and close SPI transaction
                        ili9488_wr(p_buf.data(), o_data - p_buf.begin());
                        ili9488_set_cs();
                        return;
                    }
                    continue;
                }

                // Transform pixel data
                pixel = qoi::transform::apply_rop(pixel, rop);

                const Color c = Color::mix(back_color, Color::from_rgb(pixel.r, pixel.g, pixel.b), pixel.a);

                // Store to output buffer
                *o_data++ = c.b;
                *o_data++ = c.g;
                *o_data++ = c.r;

                // Another 3 bytes wouldn't fit, write to display
                if (p_buf.end() - o_data < 3) {
                    ili9488_wr(p_buf.data(), o_data - p_buf.begin());
                    o_data = p_buf.begin();
                }
            }
        }
    }

    // Write remaining pixels to display and close SPI transaction
    ili9488_wr(p_buf.data(), o_data - p_buf.begin());
    ili9488_set_cs();
}

void ili9488_inversion_on(void) {
    ili9488_config.is_inverted = 1;
    ili9488_cmd(CMD_INVON, 0, 0);
}
void ili9488_inversion_off(void) {
    ili9488_config.is_inverted = 0;
    ili9488_cmd(CMD_INVOFF, 0, 0);
}

void ili9488_inversion_tgl(void) {
#if CMD_INVON == CMD_INVOFF + 1
    // faster code if CMD_INVON == CMD_INVOFF + 1
    // The result of the logical negation operator ! is 1 if the value of its operand is 0,
    // 0 if the value of its operand is non-zero.
    ili9488_config.is_inverted = !ili9488_inversion_get();
    ili9488_cmd(CMD_INVOFF + ili9488_config.is_inverted, 0, 0);
#else
    // to be portable
    if (ili9488_inversion_get()) {
        ili9488_inversion_off();
    } else {
        ili9488_inversion_on();
    }

#endif
}
uint8_t ili9488_inversion_get(void) {
    return ili9488_config.is_inverted;
}

// 0x01 -> 0x02 -> 0x04 -> 0x08 -> 0x01
void ili9488_gamma_next(void) {
    ili9488_gamma_set_direct(((ili9488_config.gamma << 1) | (ili9488_config.gamma >> 3)) & 0x0f);
}

// 0x01 -> 0x08 -> 0x04 -> 0x02 -> 0x01
void ili9488_gamma_prev(void) {
    ili9488_gamma_set_direct(((ili9488_config.gamma << 3) | (ili9488_config.gamma >> 1)) & 0x0f);
}

// use GAMMA_CURVE0 - GAMMA_CURVE3
void ili9488_gamma_set_direct(uint8_t gamma_enu) {
    ili9488_config.gamma = gamma_enu;
    ili9488_cmd(CMD_GAMMA_SET, &ili9488_config.gamma, sizeof(ili9488_config.gamma));
}

// use 0 - 3
void ili9488_gamma_set(uint8_t gamma) {
    if (gamma != ili9488_gamma_get()) {
        ili9488_gamma_set_direct(1 << (gamma & 0x03));
    }
}

// returns 0 - 3
uint8_t ili9488_gamma_get() {
    uint8_t position;
    for (position = 3; position != 0; --position) {
        if (ili9488_config.gamma == 1 << position) {
            break;
        }
    }

    return position;
}

void ili9488_brightness_enable(void) {
    ili9488_ctrl_set(ili9488_config.control | MASK_CTRLD_BCTRL | MASK_CTRLD_BL);
}

void ili9488_brightness_disable(void) {
    ili9488_ctrl_set(ili9488_config.control & (~MASK_CTRLD_BCTRL) & (~MASK_CTRLD_BL));
}

void ili9488_brightness_set(uint8_t brightness) {
    ili9488_config.brightness = brightness;
    // set brightness
    ili9488_cmd(CMD_WRDISBV, &ili9488_config.brightness, sizeof(ili9488_config.brightness));
}

uint8_t ili9488_brightness_get(void) {
    return ili9488_config.brightness;
}

void ili9488_ctrl_set(uint8_t ctrl) {
    ili9488_config.control = ctrl;
    ili9488_cmd(CMD_WRCTRLD, &ili9488_config.control, sizeof(ili9488_config.control));
}

//! @brief enable safe mode (direct acces + safe delay)
void ili9488_enable_safe_mode(void) {
    ili9488_flg |= ILI9488_FLG_SAFE;
}

void ili9488_spi_tx_complete(void) {
    osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_TX);
}

void ili9488_spi_rx_complete(void) {
    osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_RX);
}

void ili9488_cmd_nop() {
    ili9488_cmd(CMD_NOP, 0, 0);
}
