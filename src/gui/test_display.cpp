//test_display.c - display performance testing functions

#include <inttypes.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "dbg.h"
#include "gui.h"
#include "resource.h"
#include <algorithm>

typedef void(test_display_t)(uint16_t cnt);

void test_display_random_dots(uint16_t cnt);
void test_display_random_lines(uint16_t cnt);
void test_display_random_rects(uint16_t cnt);
void test_display_random_filled_rects(uint16_t cnt);
void test_display_random_chars_small(uint16_t cnt);
void test_display_random_chars_normal(uint16_t cnt);
void test_display_random_chars_big(uint16_t cnt);
void test_display_random_chars_terminal(uint16_t cnt);
void test_display_fade(uint16_t cnt);
void test_display_rgbcolors(uint16_t cnt);
void test_display_spectrum(uint16_t cnt);

void do_test(test_display_t *func, int cnt, const char *name, const char *unit) {
    display::Clear(COLOR_BLACK);

#if (DBG_LEVEL >= 3)
    uint32_t tim = _microseconds();
#endif
    func(cnt);
#if (DBG_LEVEL >= 3)
    tim = _microseconds() - tim;
    float spd = (float)cnt * 1000000 / tim;
    _dbg3(" %-20.20s %5u %5.5ss/sec  %5uus/%-5.5s", name, (uint32_t)spd, unit, tim / cnt, unit);
#endif
    osDelay(1000);
}

void test_display(void) {
    while (1) {
        _dbg3("test_display start");
        do_test(test_display_random_dots, 20000, "random_dots", "dot");
        do_test(test_display_random_lines, 500, "random_lines", "line");
        do_test(test_display_random_rects, 500, "random_rects", "rect");
        do_test(test_display_random_filled_rects, 500, "random_filled_rects", "rect");
        do_test(test_display_random_chars_small, 3000, "random_chars_small", "char");
        do_test(test_display_random_chars_normal, 2000, "random_chars_normal", "char");
        do_test(test_display_random_chars_big, 1500, "random_chars_big", "char");
        do_test(test_display_random_chars_terminal, 1500, "random_chars_terminal", "char");
        do_test(test_display_fade, 32, "fade", "frame");
        do_test(test_display_rgbcolors, 20, "rgbcolors", "frame");
        do_test(test_display_spectrum, 20, "spectrum", "frame");
        _dbg3("test_display end\n");
    }
}

point_ui16_t random_point() {
    return point_ui16(rand() % display::GetW(), rand() % display::GetH());
}

color_t random_color() {
    return color_rgb(rand() % 0x100, rand() % 0x100, rand() % 0x100);
}

rect_ui16_t random_rect() {
    const uint16_t x0 = rand() % display::GetW();
    const uint16_t x1 = rand() % display::GetW();
    const uint16_t y0 = rand() % display::GetH();
    const uint16_t y1 = rand() % display::GetH();
    return rect_ui16(std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0) + 1, std::abs(y1 - y0) + 1);
}

void test_display_random_dots(uint16_t cnt) {
    for (uint16_t n = 0; n < cnt; ++n)
        display::SetPixel(random_point(), random_color());
}

void test_display_random_lines(uint16_t cnt) {
    for (uint16_t n = 0; n < cnt; ++n)
        display::DrawLine(random_point(), random_point(), random_color());
}

void test_display_random_rects(uint16_t cnt) {
    for (uint16_t n = 0; n < cnt; ++n)
        display::DrawRect(random_rect(), random_color());
}

void test_display_random_filled_rects(uint16_t cnt) {
    for (uint16_t n = 0; n < cnt; ++n)
        display::FillRect(random_rect(), random_color());
}

void test_display_random_chars(uint16_t cnt, font_t *font) {
    uint16_t x;
    uint16_t y;
    char c;

    for (uint16_t n = 0; n < cnt; ++n) {
        x = rand() % (display::GetW() - font->w + 1);
        y = rand() % (display::GetH() - font->h + 1);
        //		c = 'a' + ('z' - 'a' + 1) * ((float)rand() / RAND_MAX);
        c = font->asc_min + rand() % (font->asc_max - font->asc_min + 1 + 1);
        display::DrawChar(point_ui16(x, y), c, font, random_color(), random_color());
    }
}

void test_display_random_chars_small(uint16_t cnt) {
    font_t *font = resource_font(IDR_FNT_SMALL);
    test_display_random_chars(cnt, font);
}

void test_display_random_chars_normal(uint16_t cnt) {
    font_t *font = resource_font(IDR_FNT_NORMAL);
    test_display_random_chars(cnt, font);
}

void test_display_random_chars_big(uint16_t cnt) {
    font_t *font = resource_font(IDR_FNT_BIG);
    test_display_random_chars(cnt, font);
}

void test_display_random_chars_terminal(uint16_t cnt) {
    font_t *font = resource_font(IDR_FNT_TERMINAL);
    test_display_random_chars(cnt, font);
}

// original source: https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
// RGB <0,1> <- lambda l <400,700> [nm]
void spectral_color(float l, float *pr, float *pg, float *pb) {
    float t;
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;

    if ((l >= 400.0F) && (l < 410.0F)) {
        t = (l - 400.0F) / (410.0F - 400.0F);
        r = +(0.33F * t) - (0.20F * t * t);
    } else if ((l >= 410.0F) && (l < 475.0F)) {
        t = (l - 410.0F) / (475.0F - 410.0F);
        r = 0.14F - (0.13F * t * t);
    } else if ((l >= 545.0F) && (l < 595.0F)) {
        t = (l - 545.0F) / (595.0F - 545.0F);
        r = +(1.98F * t) - (t * t);
    } else if ((l >= 595.0F) && (l < 650.0F)) {
        t = (l - 595.0F) / (650.0F - 595.0F);
        r = 0.98F + (0.06F * t) - (0.40F * t * t);
    } else if ((l >= 650.0F) && (l < 700.0F)) {
        t = (l - 650.0F) / (700.0F - 650.0F);
        r = 0.65F - (0.84F * t) + (0.20F * t * t);
    }

    if ((l >= 415.0F) && (l < 475.0F)) {
        t = (l - 415.0F) / (475.0F - 415.0F);
        g = +(0.80F * t * t);
    } else if ((l >= 475.0F) && (l < 590.0F)) {
        t = (l - 475.0F) / (590.0F - 475.0F);
        g = 0.80F + (0.76F * t) - (0.80F * t * t);
    } else if ((l >= 585.0F) && (l < 639.0F)) {
        t = (l - 585.0F) / (639.0F - 585.0F);
        g = 0.84F - (0.84F * t);
    }

    if ((l >= 400.0F) && (l < 475.0F)) {
        t = (l - 400.0F) / (475.0F - 400.0F);
        b = +(2.20F * t) - (1.50F * t * t);
    } else if ((l >= 475.0F) && (l < 560.0F)) {
        t = (l - 475.0F) / (560.0F - 475.0F);
        b = 0.70F - (t) + (0.30F * t * t);
    }

    if (pr)
        *pr = r;
    if (pg)
        *pg = g;
    if (pb)
        *pb = b;
}

void test_display_fade(uint16_t cnt) {
    int b;
    color_t clr;
    int i;
    for (i = 0; i < cnt; i++) {
        b = 255 * i / (cnt - 1);
        clr = color_rgb(b, b, b);
        display::Clear(clr);
    }
}

void display_fill_rect_sub_rect(rect_ui16_t rc, rect_ui16_t rc1, color_t clr) {
    rect_ui16_t rc_t = { rc.x, rc.y, rc.w, uint16_t(rc1.y - rc.y) };
    rect_ui16_t rc_b = { rc.x, uint16_t(rc1.y + rc1.h), rc.w, uint16_t((rc.y + rc.h) - (rc1.y + rc1.h)) };
    rect_ui16_t rc_l = { rc.x, rc.y, uint16_t(rc1.x - rc.x), rc.h };
    rect_ui16_t rc_r = { uint16_t(rc1.x + rc1.w), rc.y, uint16_t((rc.x + rc.w) - (rc1.x + rc1.w)), rc.h };
    //display::FillRect(rc, clr);
    display::FillRect(rc_t, clr);
    display::FillRect(rc_b, clr);
    display::FillRect(rc_l, clr);
    display::FillRect(rc_r, clr);
}

void test_display_rgbcolors(uint16_t cnt) {
    color_t colors[] = {
        COLOR_BLACK, COLOR_WHITE,
        COLOR_RED, COLOR_LIME, COLOR_BLUE,
        COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
        COLOR_SILVER, COLOR_GRAY,
        COLOR_MAROON, COLOR_OLIVE, COLOR_GREEN,
        COLOR_PURPLE, COLOR_TEAL, COLOR_NAVY
    };
    const char *names[] = {
        "BLACK", "WHITE",
        "RED", "LIME", "BLUE",
        "YELLOW", "CYAN", "MAGENTA",
        "SILVER", "GRAY",
        "MAROON", "OLIVE", "GREEN",
        "PURPLE", "TEAL", "NAVY"
    };
    int count = sizeof(colors) / sizeof(color_t);
    font_t *font = resource_font(IDR_FNT_NORMAL);
    int item_height = 20;
    int i;
    int n;
    for (n = 0; n < cnt; n++)
        for (i = 0; i < count; i++) {
            int chars = strlen(names[i]);
            int text_w = chars * font->w;
            int text_h = font->h;
            rect_ui16_t rc_item = rect_ui16(0, item_height * i, 240, item_height);
            rect_ui16_t rc_text = rect_ui16(10, item_height * i + 1, text_w, text_h);
            //display::FillRect(rc_item, colors[i]);
            display_fill_rect_sub_rect(rc_item, rc_text, colors[i]);
            display::DrawText(rc_text, names[i], font, colors[i], (i == 0) ? COLOR_WHITE : COLOR_BLACK);
        }
}

void test_display_spectrum(uint16_t cnt) {
    //	int i;
    int y;
    float l;
    float r;
    float g;
    float b;
    color_t clr;
    for (int n = 0; n < cnt; ++n)
        for (y = 0; y < display::GetH(); ++y) {
            l = 400.0F + (3.0F * y / 3.2F);
            spectral_color(l, &r, &g, &b);
            clr = color_rgb(255 * r, 255 * g, 255 * b);
            display::DrawLine(point_ui16(0, y), point_ui16(display::GetW() - 1, y), clr);
        }
}

//extern uint8_t png_data[];
//extern const uint8_t png_a3ides_logo[];
//extern const uint8_t png_splash_screen[];
//extern const uint8_t png_status_screen[];
//extern const uint8_t png_main_menu[];

extern const uint8_t png_icon_64x64_noise[];
extern const uint16_t png_icon_64x64_noise_size;

void test_display_random_png_64x64(uint16_t count) {
    uint16_t x;
    uint16_t y;
    FILE *pf = fmemopen((void *)png_icon_64x64_noise, png_icon_64x64_noise_size, "rb");
    for (uint16_t n = 0; n < count; n++) {
        x = rand() % (display::GetW() - 64 + 1);
        y = rand() % (display::GetH() - 64 + 1);
        display::DrawPng(point_ui16(x, y), pf);
    }
    fclose(pf);
}

int __read(struct _reent *_r, void *pv, char *pc, int n) {
    return 0;
}

int __write(struct _reent *_r, void *pv, const char *pc, int n) {
    return 0;
}

void test_display2(void) {
    //	FILE* pf = fmemopen(png_a3ides_logo, 2889, "rb");
    //	FILE* pf0 = fmemopen(png_splash_screen, 10318, "rb");
    //	FILE* pf1 = fmemopen((void*)png_status_screen, 11438, "rb");
    //	FILE* pf2 = fmemopen(png_main_menu, 4907, "rb");
    //	uint8_t buf[10];
    //	int c = fread(buf, 1, 6, pf);

    /*	FILE f;
		memset(&f, 0, sizeof(f));
		f._read = __read;
	//	f._write = __write;
		f._file = -1;
		f._flags = __SRD | __SNBF;
		f._blksize = 0;
		f._lbfsize = 0;
	//	f.
		uint8_t buf[10];
		int c = fread(buf, 1, 1, &f);
	*/
    //	printf("test\n");
    while (1) {
//		osDelay((c>0)?c:0);
//		if (pf)
//	  pf = fopen("test.x", "rb");
//fdev_setup_stream()
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, 1)
#if 0
		uint32_t tim;
		uint32_t spd;
#endif

#if 0
		tim = _microseconds();
		display::Clear(COLOR_BLACK);
		tim = _microseconds() - tim;
		_dbg3("display_clear %u", tim);
#endif

#if 0
		display::Clear(COLOR_BLACK);
		tim = _microseconds();
		test_display_random_png_64x64(100);
		tim = _microseconds() - tim;
		spd = 100 * 1000000 / tim;
		_dbg3("test_random_png_64x64 %u (%u icons/sec)", tim, spd);
		osDelay(1000);
#endif

        /*		tim = _microseconds();
		display::FillRect(64, 64, 128, 128, CLR565_BLUE);
		tim = _microseconds() - tim;
		_dbg3("fill_rect %u", tim);
		osDelay(1000);*/

        //	  	st7789v_draw_pict(10, 10, 83, 63, (uint16_t*)png_a3ides_logo);
        //	  	st7789v_draw_png(10, 10, pf);
        //	  	osDelay(1000);
        //	  	st7789v_draw_png(0, 0, pf0);
        //	  	osDelay(1000);

        //		tim = _microseconds();
        //		display::DrawPng(0, 0, pf1);
        //		tim = _microseconds() - tim;
        //		_dbg3("draw_png %u", tim);
        //		osDelay(1000);
        //	  	st7789v_draw_png(0, 0, pf2);
        //	  	osDelay(2000);
        /*
		osDelay(1000);*/
        /*	  	st7789v_display_clear(CLR565_GREEN);
	  //	st7789v_draw_line(0, 0, 239, 0, CLR565_BLUE);
		st7789v_display_clear(CLR565_RED);
		st7789v_fill_rect(20, 20, 64, 64, CLR565_YELLOW);
		osDelay(1000);
	  //	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, 0);
		st7789v_display_clear(CLR565_GREEN);
		osDelay(1000);
		st7789v_display_clear(CLR565_BLUE);
		st7789v_draw_text(10, 10, 0, 0, "Testik", &font_12x12, CLR565_YELLOW);
		osDelay(1000);
		//osDelay(1000);
		st7789v_spectrum();
		osDelay(1000);
		//osDelay(1000);
		for (i = 0; i < 200; i++)
		{
			disp_set_pixel(i, i, CLR565_RED);
		}
		osDelay(1000);*/
    }
}
