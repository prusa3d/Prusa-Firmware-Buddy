// ili9488.cpp
#include "ili9488.hpp"
#include <guiconfig.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "bsod.h"
#include "cmath_ext.h"
#include "config_buddy_2209_02.h"

#include "touch_dependency.hpp"

#include "raster_opfn_c.h"
#include "hwio_pindef.h"
#include <device/board.h>
#ifdef ILI9488_USE_RTOS
    #include "cmsis_os.h"
#endif //ILI9488_USE_RTOS
#include "main.h"

#include "png_measure.hpp"

#include <option/bootloader.h>

LOG_COMPONENT_REF(GUI);

//ili9488 commands
#define CMD_SLPIN     0x10
#define CMD_SLPOUT    0x11
#define CMD_INVOFF    0x20 //Display Inversion Off
#define CMD_INVON     0x21 //Display Inversion On
#define CMD_GAMMA_SET 0x26 //gamma set
#define CMD_DISPOFF   0x28
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_RASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_RAMRD     0x2E
#define CMD_MADCTL    0x36
//#define CMD_IDMOFF 		0x38//Idle Mode Off
//#define CMD_IDMON 		0x38//Idle Mode On
#define CMD_COLMOD  0x3A
#define CMD_RAMWRC  0x3C
#define CMD_WRDISBV 0x51 //Write Display Brightness
#define CMD_RDDISBV 0x52 //Read Display Brightness Value
#define CMD_WRCTRLD 0x53 // Write CTRL Display
//-Brightness Control Block - bit 5
//-Display Dimming			- bit 3
//-Backlight Control On/Off - bit 2
#define CMD_RDCTRLD 0x54 //Read CTRL Value Display

//ili9488 gamma
#define GAMMA_CURVE0 0x01
#define GAMMA_CURVE1 0x02
#define GAMMA_CURVE2 0x04
#define GAMMA_CURVE3 0x08

//ili9488 CTRL Display
static const uint8_t MASK_CTRLD_BCTRL = (0x01 << 5); //Brightness Control Block
/* #define MASK_CTRLD_BCTRL (0x01 << 5) //Brightness Control Block */
#define MASK_CTRLD_DD (0x01 << 3) //Display Dimming
#define MASK_CTRLD_BL (0x01 << 2) //Backlight Control

const uint8_t CMD_MADCTLRD = 0x0B;
constexpr static uint8_t CMD_NOP = 0x00;

uint8_t ili9488_flg = 0; // flags

uint16_t ili9488_x = 0;  // current x coordinate (CASET)
uint16_t ili9488_y = 0;  // current y coordinate (RASET)
uint16_t ili9488_cx = 0; //
uint16_t ili9488_cy = 0; //

namespace {
bool do_complete_lcd_reinit = false;
}

uint8_t ili9488_buff[ILI9488_COLS * 3 * ILI9488_BUFF_ROWS]; // 3 bytes for pixel color

#ifdef ILI9488_USE_RTOS
osThreadId ili9488_task_handle = 0;
#endif //ILI9488_USE_RTOS

/*some functions are in header - excluded from display_t struct*/
void ili9488_gamma_set_direct(uint8_t gamma_enu);
uint8_t ili9488_read_ctrl(void);
void ili9488_ctrl_set(uint8_t ctrl);

using namespace buddy::hw;

static void ili9488_set_cs(void) {
#if (BOARD_IS_BUDDY)
    displayCs.write(Pin::State::high);
#endif
}

static void ili9488_clr_cs(void) {
#if (BOARD_IS_BUDDY)
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
    while (c--)
        *(p++) = v;
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
#ifdef ILI9488_USE_RTOS
        osDelay(ms);
#else
        HAL_Delay(ms);
#endif
    }
}

void ili9488_spi_wr_byte(uint8_t b) {
    HAL_SPI_Transmit(ili9488_config.phspi, &b, 1, HAL_MAX_DELAY);
}

void ili9488_spi_wr_bytes(uint8_t *pb, uint16_t size) {
    if ((ili9488_flg & ILI9488_FLG_DMA) && !(ili9488_flg & ILI9488_FLG_SAFE) && (size > 4)) {
#ifdef ILI9488_USE_RTOS
        osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_TX);
        osSignalWait(ILI9488_SIG_SPI_TX, osWaitForever);
#endif //ILI9488_USE_RTOS
        HAL_SPI_Transmit_DMA(ili9488_config.phspi, pb, size);
#ifdef ILI9488_USE_RTOS
        osSignalWait(ILI9488_SIG_SPI_TX, osWaitForever);
#else  //ILI9488_USE_RTOS
//TODO:
#endif //ILI9488_USE_RTOS
    } else
        HAL_SPI_Transmit(ili9488_config.phspi, pb, size, HAL_MAX_DELAY);
}

uint32_t saved_prescaler;

void ili9488_spi_rd_bytes(uint8_t *pb, uint16_t size) {
    saved_prescaler = ili9488_config.phspi->Init.BaudRatePrescaler;
    if (HAL_SPI_DeInit(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ili9488_config.phspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;

    if (HAL_SPI_Init(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    HAL_StatusTypeDef ret;
    ret = HAL_SPI_Receive(ili9488_config.phspi, pb, size, HAL_MAX_DELAY);

    if (HAL_SPI_DeInit(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ili9488_config.phspi->Init.BaudRatePrescaler = saved_prescaler;
    if (HAL_SPI_Init(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ret = ret; //prevent warning
}

void ili9488_cmd(uint8_t cmd, uint8_t *pdata, uint16_t size) {
    ili9488_clr_cs();         // CS = L
    ili9488_clr_rs();         // RS = L
    ili9488_spi_wr_byte(cmd); // write command byte
    if (pdata && size) {
        ili9488_set_rs();                  // RS = H
        ili9488_spi_wr_bytes(pdata, size); // write data bytes
    }
    ili9488_set_cs(); // CS = H
}

void ili9488_cmd_rd(uint8_t cmd, uint8_t *pdata) {
    saved_prescaler = ili9488_config.phspi->Init.BaudRatePrescaler;
    if (HAL_SPI_DeInit(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ili9488_config.phspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;

    if (HAL_SPI_Init(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ili9488_clr_cs(); // CS = L
    ili9488_clr_rs(); // RS = L
    uint8_t data_to_write[ILI9488_MAX_COMMAND_READ_LENGHT] = { 0x00 };
    data_to_write[0] = cmd;
    data_to_write[1] = 0x00;
    HAL_SPI_TransmitReceive(ili9488_config.phspi, data_to_write, pdata, ILI9488_MAX_COMMAND_READ_LENGHT, HAL_MAX_DELAY);
    ili9488_set_cs();
    if (HAL_SPI_DeInit(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }

    ili9488_config.phspi->Init.BaudRatePrescaler = saved_prescaler;
    if (HAL_SPI_Init(ili9488_config.phspi) != HAL_OK) {
        Error_Handler();
    }
}

void ili9488_wr(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size))
        return;                        // null or empty data - return
    ili9488_clr_cs();                  // CS = L
    ili9488_set_rs();                  // RS = H
    ili9488_spi_wr_bytes(pdata, size); // write data bytes
    ili9488_set_cs();                  // CS = H
}

void ili9488_rd(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size))
        return; // null or empty data - return
    //generate little pulse on displayCs, because ILI need change displayCs logic level
    displayCs.write(Pin::State::high);
    ili9488_delay_ms(1);
    displayCs.write(Pin::State::low);

    ili9488_clr_cs();               // CS = L
    ili9488_clr_rs();               // RS = L
    ili9488_spi_wr_byte(CMD_RAMRD); // write command byte
    ili9488_spi_wr_byte(0);         // write dummy byte, datasheet p.122

    ili9488_spi_rd_bytes(pdata, size); // read data bytes
    ili9488_set_cs();                  // CS = H

    //generate little pulse on displayCs, because ILI need change displayCs logic level
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

void ili9488_cmd_madctlrd(uint8_t *pdata) {
    ili9488_cmd_rd(CMD_MADCTLRD, pdata);
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
    ili9488_clr_rst();
    ili9488_delay_ms(15);
    touch::reset_chip(ili9488_set_rst); //touch will restore reset
}

// weak functions to be used without touch driver
void __attribute__((weak)) touch::reset_chip(touch::reset_clr_fnc_t reset_clr_fnc) {
    log_info(GUI, "%s not bound to touch", __PRETTY_FUNCTION__);
    reset_clr_fnc();
}

bool __attribute__((weak)) touch::set_registers() {
    log_info(GUI, "%s not bound to touch", __PRETTY_FUNCTION__);
    return false;
}

void ili9488_set_backlight(uint8_t bck) {
}

void ili9488_set_complete_lcd_reinit() {
    do_complete_lcd_reinit = true;
}

void ili9488_init(void) {
    displayCs.write(Pin::State::low);
#ifdef ILI9488_USE_RTOS
    ili9488_task_handle = osThreadGetId();
#endif //ILI9488_USE_RTOS
    if (ili9488_flg & ILI9488_FLG_SAFE) {
        ili9488_flg &= ~ILI9488_FLG_DMA;
    } else {
        ili9488_flg = ili9488_config.flg;
    }

    if (!option::bootloader || do_complete_lcd_reinit) {
        ili9488_reset();       // 15ms reset pulse
        ili9488_delay_ms(120); // 120ms wait
        ili9488_cmd_slpout();  // wakeup
        ili9488_delay_ms(120); // 120ms wait
    }

    ili9488_cmd_madctl(ili9488_config.madctl); // interface pixel format
    ili9488_cmd_colmod(ili9488_config.colmod); // memory data access control

    if (!option::bootloader || do_complete_lcd_reinit) {
        ili9488_cmd_dispon();       // display on
        ili9488_delay_ms(10);       // 10ms wait
        ili9488_clear(COLOR_BLACK); // black screen after power on
        ili9488_delay_ms(100);      // time to set black color
    }

    if (touch::is_enabled()) {
        touch::set_registers(); // do not disable it, it is handled in GUI
    }

    ili9488_set_backlight(0xFF); // set backlight to maximum

    ili9488_inversion_on();

    do_complete_lcd_reinit = false;
}

void ili9488_done(void) {
}

void ili9488_clear(uint32_t clr666) {
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
    for (i = 0; i < ILI9488_ROWS / ILI9488_BUFF_ROWS; i++)
        ili9488_wr(ili9488_buff, sizeof(ili9488_buff));
    ili9488_set_cs();
    //	ili9488_test_miso();
}

void ili9488_set_pixel(uint16_t point_x, uint16_t point_y, uint32_t clr666) {
    ili9488_cmd_caset(point_x, point_x + 1);
    ili9488_cmd_raset(point_y, point_y + 1);
    ili9488_cmd_ramwr((uint8_t *)(&clr666), 3);
}

uint8_t *ili9488_get_block(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    if (start_x >= ILI9488_COLS || start_y >= ILI9488_ROWS || end_x >= ILI9488_COLS || end_y >= ILI9488_ROWS)
        return NULL;
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
    return ret; //directColor;
}

void ili9488_fill_rect_colorFormat666(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t clr666) {
    int i;
    uint32_t size = (uint32_t)rect_w * rect_h * 3;
    int n = size / sizeof(ili9488_buff);
    int s = size % sizeof(ili9488_buff);
    if (n)
        ili9488_fill_ui24((uint8_t *)ili9488_buff, clr666, sizeof(ili9488_buff) / 3);
    else
        ili9488_fill_ui24((uint8_t *)ili9488_buff, clr666, size / 3);
    ili9488_clr_cs();
    ili9488_cmd_caset(rect_x, rect_x + rect_w - 1);
    ili9488_cmd_raset(rect_y, rect_y + rect_h - 1);
    ili9488_cmd_ramwr(0, 0);
    for (i = 0; i < n; i++)
        ili9488_wr(ili9488_buff, sizeof(ili9488_buff));
    if (s)
        ili9488_wr(ili9488_buff, s);
    ili9488_set_cs();
}

void ili9488_draw_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    ili9488_clr_cs();
    ili9488_cmd_caset(x, x + w - 1);
    ili9488_cmd_raset(y, y + h - 1);
    ili9488_cmd_ramwr(ili9488_buff, 3 * w * h);
    ili9488_set_cs();
}

#ifdef ILI9488_PNG_SUPPORT

    #include <png.h>
    #include <optional>
    #include "scratch_buffer.hpp"
    #include "bsod_gui.hpp"

std::optional<buddy::scratch_buffer::Ownership> png_memory;
uint32_t png_mem_total = 0;
void *png_mem_ptrs[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_sizes[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_cnt = 0;

png_voidp _pngmalloc(png_structp pp, png_alloc_size_t size) {
    if (!png_memory.has_value()) {
        png_memory = buddy::scratch_buffer::Ownership();
        png_memory->acquire(/*wait=*/true);
    }
    if (png_mem_total + size >= png_memory->get().size()) {
        fatal_error(ErrCode::ERR_SYSTEM_PNG_MALLOC_ERROR);
    }
    int i;
    void *p = ((uint8_t *)png_memory->get().buffer) + png_mem_total;
    {
        for (i = 0; i < 10; i++)
            if (png_mem_ptrs[i] == 0)
                break;
        png_mem_ptrs[i] = p;
        png_mem_sizes[i] = size;
        png_mem_total += size;
        png_mem_cnt++;
    }
    return p;
}

void _pngfree(png_structp pp, png_voidp mem) {
    int i;

    for (i = 0; i < 10; i++)
        if (mem == png_mem_ptrs[i]) {
            uint32_t size = png_mem_sizes[i];
            png_mem_ptrs[i] = 0;
            png_mem_sizes[i] = 0;
            png_mem_total -= size;
            png_mem_cnt--;
        }

    if (png_mem_cnt == 0) {
        png_memory->release();
    }
}

using raster_fn = void (*)(uint8_t *);

template <bool ALPHA, raster_fn FN>
static inline void fill_buffer(uint32_t back_color, int png_rows_to_read_in_this_loop, uint16_t png_cols, int pixsize, uint8_t *ili9488_buff) {
    for (int j = 0; j < png_rows_to_read_in_this_loop * png_cols; j++) {
        uint8_t *ppx666 = (uint8_t *)(ili9488_buff + j * 3);
        uint8_t *ppx888 = (uint8_t *)(ili9488_buff + j * pixsize);

        // if constexpr (FN != nullptr) generates warning, so I will use std::is_same_v instead
        using TypeOfNull = std::integral_constant<decltype(FN), nullptr>;
        using TypeOfFN = std::integral_constant<decltype(FN), FN>;

        if constexpr (!std::is_same_v<TypeOfNull, TypeOfFN>)
            FN(ppx888);

        if constexpr (ALPHA) {
            const uint32_t clr = color_alpha(back_color, color_rgb(ppx888[0], ppx888[1], ppx888[2]), ppx888[3]);
            memcpy(ppx888, &clr, sizeof(clr));
        }

        const uint32_t clr = color_to_666(color_rgb(ppx888[0], ppx888[1], ppx888[2]));
        memcpy(ppx666, &clr, sizeof(clr));
    }
}

    #define FILL_BUFFER(FN) alphaChannel ? fill_buffer<true, FN>(back_color, png_rows_to_read_in_this_loop, png_cols, pixsize, ili9488_buff) : fill_buffer<false, FN>(back_color, png_rows_to_read_in_this_loop, png_cols, pixsize, ili9488_buff);

/**
 * @brief draw png from FILE
 *
 * @param pf                    pointer to file
 * @param point_x               X coordinate where to draw png on screen
 * @param point_y               Y coordinate where to draw png on screen
 * @param back_color            color of background
 * @param rop                   raster operations
 * @param subrect               sub rectangle inside png - area to draw
 */
void ili9488_draw_png_ex(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, uint8_t rop, Rect16 subrect) {
    PNGMeasure PM;
    if ((point_x >= ILI9488_COLS) || (point_y >= ILI9488_ROWS))
        return;
    png_structp pp = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, _pngmalloc, _pngfree);
    //png_set_mem_fn(pp, 0, _pngmalloc, _pngfree);
    if (pp == NULL)
        goto _e_0;
    { // explicitly limit the scope of the local vars in order to enable use of goto
        // which is the preffered error recovery system in libpng
        // the vars must be destroyed before label _e_0
        png_infop ppi = png_create_info_struct(pp);
        if (ppi == NULL)
            goto _e_1;
        if (setjmp(png_jmpbuf(pp)))
            goto _e_1;
        { // explicitly limit the scope of the local vars in order to enable use of goto
            // which is the preffered error recovery system in libpng
            // the vars must be destroyed before label _e_0
            png_init_io(pp, pf);
            {
                uint8_t sig[8];
                if (fread(sig, 1, 8, pf) < 8)
                    goto _e_1;
                if (!png_check_sig(sig, 8))
                    goto _e_1; /* bad signature */
                png_set_sig_bytes(pp, 8);
            }
            png_read_info(pp, ppi);
            uint16_t w = png_get_image_width(pp, ppi);
            uint16_t h = png_get_image_height(pp, ppi);
            int rowsize = png_get_rowbytes(pp, ppi);

            //check image type (indexed or other color type)
            png_byte colorType = png_get_color_type(pp, ppi);
            bool alphaChannel = false;

            switch (colorType) {
            case PNG_COLOR_TYPE_GRAY:
                //transform image from grayscale to rgb or rgba
                png_set_gray_to_rgb(pp);
                //check if alpha channel is present
                if (png_get_valid(pp, ppi, PNG_INFO_tRNS)) {
                    png_set_tRNS_to_alpha(pp);
                    alphaChannel = true;
                }
                break;
            case PNG_COLOR_TYPE_PALETTE:
                //transform image from palette to rgb or rgba
                png_set_palette_to_rgb(pp);
                //check if alpha channel is present if yes then add it
                if (png_get_valid(pp, ppi, PNG_INFO_tRNS)) {
                    png_set_tRNS_to_alpha(pp);
                    alphaChannel = true;
                }
                break;
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                //transform image from grayscale with alpha to rgba
                png_set_gray_to_rgb(pp);
                alphaChannel = true;
                break;
            case PNG_COLOR_TYPE_RGB_ALPHA:
                alphaChannel = true;
                break;

            default:
                break;
            }

            //if no alpha channel is present (3 bytes per pixel), add padding to 4 pixels
            //if we used only 3 bytes, we would overwrite first byte of next pixel
            if (!alphaChannel) {
                png_set_filler(pp, 0x00, PNG_FILLER_AFTER);
            }

            //Pixsize is always 4 bytes. rgba is 4 bytes wide, if PNG is only rgb, we add padding to 4 bytes
            const int pixsize = 4;

            //_dbg("ili9488_draw_png rowsize = %i", rowsize);
            if (rowsize > ILI9488_COLS * 4)
                goto _e_1;

            //target coordinates have to be substracted form display sizes to be able to fit it on the screen
            const Rect16 MaxSubrect(0, 0, MIN(w, ILI9488_COLS - point_x), MIN(h, ILI9488_ROWS - point_y));

            // recalculate subrect
            if (subrect.IsEmpty())
                subrect = MaxSubrect;
            else
                subrect.Intersection(MaxSubrect);

            // make there is something to print
            if (subrect.IsEmpty()) {
                goto _e_1;
            }

            //_dbg("ili9488_draw_png pixsize = %i", pixsize);
            ili9488_clr_cs();
            if (setjmp(png_jmpbuf(pp)))
                goto _e_2;

            {
                Rect16 print_rect(subrect.Left() + point_x, subrect.Top() + point_y, subrect.Width(), subrect.Height());
                const uint16_t x_end = print_rect.Left() + print_rect.Width();
                const uint16_t y_end = print_rect.Top() + print_rect.Height();

                ili9488_cmd_caset(print_rect.Left(), x_end - 1);
                ili9488_cmd_raset(print_rect.Top(), y_end - 1);
                ili9488_cmd_ramwr(0, 0);

                uint16_t png_rows_left = print_rect.Height();
                uint16_t png_cols = print_rect.Width();
                uint16_t buff_row_space = (ILI9488_BUFF_ROWS * ILI9488_COLS * 3) / (png_cols * pixsize);

                // Skip unwanted PNG rows (reloative to subrect)
                for (int k = 0; k < subrect.Top(); k++) {
                    // png is compressed, there is no other way to access specific row
                    png_read_row(pp, ili9488_buff, NULL); // Not using png_read_rows() because ili9488_buff is a 1D array (Not 2D)
                }

                while (png_rows_left) {

                    // Calculate how many rows to draw in this cycle
                    int png_rows_to_read_in_this_loop = png_rows_left < buff_row_space ? png_rows_left : buff_row_space;

                    // Load as many png rows to ili9488 buffer as possible
                    // it is more effitient to have 2 for cycles inside if/else than other way around
                    if (subrect.Left() == 0 && subrect.Width() <= w) { // subrect is correct here, not a bug
                        for (int i = 0; i < png_rows_to_read_in_this_loop; i++) {
                            png_read_row(pp, ili9488_buff + i * png_cols * pixsize, NULL);
                        }
                    } else {
                        for (int i = 0; i < png_rows_to_read_in_this_loop; i++) {
                            // Skip unwanted cols from the row (reloative to subrect)
                            uint8_t row[w * pixsize] = { 0 };
                            png_read_row(pp, row, NULL);
                            memcpy(ili9488_buff + i * png_cols * pixsize, row + subrect.Left() * pixsize, png_cols * pixsize);
                        }
                    }

                    // Manipulate all loaded pixels
                    switch (rop) {
                    case ROPFN_INVERT:
                        FILL_BUFFER(rop_rgb888_invert);
                        break;
                    case ROPFN_SWAPBW:
                        FILL_BUFFER(rop_rgb888_swapbw);
                        break;
                    case ROPFN_SHADOW:
                        FILL_BUFFER(rop_rgb888_disabled);
                        break;
                    case ROPFN_DESATURATE:
                        FILL_BUFFER(rop_rgb888_desaturate);
                        break;
                    case ROPFN_SWAPBW | ROPFN_SHADOW:
                    default:
                        FILL_BUFFER(nullptr); // fill buffer while using no raster function
                    }

                    // Display pixels with minimal SPI overhead
                    ili9488_wr(ili9488_buff, 3 * png_cols * png_rows_to_read_in_this_loop);
                    png_rows_left -= png_rows_to_read_in_this_loop;
                }
            }
        _e_2:
            ili9488_set_cs();
        }
    _e_1:
        png_destroy_read_struct(&pp, &ppi, 0);
    }
_e_0:
    return;
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
    //faster code if CMD_INVON == CMD_INVOFF + 1
    //The result of the logical negation operator ! is 1 if the value of its operand is 0,
    //0 if the value of its operand is non-zero.
    ili9488_config.is_inverted = !ili9488_inversion_get();
    ili9488_cmd(CMD_INVOFF + ili9488_config.is_inverted, 0, 0);
    #else
    //to be portable
    if (ili9488_inversion_get())
        ili9488_inversion_off();
    else
        ili9488_inversion_on();

    #endif
}
uint8_t ili9488_inversion_get(void) {
    return ili9488_config.is_inverted;
}

//0x01 -> 0x02 -> 0x04 -> 0x08 -> 0x01
void ili9488_gamma_next(void) {
    ili9488_gamma_set_direct(((ili9488_config.gamma << 1) | (ili9488_config.gamma >> 3)) & 0x0f);
}

//0x01 -> 0x08 -> 0x04 -> 0x02 -> 0x01
void ili9488_gamma_prev(void) {
    ili9488_gamma_set_direct(((ili9488_config.gamma << 3) | (ili9488_config.gamma >> 1)) & 0x0f);
}

//use GAMMA_CURVE0 - GAMMA_CURVE3
void ili9488_gamma_set_direct(uint8_t gamma_enu) {
    ili9488_config.gamma = gamma_enu;
    ili9488_cmd(CMD_GAMMA_SET, &ili9488_config.gamma, sizeof(ili9488_config.gamma));
}

//use 0 - 3
void ili9488_gamma_set(uint8_t gamma) {
    if (gamma != ili9488_gamma_get())
        ili9488_gamma_set_direct(1 << (gamma & 0x03));
}

//returns 0 - 3
uint8_t ili9488_gamma_get() {
    uint8_t position = 0;
    for (int8_t position = 3; position >= 0; --position) {
        if (ili9488_config.gamma == 1 << position)
            break;
    }

    return position;
}

void ili9488_brightness_enable(void) {
    ili9488_ctrl_set(ili9488_config.control | MASK_CTRLD_BCTRL);
}

void ili9488_brightness_disable(void) {
    ili9488_ctrl_set(ili9488_config.control & (~MASK_CTRLD_BCTRL));
}

void ili9488_brightness_set(uint8_t brightness) {
    ili9488_config.brightness = brightness;
    //set brightness
    ili9488_cmd(CMD_WRDISBV, &ili9488_config.brightness, sizeof(ili9488_config.brightness));
}

uint8_t ili9488_brightness_get(void) {
    return ili9488_config.brightness;
}

void ili9488_ctrl_set(uint8_t ctrl) {
    ili9488_config.control = ctrl;
    ili9488_cmd(CMD_WRCTRLD, &ili9488_config.control, sizeof(ili9488_config.control));
}

#else //ILI9488_PNG_SUPPORT

void ili9488_draw_png_ex(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, uint8_t rop, Rect16 subrect) {}

#endif //ILI9488_PNG_SUPPORT

ili9488_config_t ili9488_config = {
    0,            // spi handle pointer
    0,            // flags (DMA, MISO)
    0,            // interface pixel format (5-6-5, hi-color)
    0,            // memory data access control (no mirror XY)
    GAMMA_CURVE0, // gamma curve
    0,            // brightness
    0,            // inverted
    0,            // default control reg value
};

//! @brief enable safe mode (direct acces + safe delay)
void ili9488_enable_safe_mode(void) {
    ili9488_flg |= ILI9488_FLG_SAFE;
}

void ili9488_spi_tx_complete(void) {
#ifdef ILI9488_USE_RTOS
    osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_TX);
#endif //ILI9488_USE_RTOS
}

void ili9488_spi_rx_complete(void) {
#ifdef ILI9488_USE_RTOS
    osSignalSet(ili9488_task_handle, ILI9488_SIG_SPI_RX);
#endif // ILI9488_USE_RTOS
}

void ili9488_cmd_nop() {
    ili9488_cmd(CMD_NOP, 0, 0);
}
