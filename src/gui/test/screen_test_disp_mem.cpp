/*
 * screen_test_disp_mem.c
 *
 *  Created on: 2019-09-24
 *      Author: Radek Vana
 */

#include "gui.h"
#include "config.h"
#include "stm32f4xx_hal.h"

#include "st7789v.h"
#include "sys.h"
#include <assert.h>

extern int sim_heater_temp2val(float temp);

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;
    window_text_t textSpiClk;
    window_text_t textMode;
    window_text_t textSpiUserPattern1;
    window_text_t text0x;
    window_text_t textR0x;
    window_text_t textG0x;
    window_text_t textB0x;
    window_text_t textGamma;
    window_text_t textBrightness;

    window_numb_t numbSpiClk;
    window_numb_t numbSpiPattern;

    window_spin_t spinGamma;
    window_spin_t spinBrightness;

    window_list_t spinSpiClk;
    window_list_t spinMode;
    window_list_t spinInversion;
    window_list_t spinBrigt_ena;
    window_spin_t spinStrHx0;
    window_spin_t spinStrHx1;
    window_spin_t spinStrHx2;
    window_spin_t spinStrHx3;

    window_spin_t spinStrR0;
    window_spin_t spinStrR1;
    window_spin_t spinStrG0;
    window_spin_t spinStrG1;
    window_spin_t spinStrB0;
    window_spin_t spinStrB1;

    window_text_t textExit;
} screen_test_disp_mem_data_t;

#define pd ((screen_test_disp_mem_data_t *)screen->pdata)
/******************************************************************************************************/
//variables
extern int16_t spi_prescaler;
static const char *opt_spi[] = { "21M", "10.5M", "5.25M", "2.63M", "1.31M", "656k", "328k", "164k" };
#define opt_spi_sz (sizeof(opt_spi) / sizeof(const char *))

//static const char* modes[] = {"RGBW scale", "Direct hex", "RGB"};
static const char *modes[] = { "RGBW scale", "WrRdWr + RGB" };
#define modes_sz (sizeof(modes) / sizeof(const char *))

static const char *inversions[] = { "Inv. DIS.", "Inv. ENA." };
#define inversions_sz (sizeof(inversions) / sizeof(const char *))

static const char *bright_enas[] = { "Bri. DIS.", "Bri. ENA." };
#define bright_enas_sz (sizeof(bright_enas) / sizeof(const char *))

static int16_t spinSpiClkVal_last = -1;
static int16_t spinSpiClkVal_actual = -1;
static int16_t mode = 0;
static int16_t user_value = 0;
static int16_t row2draw;

static uint8_t clrR = 0;
static uint8_t clrG = 0;
static uint8_t clrB = 0;

static int16_t gamma_last = -1;
static int16_t gamma_actual = -1;
static int16_t brightness_last = -1;
static int16_t brightness_actual = -1;
static int8_t isInverted_last = -1;
static int8_t isInverted_actual = -1;
static int8_t isBrightness_ena_last = -1;
static int8_t isBrightness_ena_actual = -1;

void window_list_spi_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index < opt_spi_sz)
        *pptext = (char *)opt_spi[index];
    else
        *pptext = "Index ERROR";
    *pid_icon = 0;
}

void window_list_modes_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index < modes_sz)
        *pptext = (char *)modes[index];
    else
        *pptext = "Index ERROR";
    *pid_icon = 0;
}

void window_list_inversions_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index < inversions_sz)
        *pptext = (char *)inversions[index];
    else
        *pptext = "Index ERROR";
    *pid_icon = 0;
}

void window_list_bright_enas_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index < bright_enas_sz)
        *pptext = (char *)bright_enas[index];
    else
        *pptext = "Index ERROR";
    *pid_icon = 0;
}

/******************************************************************************************************/
//methods
//draw line in Y direction
void drawCol(size_t col, size_t row, size_t len, uint16_t directColor);
//draw line in Y direction from buffer
void drawCol_buff(size_t col, size_t row, size_t len, uint16_t *directColorBuff);
//read line in Y direction
void readCol(size_t col, size_t row, size_t len, uint16_t *directColorBuff);
//draw line in Y direction, read it and draw it under first line
void draw_read_drawCol(size_t col, size_t row, size_t row_space, size_t len, uint16_t directColor, uint16_t *directColorBuff);
void draw_read_drawRect(size_t col, size_t row, size_t len, size_t w, uint16_t directColor, uint16_t *directColorBuff);
size_t dispRamTest_NextCol(size_t row_pos, size_t rect_w, size_t rect_space, size_t rect_count, size_t border_w);
int _getRectIndex(size_t col_x, size_t rect_w, size_t rect_space, size_t border_w);
void dispRamTest(size_t test_ID, size_t row);

/******************************************************************************************************/
//draw types
void printRGBWscale(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space);
void printDirectHex(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space); //write read write
void printRGB(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space);
void printRGB_DirHx(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space);

typedef void(drawCol_t)(size_t rect_index, size_t rect_count, size_t col, size_t row2draw, size_t row_space);
//drawCol_t* fptrArr[] = {printRGBWscale,printDirectHex,printRGB};
drawCol_t *fptrArr[] = { printRGBWscale, printRGB_DirHx };
#define fptrArr_sz (sizeof(fptrArr) / sizeof(drawCol_t *))

static_assert(modes_sz == fptrArr_sz, "wrong number of function pointers");

/******************************************************************************************************/
//column specifications
enum { col_0 = 2,
    col_1 = 96 };
enum { col_0_w = col_1 - col_0,
    col_1_w = 240 - col_1 - col_0 };
enum { col_2_w = 38 };
#define RECT_MACRO(col) rect_ui16(col_##col, row2draw, col_##col##_w, row_h)

enum {
    TAG_QUIT = 10,
    TAG_BRIGHTNESS

};

void screen_test_disp_mem_init(screen_t *screen) {
    row2draw = 0;
    int16_t id;
    int16_t row_h = 22; //item_height from list

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 0, display::GetW(), 22), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"Disp. TEST rd mem.");

    row2draw += 25;

    //write pattern
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textMode));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"MODE");

    id = window_create_ptr(WINDOW_CLS_LIST, id0, RECT_MACRO(1), &(pd->spinMode));
    window_set_item_count(id, modes_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_modes_item);

    row2draw += 25;

    //clk setting
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textSpiClk));
    pd->textSpiClk.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"SPI clk");

    id = window_create_ptr(WINDOW_CLS_LIST, id0, RECT_MACRO(1), &(pd->spinSpiClk));
    window_set_item_count(id, opt_spi_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_spi_item);

    row2draw += 25;

    //Gamma setting
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textGamma));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"Gamma");

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col_1, row2draw, col_2_w, row_h), &(pd->spinGamma));
    window_set_format(id, "%1.0f");
    window_set_min_max_step(id, 0.0F, 3.0F, 1.0F);
    window_set_value(id, (float)st7789v_gamma_get());

    //INVERSION
    id = window_create_ptr(WINDOW_CLS_LIST, id0, rect_ui16(col_1 + col_2_w, row2draw, col_1_w - col_2_w, row_h), &(pd->spinInversion));
    window_set_item_count(id, inversions_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_inversions_item);

    row2draw += 25;
    //Brightness setting
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textBrightness));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"Brightn.");

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col_1, row2draw, col_2_w, row_h), &(pd->spinBrightness));
    window_set_format(id, "%1.0f");
    window_set_min_max_step(id, 0.0F, 255.0F, 5.0F);
    window_set_value(id, (float)st7789v_brightness_get());
    window_set_tag(id, TAG_BRIGHTNESS);

    //Brightness enabled
    id = window_create_ptr(WINDOW_CLS_LIST, id0, rect_ui16(col_1 + col_2_w, row2draw, col_1_w - col_2_w, row_h), &(pd->spinBrigt_ena));
    window_set_item_count(id, bright_enas_sz);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window_list_bright_enas_item);
    window_set_tag(id, TAG_BRIGHTNESS);

    row2draw += 25;
    int16_t w_of_0x = 23;
    int16_t col = col_1 + w_of_0x;
    int16_t offset = 12;
    //user write pattern
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textSpiUserPattern1));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"Wr-Rd-Wr");
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(1), &(pd->text0x));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"0x");

    //cannot use normal spin with window_set_format(id, "%A");
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrHx3));
    pd->spinStrHx3.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrHx2));
    pd->spinStrHx2.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrHx1));
    pd->spinStrHx1.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrHx0));
    pd->spinStrHx0.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);

    //write pixels
    row2draw += 25;
    int16_t w_of_0xX = 48;
    int16_t RGBspaceW = 5;

    col = col_0;
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col, row2draw, w_of_0xX, row_h), &(pd->textR0x));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"R 0x");
    window_set_color_text(id, COLOR_RED);

    col += w_of_0xX;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrR0));
    pd->spinStrR0.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);

    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrR1));
    pd->spinStrR1.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    col += RGBspaceW;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col, row2draw, w_of_0xX, row_h), &(pd->textG0x));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"G 0x");
    window_set_color_text(id, COLOR_GREEN);

    col += w_of_0xX;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrG0));
    pd->spinStrG0.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrG1));
    pd->spinStrG1.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    col += RGBspaceW;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col, row2draw, w_of_0xX, row_h), &(pd->textB0x));
    pd->textMode.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, (const char *)"B 0x");
    window_set_color_text(id, COLOR_BLUE);

    col += w_of_0xX;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrB0));
    pd->spinStrB0.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);
    col += offset;
    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(col, row2draw, offset, row_h), &(pd->spinStrB1));
    pd->spinStrB1.window.win.flg |= WINDOW_FLG_NUMB_FLOAT2INT;
    window_set_format(id, "%X");
    window_set_min_max_step(id, 0.0F, 15.0F, 1.0F);
    window_set_value(id, 0.0F);

    row2draw += 25; //position for drawing - it is global in this file

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 290, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);
}

//draw line in Y direction
void drawCol(size_t col, size_t row, size_t len, uint16_t directColor) {
    for (size_t i = 0; i < len; ++i) {
        st7789v_set_pixel_directColor(point_ui16(col, row + i), directColor);
    }
}
//draw line in Y direction from buffer
void drawCol_buff(size_t col, size_t row, size_t len, uint16_t *directColorBuff) {
    for (size_t i = 0; i < len; ++i) {
        st7789v_set_pixel_directColor(point_ui16(col, row + i), directColorBuff[i]);
    }
}

//read line in Y direction
void readCol(size_t col, size_t row, size_t len, uint16_t *directColorBuff) {
    for (size_t i = 0; i < len; ++i) {
        directColorBuff[i] = st7789v_get_pixel_directColor(point_ui16(col, row + i));
    }
}
//draw line in Y direction, read it and draw it under first line
void draw_read_drawCol(size_t col, size_t row, size_t row_space, size_t len, uint16_t directColor, uint16_t *directColorBuff) {
    drawCol(col, row, len, directColor);
    readCol(col, row, len, directColorBuff);
    drawCol_buff(col, row + len + row_space, len, directColorBuff);
}
/*
void draw_read_drawRect(size_t col, size_t row, size_t len, size_t w, uint16_t directColor, uint16_t *directColorBuff){
	for (;w>0;--w)
		draw_read_drawCol(col, row, len, directColor, directColorBuff);
}
*/

//entire disp takes about 500ms .. 320 * 240 = 76 800 dots
//time is 50 ms                     .. 7 680 dots
//21Mhz                             .. 7 680 dots
//164k                              ..    60 dots
//164k write - read - write .. 1/12 ..    20 dots
/*
size_t getNumOfWritesIn_1_Cycle(size_t wr_len){
	if (spinSpiClkVal_actual < 0) return 0;
	if (wr_len == 0) return 0;
	return 7680 / (wr_len << spinSpiClkVal_actual);
}
*/

#define directColorBuff_sz 20
uint16_t directColorBuff[directColorBuff_sz];

void printRGBWscale(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space) {
    color_t clr;
    const uint32_t val = 0xff * rect_index / rect_count;

    //R
    clr = color_rgb(val, 0, 0);
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);

    //G
    clr = color_rgb(0, val, 0);
    row += directColorBuff_sz + row_space;
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);

    //B
    clr = color_rgb(0, 0, val);
    row += directColorBuff_sz + row_space;
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);

    //W
    clr = color_rgb(val, val, val);
    row += directColorBuff_sz + row_space;
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);
}

void printDirectHex(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space) {
    draw_read_drawCol(col, row, row_space, directColorBuff_sz, user_value, directColorBuff);
}

void printRGB(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space) {
    color_t clr = color_rgb(clrR, clrG, clrB);
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);

    clr = color_rgb((uint8_t)(~clrR), (uint8_t)(~clrG), (uint8_t)(~clrB));
    row += directColorBuff_sz + row_space;
    display::DrawLine(point_ui16(col, row), point_ui16(col, row + directColorBuff_sz - 1), clr);
}

void printRGB_DirHx(size_t rect_index, size_t rect_count, size_t col, size_t row, size_t row_space) {
    printDirectHex(rect_index, rect_count, col, row, row_space);
    printRGB(rect_index, rect_count, col, row + 2 * (directColorBuff_sz + row_space), row_space);
}

void dispRamTest(size_t test_ID, size_t row) {
    //const size_t wr_rows       = 20;

    const size_t rect_w = 12;
    const size_t rect_space = 1;
    const size_t rect_count = 16;
    const size_t border_w = (display::GetW() - (rect_w + rect_space) * rect_count) / 2;
    const size_t row_space = 1;
    const uint32_t ttl = 45; //time to live

    static size_t col_pos = -1;
    static uint32_t start_time = 0;

    start_time = HAL_GetTick();

    //ttl check - to be responsive
    while (start_time + ttl >= HAL_GetTick()) {
        col_pos = dispRamTest_NextCol(col_pos, rect_w, rect_space, rect_count, border_w);
        size_t rect_index = _getRectIndex(col_pos, rect_w, rect_space, border_w);

        if (test_ID < sizeof(fptrArr) / sizeof(drawCol_t *))
            fptrArr[test_ID](rect_index, rect_count, col_pos, row, row_space);
    }
}

size_t dispRamTest_NextCol(size_t col_pos, size_t rect_w, size_t rect_space, size_t rect_count, size_t border_w) {
    const size_t col_pos_end = border_w + rect_count * rect_w + (rect_count - 1) * rect_space;

    ++col_pos;
    if (col_pos >= col_pos_end)
        return border_w;
    if (col_pos < border_w)
        return border_w;

    if (_getRectIndex(col_pos, rect_w, rect_space, border_w) == -1)
        col_pos += rect_space;
    return col_pos;
}

int _getRectIndex(size_t col_x, size_t rect_w, size_t rect_space, size_t border_w) {
    col_x -= border_w;
    size_t mod = col_x % (rect_w + rect_space);
    size_t div = col_x / (rect_w + rect_space);
    if (mod >= rect_w)
        return -1;
    return div;
}

/*
void drawPartLine(size_t partRow, size_t partDivider, color_t clr){
	size_t partLen = display::GetW() / partDivider;
	size_t row     = partRow / partDivider;
	size_t col     = (partRow % partDivider) * partLen;

	//paint part of line
	for (size_t i = 0; i < partLen; ++i){
		display::SetPixel(point_ui16(col + i, row), clr);
	}

}

void readPartLine(size_t partRow, size_t partDivider, color_t *buff){
	size_t partLen = display::GetW() / partDivider;
	size_t row     = partRow / partDivider;
	size_t col     = (partRow % partDivider) * partLen;

	//read part of line
	for (size_t i = 0; i < quarter; ++i){
		buff[i] = st7789v_get_pixel(point_ui16(col + i, row2draw));
	}

}
*/

void screen_test_disp_mem_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_test_disp_mem_draw(screen_t *screen) {
}

int screen_test_disp_mem_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            screen_close();
            return 1;
        }
    /*if (event == WINDOW_EVENT_CHANGE){
		switch ((int)param){
		case TAG_BRIGHTNESS:
			isBrightness_ena_actual = window_get_item_index(pd->spinBrigt_ena.window.win.id);
			//if(isBrightness_ena_actual!=isBrightness_ena_last)
			{
				if(isBrightness_ena_actual) st7789v_brightness_enable();
				else                        st7789v_brightness_disable();
				isBrightness_ena_last = isBrightness_ena_actual;
			}


			brightness_actual = window_get_item_index(pd->spinBrightness.window.win.id);
			//if(brightness_actual!=brightness_last)
			{
				st7789v_brightness_set(brightness_actual);
				brightness_last = brightness_actual;
			}
			break;
		}

	}*/
    if (event == WINDOW_EVENT_LOOP) {

        isBrightness_ena_actual = window_get_item_index(pd->spinBrigt_ena.win.id);
        brightness_actual = window_get_value(pd->spinBrightness.window.win.id);

        if ((isBrightness_ena_actual != isBrightness_ena_last) || (brightness_actual != brightness_last)) {
            st7789v_brightness_set(brightness_actual);
            brightness_last = brightness_actual;

            if (isBrightness_ena_actual)
                st7789v_brightness_enable();
            else
                st7789v_brightness_disable();
            isBrightness_ena_last = isBrightness_ena_actual;
        }

        mode = window_get_item_index(pd->spinMode.win.id);

        user_value = window_get_item_index(pd->spinStrHx0.window.win.id)
            | ((window_get_item_index(pd->spinStrHx1.window.win.id)) << 4)
            | ((window_get_item_index(pd->spinStrHx2.window.win.id)) << 8)
            | ((window_get_item_index(pd->spinStrHx3.window.win.id)) << 12);

        clrR = (window_get_item_index(pd->spinStrR0.window.win.id) << 4) | window_get_item_index(pd->spinStrR1.window.win.id);
        clrG = (window_get_item_index(pd->spinStrG0.window.win.id) << 4) | window_get_item_index(pd->spinStrG1.window.win.id);
        clrB = (window_get_item_index(pd->spinStrB0.window.win.id) << 4) | window_get_item_index(pd->spinStrB1.window.win.id);

        gamma_actual = window_get_item_index(pd->spinGamma.window.win.id);
        if (gamma_actual != gamma_last) {
            st7789v_gamma_set(gamma_actual);
            gamma_last = gamma_actual;
        }

        isInverted_actual = window_get_item_index(pd->spinInversion.win.id);
        if (isInverted_actual != isInverted_last) {
            if (isInverted_actual)
                st7789v_inversion_on();
            else
                st7789v_inversion_off();
            isInverted_last = isInverted_actual;
        }

        //check if spin changed
        spinSpiClkVal_actual = window_get_item_index(pd->spinSpiClk.win.id);
        if (spinSpiClkVal_actual != spinSpiClkVal_last) {
            sys_spi_set_prescaler(spinSpiClkVal_actual);
            spinSpiClkVal_last = spinSpiClkVal_actual;
            //window_set_text(pd->numbSpiClk.win.id,(const char*)opt_spi[spinSpiClkVal_actual]);
        }

        dispRamTest(mode, row2draw);
    }

    return 0;
}

screen_t screen_test_disp_mem = {
    0,
    0,
    screen_test_disp_mem_init,
    screen_test_disp_mem_done,
    screen_test_disp_mem_draw,
    screen_test_disp_mem_event,
    sizeof(screen_test_disp_mem_data_t), //data_size
    0,                                   //pdata
};

screen_t *const get_scr_test_disp_mem() { return &screen_test_disp_mem; }
