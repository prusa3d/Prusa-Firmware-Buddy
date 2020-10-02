// st7789v.c
#include "st7789v.h"
#include "st7789v_impl.h"

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
enum {
    CMD_SLPIN = 0x10,
    CMD_SLPOUT = 0x11,
    CMD_INVOFF = 0x20,    //Display Inversion Off
    CMD_INVON = 0x21,     //Display Inversion On
    CMD_GAMMA_SET = 0x26, //gamma set
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
    CMD_WRDISBV = 0x51, //Write Display Brightness
    CMD_RDDISBV = 0x52, //Read Display Brightness Value
    CMD_WRCTRLD = 0x53, // Write CTRL Display
                        //-Brightness Control Block - bit 5
                        //-Display Dimming			- bit 3
                        //-Backlight Control On/Off - bit 2
    CMD_RDCTRLD = 0x54, //Read CTRL Value Display
};

//st7789 gamma
enum {
    GAMMA_CURVE0 = 0x01,
    GAMMA_CURVE1 = 0x02,
    GAMMA_CURVE2 = 0x04,
    GAMMA_CURVE3 = 0x08,
};

//st7789 CTRL Display
static const uint8_t MASK_CTRLD_BCTRL = 1 << 5; //Brightness Control Block
// static const uint8_t MASK_CTRLD_DD = 1 << 3;    //Display Dimming
// static const uint8_t MASK_CTRLD_BL = 1 << 2;    //Backlight Control

//color constants
enum {
    CLR565_WHITE = 0xffff,
    CLR565_BLACK = 0x0000,
    CLR565_RED = 0xf800,
    CLR565_CYAN = 0x0000,
    CLR565_MAGENTA = 0x0000,
    CLR565_GREEN = 0x07e0,
    CLR565_YELLOW = 0xffe0,
    CLR565_ORANGE = 0x0000,
    CLR565_GRAY = 0x38e7,
    CLR565_BLUE = 0x001f,
};

uint8_t st7789v_flg = 0; // flags

uint16_t st7789v_x = 0;  // current x coordinate (CASET)
uint16_t st7789v_y = 0;  // current y coordinate (RASET)
uint16_t st7789v_cx = 0; //
uint16_t st7789v_cy = 0; //

uint8_t st7789v_buff[ST7789V_COLS * 2 * ST7789V_BUFF_ROWS]; //16 lines buffer

#ifdef ST7789V_USE_RTOS
osThreadId st7789v_task_handle = 0;
#endif //ST7789V_USE_RTOS

/*some functions are in header - excluded from display_t struct*/
void st7789v_gamma_set_direct(uint8_t gamma_enu);
uint8_t st7789v_read_ctrl(void);
void st7789v_ctrl_set(uint8_t ctrl);

static inline void st7789v_fill_ui16(uint16_t *p, uint16_t v, uint16_t c) {
    while (c--)
        *(p++) = v;
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void st7789v_delay_ms(uint32_t ms) {
    if (is_interrupt() || (st7789v_flg & (uint8_t)ST7789V_FLG_SAFE)) {
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
    if ((st7789v_flg & (uint8_t)ST7789V_FLG_DMA) && !(st7789v_flg & (uint8_t)ST7789V_FLG_SAFE) && (size > 4)) {
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
#if 0
//#ifdef ST7789V_DMA
    if (size <= 4)
        HAL_SPI_Receive(st7789v_config.phspi, pb, size, HAL_MAX_DELAY);
    else
    {
    #ifdef ST7789V_USE_RTOS
        osSignalSet(0, ST7789V_SIG_SPI_TX);
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    #endif //ST7789V_USE_RTOS
        HAL_SPI_Receive_DMA(st7789v_config.phspi, pb, size);
    #ifdef ST7789V_USE_RTOS
        osSignalWait(ST7789V_SIG_SPI_TX, osWaitForever);
    #endif //ST7789V_USE_RTOS
    }
#else      //ST7789V_DMA
    HAL_SPI_Receive(st7789v_config.phspi, pb, size, HAL_MAX_DELAY);
#endif     //ST7789V_DMA
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
    st7789v_flg &= ~(FLG_CS | FLG_RS | FLG_RST);
    st7789v_set_rst();
    st7789v_set_cs();
    st7789v_set_rs();
}

void st7789v_init(void) {
#ifdef ST7789V_USE_RTOS
    st7789v_task_handle = osThreadGetId();
#endif //ST7789V_USE_RTOS
    if (st7789v_flg & (uint8_t)ST7789V_FLG_SAFE)
        st7789v_flg &= ~(uint8_t)ST7789V_FLG_DMA;
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
void st7789v_clear(uint16_t clr565) {
    // FIXME similar to display_ex_fill_rect; join?
    int i;
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
void st7789v_set_pixel(uint16_t point_x, uint16_t point_y, uint16_t clr565) {
    st7789v_cmd_caset(point_x, 1);
    st7789v_cmd_raset(point_y, 1);
    st7789v_cmd_ramwr((uint8_t *)(&clr565), 2);
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

uint16_t st7789v_get_pixel_colorFormat565(uint16_t point_x, uint16_t point_y) {
    enum { buff_sz = 5 };
    uint8_t buff[buff_sz];
    st7789v_cmd_caset(point_x, 1);
    st7789v_cmd_raset(point_y, 1);
    st7789v_cmd_ramrd(buff, buff_sz);
    uint16_t ret = rd18bit_to_16bit(buff + 2);
    return ret; //directColor;
}

uint8_t *st7789v_get_block(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    if (start_x > ST7789V_COLS || start_y > ST7789V_ROWS || end_x > ST7789V_COLS || end_y > ST7789V_ROWS)
        return NULL;
    st7789v_cmd_caset(start_x, end_x);
    st7789v_cmd_raset(start_y, end_y);
    st7789v_cmd_ramrd(st7789v_buff, ST7789V_COLS * 2 * ST7789V_BUFF_ROWS);
    return st7789v_buff;
}

/// Draws a solid rectangle of defined color
void st7789v_fill_rect_colorFormat565(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint16_t clr565) {

    uint32_t size = (uint32_t)rect_w * rect_h * 2; // area of rectangle

    st7789v_fill_ui16((uint16_t *)st7789v_buff, clr565, MIN(size, sizeof(st7789v_buff) / 2));
    st7789v_clr_cs();
    st7789v_cmd_caset(rect_x, rect_x + rect_w - 1);
    st7789v_cmd_raset(rect_y, rect_y + rect_h - 1);
    st7789v_cmd_ramwr(0, 0);

    for (unsigned int i = 0; i < size / sizeof(st7789v_buff); i++) // writer buffer by buffer
        st7789v_wr(st7789v_buff, sizeof(st7789v_buff));

    st7789v_wr(st7789v_buff, size % sizeof(st7789v_buff)); // write the remainder data
    st7789v_set_cs();
}

void st7789v_draw_char_from_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    st7789v_clr_cs();
    st7789v_cmd_caset(x, x + w - 1);
    st7789v_cmd_raset(y, y + h - 1);
    st7789v_cmd_ramwr(st7789v_buff, 2 * w * h);
    st7789v_set_cs();
}

#ifdef ST7789V_PNG_SUPPORT

    #include <png.h>
enum {
    PNG_MAX_CHUNKS = 10
};

void *png_mem_ptr0 = 0;
uint32_t png_mem_total = 0;
uint32_t png_mem_max = 0;
void *png_mem_ptrs[PNG_MAX_CHUNKS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_sizes[PNG_MAX_CHUNKS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t png_mem_cnt = 0;

png_voidp _pngmalloc(png_structp pp, png_alloc_size_t size) {
    //	return malloc(size);
    //	return pvPortMalloc(size);
    if (png_mem_ptr0 == 0)
        //png_mem_ptr0 = pvPortMalloc(0xc000); //48k
        png_mem_ptr0 = (void *)0x10000000; //ccram
    void *p = ((uint8_t *)png_mem_ptr0) + png_mem_total;
    //	if (p == 0)
    //		while (1);
    //	else
    {
        int i;
        for (i = 0; i < PNG_MAX_CHUNKS; i++)
            if (png_mem_ptrs[i] == 0)
                break;
        if (i >= PNG_MAX_CHUNKS)
            return NULL;
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

void st7789v_draw_png_ex(uint16_t point_x, uint16_t point_y, FILE *pf, uint32_t clr0, uint8_t rop) {
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
    //_dbg("display_ex_draw_png rowsize = %i", rowsize);
    if (rowsize > ST7789V_COLS * 4)
        goto _e_1;
    int pixsize = rowsize / w;
    //_dbg("display_ex_draw_png pixsize = %i", pixsize);
    int i;
    int j;
    st7789v_clr_cs();
    if (setjmp(png_jmpbuf(pp)))
        goto _e_2;
    st7789v_cmd_caset(point_x, point_x + w - 1);
    st7789v_cmd_raset(point_y, point_y + h - 1);
    st7789v_cmd_ramwr(0, 0);
    switch (rop) {
        //case ROPFN_INVERT: rop_rgb888_invert((uint8_t*)&clr0); break;
        //case ROPFN_SWAPBW: rop_rgb888_swapbw((uint8_t*)&clr0); break;
    }
    for (i = 0; i < h; i++) {
        png_read_row(pp, st7789v_buff, NULL);
        for (j = 0; j < w; j++) {
            uint16_t *ppx565 = (uint16_t *)(st7789v_buff + j * 2);
            uint8_t *ppx888 = (uint8_t *)(st7789v_buff + j * pixsize);
            if (pixsize == 4) { //RGBA
                *((uint32_t *)ppx888) = color_alpha(clr0, color_rgb(ppx888[0], ppx888[1], ppx888[2]), ppx888[3]);
            }
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
            *ppx565 = color_to_565(color_rgb(ppx888[0], ppx888[1], ppx888[2]));
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

void st7789v_draw_png_ex(uint16_t point_x, uint16_t point_y, FILE *pf, uint32_t clr0, uint8_t rop) {}

#endif //ST7789V_PNG_SUPPORT

st7789v_config_t st7789v_config = {
    0,            // spi handle pointer
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
    st7789v_flg |= (uint8_t)ST7789V_FLG_SAFE;
}

void st7789v_spi_tx_complete(void) {
#ifdef ST7789V_USE_RTOS
    osSignalSet(st7789v_task_handle, ST7789V_SIG_SPI_TX);
#endif //ST7789V_USE_RTOS
}
