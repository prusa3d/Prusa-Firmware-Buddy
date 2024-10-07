#include "st7789v.hpp"

#include "cmath_ext.h"
#include "cmsis_os.h"
#include "disable_interrupts.h"
#include "hwio_pindef.h"
#include "qoi_decoder.hpp"
#include "raster_opfn_c.h"
#include <buddy/ccm_thread.hpp>
#include <device/hal.h>
#include <span>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define FLG_CS           0x01 // current CS pin state
#define FLG_RS           0x02 // current RS pin state
#define FLG_RST          0x04 // current RST pin state
#define ST7789V_FLG_DMA  0x08 // DMA enabled
#define ST7789V_FLG_SAFE 0x20 // SAFE mode (no DMA and safe delay)

constexpr static uint8_t ST7789V_DEF_MADCTL = 0xC0; // memory data access control (mirror XY)
constexpr static uint8_t ST7789V_DEF_COLMOD = 0x05; // interface pixel format (5-6-5, hi-color)

struct st7789v_config_t {
    uint8_t flg; // flags (DMA, MISO)
    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
};

st7789v_config_t st7789v_config = {
    .flg = ST7789V_FLG_DMA, // flags (DMA, MISO)
    .gamma = 0,
    .brightness = 0,
    .is_inverted = 0,
    .control = 0,
};

// st7789 commands
enum {
    CMD_MADCTLRD = 0x0b, // Read MADCTL register
    CMD_SLPIN = 0x10,
    CMD_SLPOUT = 0x11,
    CMD_INVOFF = 0x20, // Display Inversion Off
    CMD_INVON = 0x21, // Display Inversion On
    CMD_GAMMA_SET = 0x26, // gamma set
    CMD_DISPOFF = 0x28,
    CMD_DISPON = 0x29,
    CMD_CASET = 0x2A,
    CMD_RASET = 0x2B,
    CMD_RAMWR = 0x2C,
    CMD_RAMRD = 0x2E,
    CMD_MADCTL = 0x36,
    // CMD_IDMOFF = 0x38,//Idle Mode Off FIXME shouldn't be 0x37?
    // CMD_IDMON = 0x38,//Idle Mode On
    CMD_COLMOD = 0x3A,
    CMD_RAMWRC = 0x3C,
    CMD_WRDISBV = 0x51, // Write Display Brightness
    CMD_RDDISBV = 0x52, // Read Display Brightness Value
    CMD_WRCTRLD = 0x53, // Write CTRL Display
                        //-Brightness Control Block - bit 5
                        //-Display Dimming			- bit 3
                        //-Backlight Control On/Off - bit 2
    CMD_RDCTRLD = 0x54, // Read CTRL Value Display
};

// st7789 gamma
enum {
    GAMMA_CURVE0 = 0x01,
    GAMMA_CURVE1 = 0x02,
    GAMMA_CURVE2 = 0x04,
    GAMMA_CURVE3 = 0x08,
};

// st7789 CTRL Display
static const uint8_t MASK_CTRLD_BCTRL = 1 << 5; // Brightness Control Block
// static const uint8_t MASK_CTRLD_DD = 1 << 3;    //Display Dimming
// static const uint8_t MASK_CTRLD_BL = 1 << 2;    //Backlight Control

// color constants
enum {
    CLR565_WHITE = 0xffff,
    CLR565_BLACK = 0x0000,
    CLR565_RED = 0xf800,
    CLR565_GREEN = 0x07e0,
    CLR565_YELLOW = 0xffe0,
    CLR565_GRAY = 0x38e7,
    CLR565_BLUE = 0x001f,
};

uint8_t st7789v_flg = 0; // flags

uint16_t st7789v_x = 0; // current x coordinate (CASET)
uint16_t st7789v_y = 0; // current y coordinate (RASET)
uint16_t st7789v_cx = 0; //
uint16_t st7789v_cy = 0; //

osThreadId st7789v_task_handle = 0;

static const uint32_t ST7789V_SIG_SPI_TX = 0x08;

static constexpr uint8_t ST7789V_MAX_COMMAND_READ_LENGHT = 4;

uint8_t st7789v_buff[ST7789V_BUFFER_SIZE]; // display buffer
bool st7789v_buff_borrowed = false; ///< True if buffer is borrowed by someone else

uint8_t *st7789v_borrow_buffer() {
    assert(!st7789v_buff_borrowed && "Already lent");
    assert(st7789v_task_handle == osThreadGetId() && "Must be called only from one task");
    st7789v_buff_borrowed = true;
    return st7789v_buff;
}

void st7789v_return_buffer() {
    assert(st7789v_buff_borrowed);
    st7789v_buff_borrowed = false;
}

size_t st7789v_buffer_size() {
    return sizeof(st7789v_buff);
}

/*some functions are in header - excluded from display_t struct*/
void st7789v_gamma_set_direct(uint8_t gamma_enu);
uint8_t st7789v_read_ctrl(void);
void st7789v_ctrl_set(uint8_t ctrl);
static void st7789v_delay_ms(uint32_t ms);

using buddy::hw::displayCs;
using buddy::hw::displayRs;
using buddy::hw::displayRst;
using buddy::hw::InputEnabler;
using buddy::hw::Pin;
using buddy::hw::Pull;

static void st7789v_set_cs(void) {
    displayCs.write(Pin::State::high);
    st7789v_flg |= FLG_CS;
}

static void st7789v_clr_cs(void) {
    displayCs.write(Pin::State::low);
    st7789v_flg &= ~FLG_CS;
}

static void st7789v_set_rs(void) {
    displayRs.write(Pin::State::high);
    st7789v_flg |= FLG_RS;
}

static void st7789v_clr_rs(void) {
    displayRs.write(Pin::State::low);
    st7789v_flg &= ~FLG_RS;
}

static void st7789v_set_rst(void) {
    displayRst.write(Pin::State::high);
    st7789v_flg |= FLG_RST;
}

static void st7789v_clr_rst(void) {
    displayRst.write(Pin::State::low);
    st7789v_flg &= ~FLG_RST;
}

void st7789v_reset(void) {
    st7789v_clr_rst();
    st7789v_delay_ms(15);
    volatile uint16_t delay = 0;
    {
        InputEnabler rstInput(displayRst, Pull::up);
        buddy::DisableInterrupts disable_interrupts;
        while (rstInput.read() == Pin::State::low) {
            delay++;
        }
    }
    st7789v_set_rst();
    st7789v_reset_delay = delay;
}

static inline void st7789v_fill_ui16(uint16_t *p, uint16_t v, uint16_t c) {
    while (c--) {
        *(p++) = v;
    }
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

static void st7789v_delay_ms(uint32_t ms) {
    if (is_interrupt() || (st7789v_flg & (uint8_t)ST7789V_FLG_SAFE)) {
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

void st7789v_spi_wr_byte(uint8_t b) {
    HAL_SPI_Transmit(&SPI_HANDLE_FOR(lcd), &b, 1, HAL_MAX_DELAY);
}

void st7789v_spi_wr_bytes(uint8_t *pb, uint16_t size) {
    if ((st7789v_flg & (uint8_t)ST7789V_FLG_DMA) && !(st7789v_flg & (uint8_t)ST7789V_FLG_SAFE) && (size > 4)) {
        osSignalSet(st7789v_task_handle, ST7789V_SIG_SPI_TX);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
        assert(can_be_used_by_dma(pb));
        HAL_SPI_Transmit_DMA(&SPI_HANDLE_FOR(lcd), pb, size);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    } else {
        HAL_SPI_Transmit(&SPI_HANDLE_FOR(lcd), pb, size, HAL_MAX_DELAY);
    }
}

void st7789v_spi_rd_bytes(uint8_t *pb, uint16_t size) {
#if 0
//#ifdef ST7789V_DMA
    if (size <= 4)
        HAL_SPI_Receive(&SPI_HANDLE_FOR(lcd), pb, size, HAL_MAX_DELAY);
    else
    {
        osSignalSet(0, ST7789V_SIG_SPI_TX);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
        assert(can_be_used_by_dma(pb));
        HAL_SPI_Receive_DMA(&SPI_HANDLE_FOR(lcd), pb, size);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    }
#else // ST7789V_DMA
    HAL_SPI_Receive(&SPI_HANDLE_FOR(lcd), pb, size, HAL_MAX_DELAY);
#endif // ST7789V_DMA
}

void st7789v_cmd(uint8_t cmd, uint8_t *pdata, uint16_t size) {
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS) {
        st7789v_clr_cs(); // CS = L
    }
    if (st7789v_flg & FLG_RS) {
        st7789v_clr_rs(); // RS = L
    }
    st7789v_spi_wr_byte(cmd); // write command byte
    if (pdata && size) {
        st7789v_set_rs(); // RS = H
        st7789v_spi_wr_bytes(pdata, size); // write data bytes
    }
    if (tmp_flg & FLG_CS) {
        st7789v_set_cs(); // CS = H
    }
}

#pragma GCC push_options
#pragma GCC optimize("O3")
void st7789v_cmd_rd(uint8_t cmd, uint8_t *pdata) {
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS) {
        st7789v_clr_cs(); // CS = L
    }
    if (st7789v_flg & FLG_RS) {
        st7789v_clr_rs(); // RS = L
    }
    uint8_t data_to_write[ST7789V_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    data_to_write[0] = cmd;
    data_to_write[1] = 0x00;
    HAL_SPI_TransmitReceive(&SPI_HANDLE_FOR(lcd), data_to_write, pdata, ST7789V_MAX_COMMAND_READ_LENGHT, HAL_MAX_DELAY);
    if (tmp_flg & FLG_CS) {
        st7789v_set_cs();
    }
}
#pragma GCC pop_options

void st7789v_wr(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size)) {
        return; // null or empty data - return
    }
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS) {
        st7789v_clr_cs(); // CS = L
    }
    if (!(st7789v_flg & FLG_RS)) {
        st7789v_set_rs(); // RS = H
    }
    st7789v_spi_wr_bytes(pdata, size); // write data bytes
    if (tmp_flg & FLG_CS) {
        st7789v_set_cs(); // CS = H
    }
}

void st7789v_rd(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size)) {
        return; // null or empty data - return
    }
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS) {
        st7789v_clr_cs(); // CS = L
    }
    if (!(st7789v_flg & FLG_RS)) {
        st7789v_set_rs(); // RS = H
    }
    st7789v_spi_rd_bytes(pdata, size); // read data bytes
    if (tmp_flg & FLG_CS) {
        st7789v_set_cs(); // CS = H
    }
}

void st7789v_cmd_slpout(void) {
    st7789v_cmd(CMD_SLPOUT, 0, 0);
}

void st7789v_cmd_madctl(uint8_t madctl) {
    st7789v_cmd(CMD_MADCTL, &madctl, 1);
}

void st7789v_cmd_colmod(uint8_t colmod) {
    st7789v_cmd(CMD_COLMOD, &colmod, 1);
}

void st7789v_cmd_dispon(void) {
    st7789v_cmd(CMD_DISPON, 0, 0);
}

void st7789v_cmd_caset(uint16_t x, uint16_t cx) {
    uint8_t data[4] = { static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x & 0xff), static_cast<uint8_t>(cx >> 8), static_cast<uint8_t>(cx & 0xff) };
    st7789v_cmd(CMD_CASET, data, sizeof(data));
}

void st7789v_cmd_raset(uint16_t y, uint16_t cy) {
    uint8_t data[4] = { static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y & 0xff), static_cast<uint8_t>(cy >> 8), static_cast<uint8_t>(cy & 0xff) };
    st7789v_cmd(CMD_RASET, data, sizeof(data));
}

void st7789v_cmd_ramwr(uint8_t *pdata, uint16_t size) {
    st7789v_cmd(CMD_RAMWR, pdata, size);
}

void st7789v_cmd_ramrd(uint8_t *pdata, uint16_t size) {
    st7789v_cmd(CMD_RAMRD, 0, 0);
    st7789v_rd(pdata, size);
}

bool st7789v_is_reset_required() {
    uint8_t pdata[ST7789V_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    st7789v_cmd_rd(CMD_MADCTLRD, pdata);
    if ((pdata[1] != 0xE0 && pdata[1] != 0xF0 && pdata[1] != 0xF8)) {
        return true;
    }
    return false;
}

/*void st7789v_test_miso(void)
{
//	uint16_t data_out[8] = {CLR565_WHITE, CLR565_WHITE, CLR565_RED, CLR565_RED, CLR565_GREEN, CLR565_GREEN, CLR565_BLUE, CLR565_BLUE};
    uint8_t data_out[16] = {0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t data_in[32];
    memset(data_in, 0, sizeof(data_in));
    st7789v_clr_cs();
    st7789v_cmd_caset(0, ST7789V_COLS - 1);
    st7789v_cmd_raset(0, ST7789V_ROWS - 1);
    st7789v_cmd_ramwr((uint8_t*)data_out, 16);
    st7789v_set_cs();
    st7789v_clr_cs();
    st7789v_cmd_caset(0, ST7789V_COLS - 1);
    st7789v_cmd_raset(0, ST7789V_ROWS - 1);
    st7789v_cmd_ramrd(data_in, 32);
    st7789v_set_cs();
}*/

void st7789v_init_ctl_pins(void) {
    st7789v_flg &= ~(FLG_CS | FLG_RS | FLG_RST);
    st7789v_set_rst();
    st7789v_set_cs();
    st7789v_set_rs();
}

/**
 * Warning: All interrupts are disabled in st7789v_reset() while measuring
 * delay to detect Jogwheel revision.
 */
void st7789v_init(void) {
    st7789v_task_handle = osThreadGetId();
    if (st7789v_flg & (uint8_t)ST7789V_FLG_SAFE) {
        st7789v_flg &= ~(uint8_t)ST7789V_FLG_DMA;
    } else {
        st7789v_flg = st7789v_config.flg;
    }
    st7789v_init_ctl_pins(); // CS=H, RS=H, RST=H
    st7789v_reset(); // 15ms reset pulse
    st7789v_delay_ms(120); // 120ms wait
    st7789v_cmd_slpout(); // wakeup
    st7789v_delay_ms(120); // 120ms wait
    st7789v_cmd_madctl(ST7789V_DEF_MADCTL); // interface pixel format
    st7789v_cmd_colmod(ST7789V_DEF_COLMOD); // memory data access control
    st7789v_cmd_dispon(); // display on
    st7789v_delay_ms(10); // 10ms wait
}

void st7789v_done(void) {
}

/// Fills screen by this color
void st7789v_clear(uint16_t clr565) {
    assert(!st7789v_buff_borrowed && "Buffer lent to someone");

    // FIXME similar to display_ex_fill_rect; join?
    int i;
    for (i = 0; i < ST7789V_COLS * 16; i++) {
        ((uint16_t *)st7789v_buff)[i] = clr565;
    }
    st7789v_clr_cs();
    st7789v_cmd_caset(0, ST7789V_COLS - 1);
    st7789v_cmd_raset(0, ST7789V_ROWS - 1);
    st7789v_cmd_ramwr(0, 0);
    for (i = 0; i < ST7789V_ROWS / 16; i++) {
        st7789v_wr(st7789v_buff, 2 * ST7789V_COLS * 16);
    }
    st7789v_set_cs();
    //	st7789v_test_miso();
}

/// Turns the specified pixel to the specified color
void st7789v_set_pixel(uint16_t point_x, uint16_t point_y, uint16_t clr565) {
    st7789v_cmd_caset(point_x, 1);
    st7789v_cmd_raset(point_y, 1);
    st7789v_cmd_ramwr((uint8_t *)(&clr565), 2);
}

// 1 == 0000 0001, 5 == 0001 1111
static uint16_t set_num_of_ones(uint8_t num_of_ones) {
    return ~((uint16_t)(-1) << num_of_ones);
}

// BYTEs 0 and 1 are empty
// bit0 -> bit5 BYTE2
// bit1 -> bit6 BYTE2
// bit2 -> bit7 BYTE2

// bit3 -> bit3 BYTE4
// bit4 -> bit4 BYTE4
// bit5 -> bit5 BYTE4
// bit6 -> bit6 BYTE4
// bit7 -> bit7 BYTE4

// bit8 -> bit3 BYTE3
// bit9 -> bit4 BYTE3
// bit10-> bit5 BYTE3
// bit11-> bit6 BYTE3
// bit12-> bit7 BYTE3

// bit13-> bit2 BYTE3
// bit14-> bit3 BYTE3
// bit15-> bit4 BYTE2

// since BYTE0 and BYTE1 are empty, buff[0] == BYTE2
static uint16_t rd18bit_to_16bit(uint8_t *buff) {
    return ((uint16_t)(buff[0] >> 5) & set_num_of_ones(3)) | (((uint16_t)(buff[2] >> 3) & set_num_of_ones(5)) << 3) | (((uint16_t)(buff[1] >> 3) & set_num_of_ones(5)) << 8) | (((uint16_t)(buff[0] >> 2) & set_num_of_ones(3)) << 13);
}

uint16_t st7789v_get_pixel_colorFormat565(uint16_t point_x, uint16_t point_y) {
    enum { buff_sz = 5 };
    uint8_t buff[buff_sz];
    st7789v_cmd_caset(point_x, 1);
    st7789v_cmd_raset(point_y, 1);
    st7789v_cmd_ramrd(buff, buff_sz);
    uint16_t ret = rd18bit_to_16bit(buff + 2);
    return ret; // directColor;
}

uint8_t *st7789v_get_block(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    assert(!st7789v_buff_borrowed && "Buffer lent to someone");

    if (start_x > ST7789V_COLS || start_y > ST7789V_ROWS || end_x > ST7789V_COLS || end_y > ST7789V_ROWS) {
        return NULL;
    }

    // ST7789 spec: Note1: The Command 3Ah should be set to 66h when reading pixel data from frame memory.
    // In our case, it is 06h, because we're using serial interface
    // The data readout is always in 3 bytes per pixel
    st7789v_cmd_colmod(0x06);

    st7789v_cmd_caset(start_x, end_x);
    st7789v_cmd_raset(start_y, end_y);

    // Again, 3 bytes per pixel + 2 extra bytes because of some read offset
    const auto read_bytes = (end_x - start_x + 1) * (end_y - start_y + 1) * 3 + 2;
    assert(read_bytes <= sizeof(st7789v_buff));
    st7789v_cmd_ramrd(st7789v_buff, read_bytes);

    // Revert back
    st7789v_cmd_colmod(ST7789V_DEF_COLMOD);
    return st7789v_buff;
}

/// Draws a solid rectangle of defined color
void st7789v_fill_rect_colorFormat565(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint16_t clr565) {
    assert(!st7789v_buff_borrowed && "Buffer lent to someone");

    uint32_t size = (uint32_t)rect_w * rect_h * 2; // area of rectangle

    st7789v_fill_ui16((uint16_t *)st7789v_buff, clr565, MIN(size, sizeof(st7789v_buff) / 2));
    st7789v_clr_cs();
    st7789v_cmd_caset(rect_x, rect_x + rect_w - 1);
    st7789v_cmd_raset(rect_y, rect_y + rect_h - 1);
    st7789v_cmd_ramwr(0, 0);

    for (unsigned int i = 0; i < size / sizeof(st7789v_buff); i++) { // writer buffer by buffer
        st7789v_wr(st7789v_buff, sizeof(st7789v_buff));
    }

    st7789v_wr(st7789v_buff, size % sizeof(st7789v_buff)); // write the remainder data
    st7789v_set_cs();
}

void st7789v_draw_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    /// @note This function is used when someone borrowed the buffer and filled it with data, don't check ili9488_buff_borrowed.
    /// @todo Cannot check that the buffer is borrowed because it is returned before calling this. Needs refactoring.

    st7789v_clr_cs();
    st7789v_cmd_caset(x, x + w - 1);
    st7789v_cmd_raset(y, y + h - 1);
    st7789v_cmd_ramwr(st7789v_buff, 2 * w * h);
    st7789v_set_cs();
}

void st7789v_inversion_on(void) {
    st7789v_config.is_inverted = 1;
    st7789v_cmd(CMD_INVON, 0, 0);
}
void st7789v_inversion_off(void) {
    st7789v_config.is_inverted = 0;
    st7789v_cmd(CMD_INVOFF, 0, 0);
}

void st7789v_inversion_tgl(void) {
#if CMD_INVON == CMD_INVOFF + 1
    // faster code if CMD_INVON == CMD_INVOFF + 1
    // The result of the logical negation operator ! is 1 if the value of its operand is 0,
    // 0 if the value of its operand is non-zero.
    st7789v_config.is_inverted = !st7789v_inversion_get();
    st7789v_cmd(CMD_INVOFF + st7789v_config.is_inverted, 0, 0);
#else
    // to be portable
    if (st7789v_inversion_get()) {
        st7789v_inversion_off();
    } else {
        st7789v_inversion_on();
    }

#endif
}
uint8_t st7789v_inversion_get(void) {
    return st7789v_config.is_inverted;
}

// 0x01 -> 0x02 -> 0x04 -> 0x08 -> 0x01
void st7789v_gamma_next(void) {
    st7789v_gamma_set_direct(((st7789v_config.gamma << 1) | (st7789v_config.gamma >> 3)) & 0x0f);
}

// 0x01 -> 0x08 -> 0x04 -> 0x02 -> 0x01
void st7789v_gamma_prev(void) {
    st7789v_gamma_set_direct(((st7789v_config.gamma << 3) | (st7789v_config.gamma >> 1)) & 0x0f);
}

// use GAMMA_CURVE0 - GAMMA_CURVE3
void st7789v_gamma_set_direct(uint8_t gamma_enu) {
    st7789v_config.gamma = gamma_enu;
    st7789v_cmd(CMD_GAMMA_SET, &st7789v_config.gamma, sizeof(st7789v_config.gamma));
}

// use 0 - 3
void st7789v_gamma_set(uint8_t gamma) {
    if (gamma != st7789v_gamma_get()) {
        st7789v_gamma_set_direct(1 << (gamma & 0x03));
    }
}

// returns 0 - 3
uint8_t st7789v_gamma_get() {
    uint8_t position = 3;
    for (; position != 0; --position) {
        if (st7789v_config.gamma == 1 << position) {
            break;
        }
    }

    return position;
}

void st7789v_brightness_enable(void) {
    st7789v_ctrl_set(st7789v_config.control | MASK_CTRLD_BCTRL);
}

void st7789v_brightness_disable(void) {
    st7789v_ctrl_set(st7789v_config.control & (~MASK_CTRLD_BCTRL));
}

void st7789v_brightness_set(uint8_t brightness) {
    st7789v_config.brightness = brightness;
    // set brightness
    st7789v_cmd(CMD_WRDISBV, &st7789v_config.brightness, sizeof(st7789v_config.brightness));
}

uint8_t st7789v_brightness_get(void) {
    return st7789v_config.brightness;
}

void st7789v_ctrl_set(uint8_t ctrl) {
    st7789v_config.control = ctrl;
    st7789v_cmd(CMD_WRCTRLD, &st7789v_config.control, sizeof(st7789v_config.control));
}

void st7789v_draw_qoi_ex(FILE *pf, uint16_t point_x, uint16_t point_y, Color back_color, uint8_t rop, Rect16 subrect) {
    assert(!st7789v_buff_borrowed && "Buffer lent to someone");
    assert(pf);

    // Current pixel position starts top-left where the image is placed
    point_i16_t pos = { static_cast<int16_t>(point_x), static_cast<int16_t>(point_y) };

    // Prepare input buffer
    std::span<uint8_t> i_buf(st7789v_buff, 512); ///< Input file buffer
    std::span<uint8_t> i_data; ///< Span of input data read from file

    // Prepare output buffer
    std::span<uint8_t> p_buf(st7789v_buff + i_buf.size(), std::size(st7789v_buff) - i_buf.size()); ///< Output pixel buffer
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
    subrect.Intersection(Rect16(0, 0, ST7789V_COLS, ST7789V_ROWS)); // Clip drawn subrect to display size

    // Prepare output
    // Set write rectangle
    st7789v_cmd_caset(subrect.Left(), subrect.Right());
    st7789v_cmd_raset(subrect.Top(), subrect.Bottom());
    // Start write of data
    st7789v_cmd_ramwr(0, 0);

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
                        st7789v_wr(p_buf.data(), o_data - p_buf.begin());
                        st7789v_set_cs();
                        return;
                    }
                    continue;
                }

                // Transform pixel data
                pixel = qoi::transform::apply_rop(pixel, rop);

                // Store to output buffer
                const Color c = Color::mix(back_color, Color::from_rgb(pixel.r, pixel.g, pixel.b), pixel.a);
                uint16_t clr565 = color_to_565(c);
                *o_data++ = clr565;
                *o_data++ = clr565 >> 8;

                // Another 3 bytes wouldn't fit, write to display
                if (p_buf.end() - o_data < 3) {
                    st7789v_wr(p_buf.data(), o_data - p_buf.begin());
                    o_data = p_buf.begin();
                }
            }
        }
    }

    // Write remaining pixels to display and close SPI transaction
    st7789v_wr(p_buf.data(), o_data - p_buf.begin());
    st7789v_set_cs();
}

// measured delay from low to hi in reset cycle
uint16_t st7789v_reset_delay = 0;

//! @brief enable safe mode (direct access + safe delay)
void st7789v_enable_safe_mode(void) {
    st7789v_flg |= (uint8_t)ST7789V_FLG_SAFE;
}

void st7789v_spi_tx_complete(void) {
    osSignalSet(st7789v_task_handle, ST7789V_SIG_SPI_TX);
}
