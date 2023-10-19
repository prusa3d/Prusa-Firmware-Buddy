/*
 * screen_test_disp_mem.cpp
 *
 *  Created on: 2019-09-24
 *      Author: Radek Vana
 */
#if 0
    #include "gui.hpp"
    #include "config.h"
    #include "stm32f4xx_hal.h"

    #include "sys.h"
    #include <assert.h>
    #include "st7789v.hpp"

extern int sim_heater_temp2val(float temp);

struct screen_test_disp_mem_data_t : public AddSuperWindow<screen_t> {
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
};

    #define pd              ((screen_test_disp_mem_data_t *)screen->pdata)
/******************************************************************************************************/
//variables
extern int16_t spi_prescaler;
static const char *opt_spi[] = { "21M", "10.5M", "5.25M", "2.63M", "1.31M", "656k", "328k", "164k" };
//static const char* modes[] = {"RGBW scale", "Direct hex", "RGB"};
static const char *modes[] = { "RGBW scale", "WrRdWr + RGB" };
static const char *inversions[] = { "Inv. DIS.", "Inv. ENA." };
static const char *bright_enas[] = { "Bri. DIS.", "Bri. ENA." };

enum {
    opt_spi_sz = sizeof(opt_spi) / sizeof(const char *),
    modes_sz  = sizeof(modes) / sizeof(const char *),
    inversions_sz = sizeof(inversions) / sizeof(const char *),
    bright_enas_sz = sizeof(bright_enas) / sizeof(const char *),
};

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
enum{
    fptrArr_sz = sizeof(fptrArr) / sizeof(drawCol_t *)
};

static_assert(modes_sz == fptrArr_sz, "wrong number of function pointers");

/******************************************************************************************************/
//column specifications
enum { col_0 = 2,
    col_1 = 96 };
enum { col_0_w = col_1 - col_0,
    col_1_w = 240 - col_1 - col_0 };
enum { col_2_w = 38 };
    #define RECT_MACRO(col) Rect16(col_##col, row2draw, col_##col##_w, row_h)

enum {
    TAG_QUIT = 10,
    TAG_BRIGHTNESS

};

//cannot use normal spin with format "%A"
static void hexSpinInit(int16_t id0, Rect16 rect, window_spin_t *pSpin) {
    window_create_ptr(WINDOW_CLS_SPIN, id0, rect, pSpin);
    pSpin->PrintAsInt32();
    pSpin->SetFormat("%X");
    pSpin->SetMinMaxStep(0.0F, 15.0F, 1.0F);
    pSpin->SetValue(0.0F);
}

void screen_test_disp_mem_init(screen_t *screen) {
    row2draw = 0;
    int16_t row_h = 22; //item_height from list

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, Rect16(0, 0, 0, 0), pd);

    window_create_ptr(WINDOW_CLS_TEXT, id0, Rect16(0, 0, display::GetW(), 22), &(pd->textMenuName));
    pd->textMenuName.set_font(resource_font(IDR_FNT_BIG));
    static const char dtrm[] = "Disp. TEST rd mem.";
    pd->textMenuName.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)dtrm));

    row2draw += 25;

    //write pattern
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textMode));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char mod[] = "MODE";
    pd->textMode.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)mod));

    window_create_ptr(WINDOW_CLS_LIST, id0, RECT_MACRO(1), &(pd->spinMode));
    pd->spinMode.SetItemCount(modes_sz);
    pd->spinMode.SetItemIndex(0);
    pd->spinMode.SetCallback(window_list_modes_item);

    row2draw += 25;

    //clk setting
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textSpiClk));
    pd->textSpiClk.set_font(resource_font(IDR_FNT_NORMAL));
    static const char spi[] = "SPI clk";
    pd->textSpiClk.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)spi));

    window_create_ptr(WINDOW_CLS_LIST, id0, RECT_MACRO(1), &(pd->spinSpiClk));
    pd->spinSpiClk.SetItemCount(opt_spi_sz);
    pd->spinSpiClk.SetItemIndex(0);
    pd->spinSpiClk.SetCallback(window_list_spi_item);

    row2draw += 25;

    //Gamma setting
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textGamma));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char gam[] = "Gamma";
    pd->textMode.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)gam));

    window_create_ptr(WINDOW_CLS_SPIN, id0, Rect16(col_1, row2draw, col_2_w, row_h), &(pd->spinGamma));
    pd->spinGamma.SetFormat("%1.0f");
    pd->spinGamma.SetMinMaxStep(0.0F, 3.0F, 1.0F);

    #ifdef USE_ST7789
    pd->spinGamma.SetValue((float)st7789v_gamma_get());
    #endif

    #ifdef USE_ILI9488
    pd->spinGamma.SetValue((float)ili9488_gamma_get());
    #endif

    //INVERSION
    window_create_ptr(WINDOW_CLS_LIST, id0, Rect16(col_1 + col_2_w, row2draw, col_1_w - col_2_w, row_h), &(pd->spinInversion));
    pd->spinInversion.SetItemCount(inversions_sz);
    pd->spinInversion.SetItemIndex(0);
    pd->spinInversion.SetCallback(window_list_inversions_item);

    row2draw += 25;
    //Brightness setting
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textBrightness));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char bri[] = "Brightn.";
    pd->textBrightness.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bri));

    window_create_ptr(WINDOW_CLS_SPIN, id0, Rect16(col_1, row2draw, col_2_w, row_h), &(pd->spinBrightness));
    pd->spinBrightness.SetFormat("%1.0f");
    pd->spinBrightness.SetMinMaxStep(0.0F, 255.0F, 5.0F);

    #ifdef USE_ST7789
    pd->spinBrightness.SetValue((float)st7789v_brightness_get());
    #endif

    #ifdef USE_ILI9488
    pd->spinBrightness.SetValue((float)ili9488_brightness_get());
    #endif

    pd->spinBrightness.SetTag(TAG_BRIGHTNESS);

    //Brightness enabled
    window_create_ptr(WINDOW_CLS_LIST, id0, Rect16(col_1 + col_2_w, row2draw, col_1_w - col_2_w, row_h), &(pd->spinBrigt_ena));
    pd->spinBrigt_ena.SetItemCount(bright_enas_sz);
    pd->spinBrigt_ena.SetItemIndex(0);
    pd->spinBrigt_ena.SetCallback(window_list_bright_enas_item);
    pd->spinBrigt_ena.SetTag(TAG_BRIGHTNESS);

    row2draw += 25;
    int16_t w_of_0x = 23;
    int16_t col = col_1 + w_of_0x;
    int16_t offset = 12;
    //user write pattern
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(0), &(pd->textSpiUserPattern1));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char wrw[] = "Wr-Rd-Wr";
    pd->textSpiUserPattern1.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wrw));
    window_create_ptr(WINDOW_CLS_TEXT, id0, RECT_MACRO(1), &(pd->text0x));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char zx[] = "0x";
    pd->text0x.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)zx));

    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrHx3));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrHx2));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrHx1));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrHx0));

    //write pixels
    row2draw += 25;
    int16_t w_of_0xX = 48;
    int16_t RGBspaceW = 5;

    col = col_0;
    window_create_ptr(WINDOW_CLS_TEXT, id0, Rect16(col, row2draw, w_of_0xX, row_h), &(pd->textR0x));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char rzx[] = "R 0x";
    pd->textR0x.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rzx));
    pd->textR0x.SetTextColor(COLOR_RED);

    col += w_of_0xX;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrR0));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrR1));
    col += offset;
    col += RGBspaceW;

    window_create_ptr(WINDOW_CLS_TEXT, id0, Rect16(col, row2draw, w_of_0xX, row_h), &(pd->textG0x));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char gzx[] = "G 0x";
    pd->textG0x.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)gzx));
    pd->textG0x.SetTextColor(COLOR_GREEN);

    col += w_of_0xX;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrG0));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrG1));
    col += offset;
    col += RGBspaceW;

    window_create_ptr(WINDOW_CLS_TEXT, id0, Rect16(col, row2draw, w_of_0xX, row_h), &(pd->textB0x));
    pd->textMode.set_font(resource_font(IDR_FNT_NORMAL));
    static const char bzx[] = "B 0x";
    pd->textB0x.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bzx));
    pd->textB0x.SetTextColor(COLOR_BLUE);

    col += w_of_0xX;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrB0));
    col += offset;
    hexSpinInit(id0, Rect16(col, row2draw, offset, row_h), &(pd->spinStrB1));

    row2draw += 25; //position for drawing - it is global in this file

    window_create_ptr(WINDOW_CLS_TEXT, id0, Rect16(col_0, 290, 60, 22), &(pd->textExit));
    pd->textExit.set_font(resource_font(IDR_FNT_BIG));
    static const char ex[] = "EXIT";
    pd->textExit.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ex));
    pd->textExit.Enable();
    pd->textExit.SetTag(TAG_QUIT);
}

//draw line in Y direction
void drawCol(size_t col, size_t row, size_t len, uint16_t directColor) {
    for (size_t i = 0; i < len; ++i) {
        display_ex_set_pixel_displayNativeColor(point_ui16(col, row + i), directColor);
    }
}
//draw line in Y direction from buffer
void drawCol_buff(size_t col, size_t row, size_t len, uint16_t *directColorBuff) {
    for (size_t i = 0; i < len; ++i) {
        display_ex_set_pixel_displayNativeColor(point_ui16(col, row + i), directColorBuff[i]);
    }
}

//read line in Y direction
void readCol(size_t col, size_t row, size_t len, uint16_t *directColorBuff) {
    for (size_t i = 0; i < len; ++i) {
        directColorBuff[i] = display_ex_get_pixel_displayNativeColor(point_ui16(col, row + i));
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

enum {
    directColorBuff_sz = 20
};
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

    start_time = gui::GetTick;

    //ttl check - to be responsive
    while (start_time + ttl >= gui::GetTick) {
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
		buff[i] = display_ex_get_pixel(point_ui16(col + i, row2draw));
	}

}
*/

void screen_test_disp_mem_done(screen_t *screen) {
}

void screen_test_disp_mem_draw(screen_t *screen) {
}

int screen_test_disp_mem_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == GUI_event_t::CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            Screens::Access()->Close();
            return 1;
        }
    if (event == GUI_event_t::LOOP) {

        isBrightness_ena_actual = pd->spinBrigt_ena.GetItemIndex();
        brightness_actual = pd->spinBrightness.GetValue();

    #ifdef USE_ST7789
        if ((isBrightness_ena_actual != isBrightness_ena_last) || (brightness_actual != brightness_last)) {
            st7789v_brightness_set(brightness_actual);
            brightness_last = brightness_actual;

            if (isBrightness_ena_actual)
                st7789v_brightness_enable();
            else
                st7789v_brightness_disable();
            isBrightness_ena_last = isBrightness_ena_actual;
        }
    #endif

    #ifdef USE_ILI9488
        if ((isBrightness_ena_actual != isBrightness_ena_last) || (brightness_actual != brightness_last)) {
            ili9488_brightness_set(brightness_actual);
            brightness_last = brightness_actual;

            if (isBrightness_ena_actual)
                ili9488_brightness_enable();
            else
                ili9488_brightness_disable();
            isBrightness_ena_last = isBrightness_ena_actual;
        }
    #endif

        mode = pd->spinMode.GetItemIndex();

        user_value = pd->spinStrHx0.GetItemIndex()
            | ((pd->spinStrHx1.GetItemIndex()) << 4)
            | ((pd->spinStrHx2.GetItemIndex()) << 8)
            | ((pd->spinStrHx3.GetItemIndex()) << 12);

        clrR = (pd->spinStrR0.GetItemIndex() << 4) | pd->spinStrR1.GetItemIndex();
        clrG = (pd->spinStrG0.GetItemIndex() << 4) | pd->spinStrG1.GetItemIndex();
        clrB = (pd->spinStrB0.GetItemIndex() << 4) | pd->spinStrB1.GetItemIndex();

    #ifdef USE_ST7789
        gamma_actual = pd->spinGamma.GetItemIndex();
        if (gamma_actual != gamma_last) {
            st7789v_gamma_set(gamma_actual);
            gamma_last = gamma_actual;
        }

        isInverted_actual = pd->spinInversion.GetItemIndex();
        if (isInverted_actual != isInverted_last) {
            if (isInverted_actual)
                st7789v_inversion_on();
            else
                st7789v_inversion_off();
            isInverted_last = isInverted_actual;
        }
    #endif // USE_ST7789

    #ifdef USE_ILI9488
        gamma_actual = pd->spinGamma.GetItemIndex();
        if (gamma_actual != gamma_last) {
            ili9488_gamma_set(gamma_actual);
            gamma_last = gamma_actual;
        }

        isInverted_actual = pd->spinInversion.GetItemIndex();
        if (isInverted_actual != isInverted_last) {
            if (isInverted_actual)
                ili9488_inversion_on();
            else
                ili9488_inversion_off();
            isInverted_last = isInverted_actual;
        }
    #endif // USE_ILI9488

        //check if spin changed
        spinSpiClkVal_actual = pd->spinSpiClk.GetItemIndex();
        if (spinSpiClkVal_actual != spinSpiClkVal_last) {
            sys_spi_set_prescaler(spinSpiClkVal_actual);
            spinSpiClkVal_last = spinSpiClkVal_actual;
        }

        dispRamTest(mode, row2draw);
    }

    return 0;
}
#endif // #if 0
