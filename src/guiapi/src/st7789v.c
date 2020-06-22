// st7789v.c
#include "st7789v.h"

#include <guiconfig.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "cmath_ext.h"
#ifdef ST7789V_USE_RTOS
    #include "cmsis_os.h"
#endif //ST7789V_USE_RTOS

//st7789 commands
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

//st7789 gamma
#define GAMMA_CURVE0 0x01
#define GAMMA_CURVE1 0x02
#define GAMMA_CURVE2 0x04
#define GAMMA_CURVE3 0x08

//st7789 CTRL Display
#define MASK_CTRLD_BCTRL (0x01 << 5) //Brightness Control Block
#define MASK_CTRLD_DD    (0x01 << 3) //Display Dimming
#define MASK_CTRLD_BL    (0x01 << 2) //Backlight Control

//color constants
#define CLR565_WHITE   0xffff
#define CLR565_BLACK   0x0000
#define CLR565_RED     0xf800
#define CLR565_CYAN    0x0000
#define CLR565_MAGENTA 0x0000
#define CLR565_GREEN   0x07e0
#define CLR565_YELLOW  0xffe0
#define CLR565_ORANGE  0x0000
#define CLR565_GRAY    0x38e7
#define CLR565_BLUE    0x001f

//color conversion
#define COLOR_TO_565(clr) color_to_565(clr)
//color reverse conversion
#define C565_TO_COLOR(clr) color_from_565(clr)

//private flags (pin states)
#define FLG_CS  0x01 // current CS pin state
#define FLG_RS  0x02 // current RS pin state
#define FLG_RST 0x04 // current RST pin state

uint8_t st7789v_flg = 0; // flags

uint16_t st7789v_x = 0;  // current x coordinate (CASET)
uint16_t st7789v_y = 0;  // current y coordinate (RASET)
uint16_t st7789v_cx = 0; //
uint16_t st7789v_cy = 0; //

uint8_t st7789v_buff[ST7789V_COLS * 2 * 16]; //16 lines buffer

rect_ui16_t st7789v_clip = { 0, 0, ST7789V_COLS, ST7789V_ROWS };

#ifdef ST7789V_USE_RTOS
osThreadId st7789v_task_handle = 0;
#endif //ST7789V_USE_RTOS

/*some functions are in header - excluded from display_t struct*/
void st7789v_gamma_set_direct(uint8_t gamma_enu);
uint8_t st7789v_read_ctrl(void);
void st7789v_ctrl_set(uint8_t ctrl);

static inline void st7789v_set_cs(void) {
    gpio_set(st7789v_config.pinCS, 1);
    st7789v_flg |= FLG_CS;
}

static inline void st7789v_clr_cs(void) {
    gpio_set(st7789v_config.pinCS, 0);
    st7789v_flg &= ~FLG_CS;
}

static inline void st7789v_set_rs(void) {
    gpio_set(st7789v_config.pinRS, 1);
    st7789v_flg |= FLG_RS;
}

static inline void st7789v_clr_rs(void) {
    gpio_set(st7789v_config.pinRS, 0);
    st7789v_flg &= ~FLG_RS;
}

static inline void st7789v_set_rst(void) {
    gpio_set(st7789v_config.pinRST, 1);
    st7789v_flg |= FLG_RST;
}

static inline void st7789v_clr_rst(void) {
    gpio_set(st7789v_config.pinRST, 0);
    st7789v_flg &= ~FLG_RST;
}

static inline void st7789v_fill_ui16(uint16_t *p, uint16_t v, uint16_t c) {
    while (c--)
        *(p++) = v;
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void st7789v_delay_ms(uint32_t ms) {
    if (is_interrupt() || (st7789v_flg & ST7789V_FLG_SAFE)) {
        volatile uint32_t temp;
        while (ms--) {
            do {
                temp = SysTick->CTRL;
            } while ((temp & 0x01) && !(temp & (1 << 16)));
        }
    } else {
#ifdef ST7789V_USE_RTOS
        osDelay(ms);
#else
        HAL_Delay(ms);
#endif
    }
}

void st7789v_spi_wr_byte(uint8_t b) {
    HAL_SPI_Transmit(st7789v_config.phspi, &b, 1, HAL_MAX_DELAY);
}

void st7789v_spi_wr_bytes(uint8_t *pb, uint16_t size) {
    if ((st7789v_flg & ST7789V_FLG_DMA) && !(st7789v_flg & ST7789V_FLG_SAFE) && (size > 4)) {
#ifdef ST7789V_USE_RTOS
        osSignalSet(st7789v_task_handle, ST7789V_SIG_SPI_TX);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
#endif //ST7789V_USE_RTOS
        HAL_SPI_Transmit_DMA(st7789v_config.phspi, pb, size);
#ifdef ST7789V_USE_RTOS
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
#else  //ST7789V_USE_RTOS
//TODO:
#endif //ST7789V_USE_RTOS
    } else
        HAL_SPI_Transmit(st7789v_config.phspi, pb, size, HAL_MAX_DELAY);
}

void st7789v_spi_rd_bytes(uint8_t *pb, uint16_t size) {
    HAL_StatusTypeDef ret;
#if 0
//#ifdef ST7789V_DMA
    if (size <= 4)
        ret = HAL_SPI_Receive(st7789v_config.phspi, pb, size, HAL_MAX_DELAY);
    else
    {
    #ifdef ST7789V_USE_RTOS
        osSignalSet(0, ST7789V_SIG_SPI_TX);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    #endif //ST7789V_USE_RTOS
        ret = HAL_SPI_Receive_DMA(st7789v_config.phspi, pb, size);
    #ifdef ST7789V_USE_RTOS
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    #endif     //ST7789V_USE_RTOS
    }
#else          //ST7789V_DMA
    ret = HAL_SPI_Receive(st7789v_config.phspi, pb, size, HAL_MAX_DELAY);
#endif         //ST7789V_DMA
    ret = ret; //prevent warning
}

void st7789v_cmd(uint8_t cmd, uint8_t *pdata, uint16_t size) {
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS)
        st7789v_clr_cs(); // CS = L
    if (st7789v_flg & FLG_RS)
        st7789v_clr_rs();     // RS = L
    st7789v_spi_wr_byte(cmd); // write command byte
    if (pdata && size) {
        st7789v_set_rs();                  // RS = H
        st7789v_spi_wr_bytes(pdata, size); // write data bytes
    }
    if (tmp_flg & FLG_CS)
        st7789v_set_cs(); // CS = H
}

void st7789v_wr(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size))
        return;                     // null or empty data - return
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS)
        st7789v_clr_cs(); // CS = L
    if (!(st7789v_flg & FLG_RS))
        st7789v_set_rs();              // RS = H
    st7789v_spi_wr_bytes(pdata, size); // write data bytes
    if (tmp_flg & FLG_CS)
        st7789v_set_cs(); // CS = H
}

void st7789v_rd(uint8_t *pdata, uint16_t size) {
    if (!(pdata && size))
        return;                     // null or empty data - return
    uint16_t tmp_flg = st7789v_flg; // save flags
    if (st7789v_flg & FLG_CS)
        st7789v_clr_cs(); // CS = L
    if (!(st7789v_flg & FLG_RS))
        st7789v_set_rs();              // RS = H
    st7789v_spi_rd_bytes(pdata, size); // read data bytes
    if (tmp_flg & FLG_CS)
        st7789v_set_cs(); // CS = H
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
    uint8_t data[4] = { x >> 8, x & 0xff, cx >> 8, cx & 0xff };
    st7789v_cmd(CMD_CASET, data, 4);
}

void st7789v_cmd_raset(uint16_t y, uint16_t cy) {
    uint8_t data[4] = { y >> 8, y & 0xff, cy >> 8, cy & 0xff };
    st7789v_cmd(CMD_RASET, data, 4);
}

void st7789v_cmd_ramwr(uint8_t *pdata, uint16_t size) {
    st7789v_cmd(CMD_RAMWR, pdata, size);
}

void st7789v_cmd_ramrd(uint8_t *pdata, uint16_t size) {
    st7789v_cmd(CMD_RAMRD, 0, 0);
    st7789v_rd(pdata, size);
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
    if (!(st7789v_flg & ST7789V_FLG_SAFE)) {
        gpio_init(st7789v_config.pinCS, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
        gpio_init(st7789v_config.pinRS, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
        gpio_init(st7789v_config.pinRST, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    }
    st7789v_flg &= ~(FLG_CS | FLG_RS | FLG_RST);
    st7789v_set_rst();
    st7789v_set_cs();
    st7789v_set_rs();
}

void st7789v_reset(void) {
    st7789v_clr_rst();
    st7789v_delay_ms(15);
    gpio_init(st7789v_config.pinRST, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
    volatile uint16_t delay = 0;
    int irq = __get_PRIMASK() & 1;
    if (irq)
        __disable_irq();
    while (!gpio_get(st7789v_config.pinRST))
        delay++;
    if (irq)
        __enable_irq();
    gpio_init(st7789v_config.pinRST, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    st7789v_set_rst();
    st7789v_reset_delay = delay;
}

void st7789v_init(void) {
#ifdef ST7789V_USE_RTOS
    st7789v_task_handle = osThreadGetId();
#endif //ST7789V_USE_RTOS
    if (st7789v_flg & ST7789V_FLG_SAFE)
        st7789v_flg &= ~ST7789V_FLG_DMA;
    else
        st7789v_flg = st7789v_config.flg;
    st7789v_init_ctl_pins();                   // CS=H, RS=H, RST=H
    st7789v_reset();                           // 15ms reset pulse
    st7789v_delay_ms(120);                     // 120ms wait
    st7789v_cmd_slpout();                      // wakeup
    st7789v_delay_ms(120);                     // 120ms wait
    st7789v_cmd_madctl(st7789v_config.madctl); // interface pixel format
    st7789v_cmd_colmod(st7789v_config.colmod); // memory data access control
    st7789v_cmd_dispon();                      // display on
    st7789v_delay_ms(10);                      // 10ms wait
}

void st7789v_done(void) {
}

/// Fills screen by this color
void st7789v_clear(color_t clr) {
    // FIXME similar to st7789v_fill_rect; join?
    int i;
    const uint16_t clr565 = COLOR_TO_565(clr);
    for (i = 0; i < ST7789V_COLS * 16; i++)
        ((uint16_t *)st7789v_buff)[i] = clr565;
    st7789v_clr_cs();
    st7789v_cmd_caset(0, ST7789V_COLS - 1);
    st7789v_cmd_raset(0, ST7789V_ROWS - 1);
    st7789v_cmd_ramwr(0, 0);
    for (i = 0; i < ST7789V_ROWS / 16; i++)
        st7789v_wr(st7789v_buff, 2 * ST7789V_COLS * 16);
    st7789v_set_cs();
    //	st7789v_test_miso();
}

/// Turns the specified pixel to the specified color
void st7789v_set_pixel(point_ui16_t pt, color_t clr) {
    if (!point_in_rect_ui16(pt, st7789v_clip))
        return;
    uint16_t clr565 = COLOR_TO_565(clr);
    st7789v_cmd_caset(pt.x, 1);
    st7789v_cmd_raset(pt.y, 1);
    st7789v_cmd_ramwr((uint8_t *)(&clr565), 2);
}

color_t st7789v_get_pixel(point_ui16_t pt) {
    if (!point_in_rect_ui16(pt, st7789v_clip))
        return 0;
    uint16_t clr565;
    st7789v_cmd_caset(pt.x, 1);
    st7789v_cmd_raset(pt.y, 1);
    st7789v_cmd_ramrd((uint8_t *)(&clr565), 2);
    return C565_TO_COLOR(clr565);
}

void st7789v_set_pixel_directColor(point_ui16_t pt, uint16_t directColor) {
    if (!point_in_rect_ui16(pt, st7789v_clip))
        return;
    st7789v_cmd_caset(pt.x, 1);
    st7789v_cmd_raset(pt.y, 1);
    st7789v_cmd_ramwr((uint8_t *)(&directColor), 2);
}

//1 == 0000 0001, 5 == 0001 1111
static uint16_t set_num_of_ones(uint8_t num_of_ones) {
    return ~((uint16_t)(-1) << num_of_ones);
}

//BYTEs 0 and 1 are empty
//bit0 -> bit5 BYTE2
//bit1 -> bit6 BYTE2
//bit2 -> bit7 BYTE2

//bit3 -> bit3 BYTE4
//bit4 -> bit4 BYTE4
//bit5 -> bit5 BYTE4
//bit6 -> bit6 BYTE4
//bit7 -> bit7 BYTE4

//bit8 -> bit3 BYTE3
//bit9 -> bit4 BYTE3
//bit10-> bit5 BYTE3
//bit11-> bit6 BYTE3
//bit12-> bit7 BYTE3

//bit13-> bit2 BYTE3
//bit14-> bit3 BYTE3
//bit15-> bit4 BYTE2

//since BYTE0 and BYTE1 are empty, buff[0] == BYTE2
static uint16_t rd18bit_to_16bit(uint8_t *buff) {
    return ((uint16_t)(buff[0] >> 5) & set_num_of_ones(3)) | (((uint16_t)(buff[2] >> 3) & set_num_of_ones(5)) << 3) | (((uint16_t)(buff[1] >> 3) & set_num_of_ones(5)) << 8) | (((uint16_t)(buff[0] >> 2) & set_num_of_ones(3)) << 13);
}

uint16_t st7789v_get_pixel_directColor(point_ui16_t pt) {
    if (!point_in_rect_ui16(pt, st7789v_clip))
        return 0;
    enum { buff_sz = 5 };
    uint8_t buff[buff_sz];
    st7789v_cmd_caset(pt.x, 1);
    st7789v_cmd_raset(pt.y, 1);
    st7789v_cmd_ramrd(buff, buff_sz);
    uint16_t ret = rd18bit_to_16bit(buff + 2);
    return ret; //directColor;
}

void st7789v_clip_rect(rect_ui16_t rc) {
    st7789v_clip = rc;
}

/// Draws simple line (no antialiasing)
/// Both end points are drawn
void st7789v_draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr) {
    int n;
    const int dx = pt1.x - pt0.x;
    const int dy = pt1.y - pt0.y;
    int cx = ABS(dx);
    int cy = ABS(dy);
    const int adx = cx; // absolute difference in x ( = width - 1)
    const int ady = cy; // absolute difference in y ( = height - 1)

    if ((adx == 0) || (ady == 0)) { // orthogonal line
        st7789v_fill_rect(rect_ui16(MIN(pt0.x, pt1.x), MIN(pt0.y, pt1.y), adx + 1, ady + 1), clr);
        return;
    }

    const int sx = SIGN1(dx);
    const int sy = SIGN1(dy);
    //FIXME every st7789v_set_pixel call checks if point is not outside the screen - performance issue

    if (adx > ady) { // likely vertical line
        for (n = adx; n > 0; --n) {
            st7789v_set_pixel(pt0, clr);
            if ((cx -= cy) <= 0) {
                pt0.y += sy;
                cx += adx;
            }
            pt0.x += sx;
        }
        return;
    }

    if (adx < ady) { // likely horizontal line
        for (n = ady; n > 0; --n) {
            st7789v_set_pixel(pt0, clr);
            if ((cy -= cx) <= 0) {
                pt0.x += sx;
                cy += ady;
            }
            pt0.y += sy;
        }
        return;
    }

    //adx == ady => diagonal line
    for (n = adx; n > 0; --n) {
        st7789v_set_pixel(pt0, clr);
        pt0.x += sx;
        pt0.y += sy;
    }
}

/// Draws a rectangle boundary of defined color
void st7789v_draw_rect(rect_ui16_t rc, color_t clr) {
    if (rc.w <= 0 || rc.h <= 0)
        return;

    point_ui16_t pt0 = { rc.x, rc.y };
    point_ui16_t pt1 = { rc.x + rc.w - 1, rc.y };
    point_ui16_t pt2 = { rc.x, rc.y + rc.h - 1 };

    st7789v_fill_rect(rect_ui16(pt0.x, pt0.y, rc.w, 1), clr); // top
    st7789v_fill_rect(rect_ui16(pt0.x, pt0.y, 1, rc.h), clr); // left
    st7789v_fill_rect(rect_ui16(pt1.x, pt1.y, 1, rc.h), clr); // right
    st7789v_fill_rect(rect_ui16(pt2.x, pt2.y, rc.w, 1), clr); // bottom
}

/// Draws a solid rectangle of defined color
void st7789v_fill_rect(rect_ui16_t rc, color_t clr) {
    rc = rect_intersect_ui16(rc, st7789v_clip);
    if (rect_empty_ui16(rc))
        return;

    uint16_t clr565 = COLOR_TO_565(clr);
    uint32_t size = (uint32_t)rc.w * rc.h * 2; // area of rectangle

    st7789v_fill_ui16((uint16_t *)st7789v_buff, clr565, MIN(size, sizeof(st7789v_buff) / 2));
    st7789v_clr_cs();
    st7789v_cmd_caset(rc.x, rc.x + rc.w - 1);
    st7789v_cmd_raset(rc.y, rc.y + rc.h - 1);
    st7789v_cmd_ramwr(0, 0);

    for (int i = 0; i < size / sizeof(st7789v_buff); i++) // writer buffer by buffer
        st7789v_wr(st7789v_buff, sizeof(st7789v_buff));

    st7789v_wr(st7789v_buff, size % sizeof(st7789v_buff)); // write the remainder data
    st7789v_set_cs();
}

/// Draws a single character according to selected font
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// If font is not available for the character, solid rectangle will be drawn in background color
/// \returns true if character is available in the font and was drawn
bool st7789v_draw_char(point_ui16_t pt, char chr, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height

    // character out of font range, display solid rectangle instead
    if ((chr < pf->asc_min) || (chr > pf->asc_max)) {
        st7789v_fill_rect(rect_ui16(pt.x, pt.y, w, h), clr_bg);
        return false;
    }

    int i;
    int j;
    uint8_t *pch;    //character data pointer
    uint8_t crd = 0; //current row byte data
    uint8_t rb;      //row byte
    uint16_t *p = (uint16_t *)st7789v_buff;
    uint8_t *pc;

    const uint8_t bpr = pf->bpr;        //bytes per row
    const uint16_t bpc = bpr * h;       //bytes per char
    const uint8_t bpp = 8 * bpr / w;    //bits per pixel
    const uint8_t ppb = 8 / bpp;        //pixels per byte
    const uint8_t pms = (1 << bpp) - 1; //pixel mask

    pch = (uint8_t *)(pf->pcs) + ((chr - pf->asc_min) * bpc);
    uint16_t clr565[16];
    for (i = 0; i <= pms; i++)
        clr565[i] = COLOR_TO_565(color_alpha(clr_bg, clr_fg, 255 * i / pms));
    for (j = 0; j < h; j++) {
        pc = pch + j * bpr;
        for (i = 0; i < w; i++) {
            if ((i % ppb) == 0) {
                if (pf->flg & FONT_FLG_SWAP) {
                    rb = (i / ppb) ^ 1;
                    crd = pch[rb + j * bpr];
                } else
                    crd = *(pc++);
            }
            if (pf->flg & FONT_FLG_LSBF) {
                *(p++) = clr565[crd & pms];
                crd >>= bpp;
            } else {
                *(p++) = clr565[crd >> (8 - bpp)];
                crd <<= bpp;
            }
        }
    }
    st7789v_clr_cs();
    st7789v_cmd_caset(pt.x, pt.x + w - 1);
    st7789v_cmd_raset(pt.y, pt.y + h - 1);
    st7789v_cmd_ramwr(st7789v_buff, 2 * w * h);
    st7789v_set_cs();

    return true;
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns true if whole text was written
bool st7789v_draw_text(rect_ui16_t rc, const char *str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.x;
    int y = rc.y;

    const uint16_t rc_end_x = rc.x + rc.w;
    const uint16_t rc_end_y = rc.y + rc.h;
    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height

    for (int i = 0; i < strlen(str); i++) {
        const char c = str[i];
        if (c == '\n') {
            y += h;
            x = rc.x;
            if (y + h > rc_end_y)
                return false;
            continue;
        }

        st7789v_draw_char(point_ui16(x, y), c, pf, clr_bg, clr_fg);
        x += w;
        // FIXME Shouldn't it try to break the line first?
        if (x + w > rc_end_x)
            return false;
    }
    return true;
}

static inline void rop_rgb888_invert(uint8_t *ppx888) {
    ppx888[0] = 255 - ppx888[0];
    ppx888[1] = 255 - ppx888[1];
    ppx888[2] = 255 - ppx888[2];
}

#define SWAPBW_TOLERANCE 64
static inline void rop_rgb888_swapbw(uint8_t *ppx888) {
    const uint8_t r = ppx888[0];
    const uint8_t g = ppx888[1];
    const uint8_t b = ppx888[2];
    const uint8_t l = (r + g + b) / 3;
    const uint8_t l0 = (l >= SWAPBW_TOLERANCE) ? (l - SWAPBW_TOLERANCE) : 0;
    const uint8_t l1 = (l <= (255 - SWAPBW_TOLERANCE)) ? (l + SWAPBW_TOLERANCE) : 255;

    if ((l0 <= r) && (r <= l1) && (l0 <= g) && (g <= l1) && (l0 <= b) && (b <= l1)) {
        ppx888[0] = 255 - r;
        ppx888[1] = 255 - g;
        ppx888[2] = 255 - b;
    }
}

static inline void rop_rgb888_disabled(uint8_t *ppx888) {
    const uint8_t r = ppx888[0];
    const uint8_t g = ppx888[1];
    const uint8_t b = ppx888[2];
    const uint8_t l = (r + g + b) / 3;
    const uint8_t l0 = (l >= SWAPBW_TOLERANCE) ? (l - SWAPBW_TOLERANCE) : 0;
    const uint8_t l1 = (l <= (255 - SWAPBW_TOLERANCE)) ? (l + SWAPBW_TOLERANCE) : 255;
    if ((l0 <= r) && (r <= l1) && (l0 <= g) && (g <= l1) && (l0 <= b) && (b <= l1)) {
        ppx888[0] = r / 2;
        ppx888[1] = g / 2;
        ppx888[2] = b / 2;
    }
}

static inline void rop_rgb8888_swapbw(uint8_t *ppx) {
    const uint8_t r = ppx[0];
    const uint8_t g = ppx[1];
    const uint8_t b = ppx[2];
    const uint8_t a = ppx[3];
    if ((r == g) && (r == b)) {
        ppx[0] = 255 - r;
        ppx[1] = 255 - g;
        ppx[2] = 255 - b;
        if (/*(r < 32) && */ (a < 128)) {
            ppx[0] = 0;
            ppx[1] = 0;
            ppx[2] = 255;
            ppx[3] = 255;
        }
    } else if (a < 128) {
        ppx[0] = 0;
        ppx[1] = 255;
        ppx[2] = 0;
        ppx[3] = 255;
    }
}

void st7789v_draw_png_ex(point_ui16_t pt, FILE *pf, color_t clr0, uint8_t rop);

void st7789v_draw_icon(point_ui16_t pt, uint16_t id_res, color_t clr0, uint8_t rop) {
    FILE *pf = resource_fopen(id_res, "rb");
    st7789v_draw_png_ex(pt, pf, clr0, rop);
    fclose(pf);
}

#ifdef ST7789V_PNG_SUPPORT

    #include <png.h>

void *png_mem_ptr0 = 0;
uint32_t png_mem_total = 0;
uint32_t png_mem_max = 0;
void *png_mem_ptrs[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_sizes[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_cnt = 0;

png_voidp _pngmalloc(png_structp pp, png_alloc_size_t size) {
    //	return malloc(size);
    //	return pvPortMalloc(size);
    if (png_mem_ptr0 == 0)
        //png_mem_ptr0 = pvPortMalloc(0xc000); //48k
        png_mem_ptr0 = (void *)0x10000000; //ccram
    int i;
    void *p = ((uint8_t *)png_mem_ptr0) + png_mem_total;
    //	if (p == 0)
    //		while (1);
    //	else
    {
        for (i = 0; i < 10; i++)
            if (png_mem_ptrs[i] == 0)
                break;
        png_mem_ptrs[i] = p;
        png_mem_sizes[i] = size;
        png_mem_total += size;
        png_mem_cnt++;
        if (png_mem_max < png_mem_total)
            png_mem_max = png_mem_total;
    }
    return p;
}

void _pngfree(png_structp pp, png_voidp mem) {
    //	free(mem);
    int i;

    for (i = 0; i < 10; i++)
        if (mem == png_mem_ptrs[i]) {
            uint32_t size = png_mem_sizes[i];
            png_mem_ptrs[i] = 0;
            png_mem_sizes[i] = 0;
            png_mem_total -= size;
            png_mem_cnt--;
        }
    //	vPortFree(mem);
}

void st7789v_draw_png_ex(point_ui16_t pt, FILE *pf, color_t clr0, uint8_t rop) {
    static const png_byte unused_chunks[] = {
        98, 75, 71, 68, '\0',   /* bKGD */
        99, 72, 82, 77, '\0',   /* cHRM */
        104, 73, 83, 84, '\0',  /* hIST */
        105, 67, 67, 80, '\0',  /* iCCP */
        105, 84, 88, 116, '\0', /* iTXt */
        111, 70, 70, 115, '\0', /* oFFs */
        112, 67, 65, 76, '\0',  /* pCAL */
        115, 67, 65, 76, '\0',  /* sCAL */
        112, 72, 89, 115, '\0', /* pHYs */
        115, 66, 73, 84, '\0',  /* sBIT */
        115, 80, 76, 84, '\0',  /* sPLT */
        116, 69, 88, 116, '\0', /* tEXt */
        116, 73, 77, 69, '\0',  /* tIME */
        122, 84, 88, 116, '\0'  /* zTXt */
    };
    //	rewind(pf);
    //png_structp pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_structp pp = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, _pngmalloc, _pngfree);
    // Ignore unused chunks: see https://libpng.sourceforge.io/decompression_bombs.html
    png_set_keep_unknown_chunks(pp, 1, unused_chunks, (int)sizeof(unused_chunks) / 5);
    //png_set_mem_fn(pp, 0, _pngmalloc, _pngfree);
    if (pp == NULL)
        goto _e_0;
    png_infop ppi = png_create_info_struct(pp);
    if (ppi == NULL)
        goto _e_1;
    if (setjmp(png_jmpbuf(pp)))
        goto _e_1;
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
    //_dbg("st7789v_draw_png rowsize = %i", rowsize);
    if (rowsize > ST7789V_COLS * 4)
        goto _e_1;
    int pixsize = rowsize / w;
    //_dbg("st7789v_draw_png pixsize = %i", pixsize);
    int i;
    int j;
    st7789v_clr_cs();
    if (setjmp(png_jmpbuf(pp)))
        goto _e_2;
    st7789v_cmd_caset(pt.x, pt.x + w - 1);
    st7789v_cmd_raset(pt.y, pt.y + h - 1);
    st7789v_cmd_ramwr(0, 0);
    switch (rop) {
        //case ROPFN_INVERT: rop_rgb888_invert((uint8_t*)&clr0); break;
        //case ROPFN_SWAPBW: rop_rgb888_swapbw((uint8_t*)&clr0); break;
    }
    for (i = 0; i < h; i++) {
        png_read_row(pp, st7789v_buff, NULL);
        if (pixsize == 3) //RGB
            for (j = 0; j < w; j++) {
                uint16_t *ppx565 = (uint16_t *)(st7789v_buff + j * 2);
                uint8_t *ppx888 = (uint8_t *)(st7789v_buff + j * pixsize);
                switch (rop) {
                case ROPFN_INVERT:
                    rop_rgb888_invert(ppx888);
                    break;
                case ROPFN_SWAPBW:
                    rop_rgb888_swapbw(ppx888);
                    break;
                case ROPFN_DISABLE:
                    rop_rgb888_disabled(ppx888);
                    break;
                }
                *ppx565 = COLOR_TO_565(color_rgb(ppx888[0], ppx888[1], ppx888[2]));
            }
        else if (pixsize == 4) //RGBA
        {
            for (j = 0; j < w; j++) {
                uint16_t *ppx565 = (uint16_t *)(st7789v_buff + j * 2);
                uint8_t *ppx888 = (uint8_t *)(st7789v_buff + j * pixsize);
                *((color_t *)ppx888) = color_alpha(clr0, color_rgb(ppx888[0], ppx888[1], ppx888[2]), ppx888[3]);
                switch (rop) {
                case ROPFN_INVERT:
                    rop_rgb888_invert(ppx888);
                    break;
                case ROPFN_SWAPBW:
                    rop_rgb888_swapbw(ppx888);
                    break;
                case ROPFN_DISABLE:
                    rop_rgb888_disabled(ppx888);
                    break;
                }
                //*ppx565 = COLOR_TO_565(color_alpha(clr0, color_rgb(ppx888[0], ppx888[1], ppx888[2]), ppx888[3]));
                *ppx565 = COLOR_TO_565(color_rgb(ppx888[0], ppx888[1], ppx888[2]));
            }
        }
        st7789v_wr(st7789v_buff, 2 * w);
    }
_e_2:
    st7789v_set_cs();
_e_1:
    png_destroy_read_struct(&pp, &ppi, 0);
_e_0:
    return;
}

void st7789v_draw_png(point_ui16_t pt, FILE *pf) {
    st7789v_draw_png_ex(pt, pf, 0, 0);
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
    //faster code if CMD_INVON == CMD_INVOFF + 1
    //The result of the logical negation operator ! is 1 if the value of its operand is 0,
    //0 if the value of its operand is non-zero.
    st7789v_config.is_inverted = !st7789v_inversion_get();
    st7789v_cmd(CMD_INVOFF + st7789v_config.is_inverted, 0, 0);
    #else
    //to be portable
    if (st7789v_inversion_get())
        st7789v_inversion_off();
    else
        st7789v_inversion_on();

    #endif
}
uint8_t st7789v_inversion_get(void) {
    return st7789v_config.is_inverted;
}

//0x01 -> 0x02 -> 0x04 -> 0x08 -> 0x01
void st7789v_gamma_next(void) {
    st7789v_gamma_set_direct(((st7789v_config.gamma << 1) | (st7789v_config.gamma >> 3)) & 0x0f);
}

//0x01 -> 0x08 -> 0x04 -> 0x02 -> 0x01
void st7789v_gamma_prev(void) {
    st7789v_gamma_set_direct(((st7789v_config.gamma << 3) | (st7789v_config.gamma >> 1)) & 0x0f);
}

//use GAMMA_CURVE0 - GAMMA_CURVE3
void st7789v_gamma_set_direct(uint8_t gamma_enu) {
    st7789v_config.gamma = gamma_enu;
    st7789v_cmd(CMD_GAMMA_SET, &st7789v_config.gamma, sizeof(st7789v_config.gamma));
}

//use 0 - 3
void st7789v_gamma_set(uint8_t gamma) {
    if (gamma != st7789v_gamma_get())
        st7789v_gamma_set_direct(1 << (gamma & 0x03));
}

//returns 0 - 3
uint8_t st7789v_gamma_get() {
    uint8_t position = 0;
    for (int8_t position = 3; position >= 0; --position) {
        if (st7789v_config.gamma == 1 << position)
            break;
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
    //set brightness
    st7789v_cmd(CMD_WRDISBV, &st7789v_config.brightness, sizeof(st7789v_config.brightness));
}

uint8_t st7789v_brightness_get(void) {
    return st7789v_config.brightness;
}

void st7789v_ctrl_set(uint8_t ctrl) {
    st7789v_config.control = ctrl;
    st7789v_cmd(CMD_WRCTRLD, &st7789v_config.control, sizeof(st7789v_config.control));
}

#else //ST7789V_PNG_SUPPORT

void st7789v_draw_png(point_ui16_t pt, FILE *pf) {}

void st7789v_draw_png_ex(point_ui16_t pt, FILE *pf, color_t clr0, uint8_t rop) {}

#endif //ST7789V_PNG_SUPPORT

st7789v_config_t st7789v_config = {
    0,            // spi handle pointer
    0,            // CS pin
    0,            // RS pin
    0,            // RST pin
    0,            // flags (DMA, MISO)
    0,            // interface pixel format (5-6-5, hi-color)
    0,            // memory data access control (no mirror XY)
    GAMMA_CURVE0, // gamma curve
    0,            // brightness
    0,            // inverted
    0,            // default control reg value
};

//measured delay from low to hi in reset cycle
uint16_t st7789v_reset_delay = 0;

//! @brief enable safe mode (direct acces + safe delay)
void st7789v_enable_safe_mode(void) {
    st7789v_flg |= ST7789V_FLG_SAFE;
}

void st7789v_spi_tx_complete(void) {
#ifdef ST7789V_USE_RTOS
    osSignalSet(st7789v_task_handle, ST7789V_SIG_SPI_TX);
#endif //ST7789V_USE_RTOS
}
