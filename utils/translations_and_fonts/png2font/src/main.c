// png2font - main.c

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "png.h"
#include "pngutils.h"

#define MAX_PATH 1024

int png2font(char *src_filename, char *dst_filename, char *out_filename, int char_w, int char_h, int char_bpp, int charset_cols, int charset_rows);
void font_preview(png_structp dst_png, png_infop dst_ppi, uint8_t *dst_data, uint8_t *charset_data, int char_w, int char_h, int char_bpp, int charset_cols, int charset_rows);

// int png2font_old(char* src_filename, char* dst_filename, int char_w, int char_h, int cols, int rows);

// #define _DEBUG_ARGS
#ifdef _DEBUG_ARGS
//-src=d:\test.png -dst=d:\test-preview.png -out=d:\test.bin -bpp=2 -w=16 -h=20 -c=16 -r=6
char *args[] = {
    "test",
    "-src=d:/test.png",
    "-dst=d:/test-preview.png",
    "-out=d:/test.bin",
    "-bpp=4",
    "-w=11",
    "-h=18",
    "-c=16",
    "-r=6",
};
int main(int argc, char **argv) {
    argc = sizeof(args) / sizeof(char *);
    argv = args;
#else //_DEBUG_ARGS
int main(int argc, char **argv) {
#endif //_DEBUG_ARGS

    int ret = 0;
    int argn = 0;
    char *arg = 0;
    char src_filename[MAX_PATH] = "";
    char dst_filename[MAX_PATH] = "";
    char out_filename[MAX_PATH] = "";
    int char_w = 0;
    int char_h = 0;
    int char_bpp = 0;
    int charset_cols = 0;
    int charset_rows = 0;
    // parse args
    while (++argn < argc) {
        arg = argv[argn];
        if (sscanf(arg, "-src=%s", src_filename) == 1) {
            continue;
        }
        if (sscanf(arg, "-dst=%s", dst_filename) == 1) {
            continue;
        }
        if (sscanf(arg, "-out=%s", out_filename) == 1) {
            continue;
        }
        if (sscanf(arg, "-bpp=%d", &char_bpp) == 1) {
            continue;
        }
        if (sscanf(arg, "-w=%d", &char_w) == 1) {
            continue;
        }
        if (sscanf(arg, "-h=%d", &char_h) == 1) {
            continue;
        }
        if (sscanf(arg, "-c=%d", &charset_cols) == 1) {
            continue;
        }
        if (sscanf(arg, "-r=%d", &charset_rows) == 1) {
            continue;
        }
    }
    // check args
    if ((ret == 0) && (strlen(src_filename) == 0)) {
        fputs("SRC_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && (strlen(dst_filename) == 0)) {
        fputs("DST_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && (strlen(out_filename) == 0)) {
        fputs("OUT_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && ((char_w < 1) || (char_w > 256))) {
        fputs("WIDTH out of range!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && ((char_h < 1) || (char_w > 256))) {
        fputs("HEIGHT out of range!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && ((charset_cols < 1) || (charset_cols > 256))) {
        fputs("COLS out of range!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && ((charset_rows < 1) || (charset_rows > 256))) {
        fputs("ROWS out of range!\n", stderr);
        ret = 1;
    }
    if ((ret == 0) && ((char_bpp != 1) && (char_bpp != 2) && (char_bpp != 4) && (char_bpp != 8))) {
        fputs("BITSPERPIXEL out of range!\n", stderr);
        ret = 1;
    }
    if (ret != 0) {
        printf("Charset convertor png2font\n");
        printf(" arguments:\n");
        printf("  -src=SRC_FILENAME  source png file name\n");
        printf("  -dst=DST_FILENAME  destination (preview) png file name\n");
        printf("  -out=OUT_FILENAME  output charset binary file name\n");
        printf("  -bpp=BITSPERPIXEL  bits per pixel (1,2,4,8, default 1)\n");
        printf("  -w=WIDTH           character width in pixels (1..256)\n");
        printf("  -h=HEIGHT          character height in pixels (1..256)\n");
        printf("  -c=COLS            charset cols (1..256)\n");
        printf("  -r=ROWS            charset rows (1..256)\n");
        getchar();
    }
    if (ret == 0) {
        ret = png2font(src_filename, dst_filename, out_filename, char_w, char_h, char_bpp, charset_cols, charset_rows);
    }

    return ret;
}

// png2font return values
#define PNG2FONT_OK          0
#define PNG2FONT_ERROR_IO    -1
#define PNG2FONT_ERROR_INVAL -2

// first bit set
int fbs(unsigned int val) {
    if (val == 0) {
        return -1;
    }
    int bit = 0;
    while ((val & 1) == 0) {
        bit++;
        val >>= 1;
    }
    return bit;
}

int png2font(char *src_filename, char *dst_filename, char *out_filename, int char_w, int char_h, int char_bpp, int charset_cols, int charset_rows) {
    // source png variables
    FILE *src = 0;
    png_structp src_png = 0;
    png_infop src_ppi = 0;
    int src_w = 0;
    int src_h = 0;
    int src_row_size = 0;
    int src_pix_size = 0;
    int src_size = 0;
    uint8_t *src_data = 0;
    uint8_t *src_pp = 0;
    uint8_t **src_rows = 0;

    // destination png variables
    FILE *dst = 0;
    png_structp dst_png = 0;
    png_infop dst_ppi = 0;
    int dst_w = 0;
    int dst_h = 0;
    int dst_row_size = 0;
    int dst_pix_size = 0;
    int dst_size = 0;
    uint8_t *dst_data = 0;
    uint8_t *dst_pp = 0;
    uint8_t **dst_rows = 0;

    // output charset variables
    FILE *out = 0;
    int char_ppb = 8 / char_bpp; // pixels per byte (bpp= 1:8, 2:4, 4:2, 8:1)
    int char_pms = ((1 << char_bpp) - 1); // pixel mask (bpp= 1:1, 2:3, 4:15, 8:255)
    int char_psr = (3 - fbs(char_bpp)); // pixel shift right (bpp =1:3, 2:2, 4:1, 8:0)
    int char_bpr = (char_w + char_ppb - 1) >> char_psr; // bytes per row
    int char_size = char_h * char_bpr; // character size in bytes
    int char_count = charset_cols * charset_rows; // character count
    int charset_size = char_size * char_count; // charset size in bytes
    uint8_t *charset_data = 0;

    // image process variables
    int i;
    int x;
    int y;
    int r;
    int g;
    int b;
    int lum;
    int opa;
    int opa8;
    int char_code;
    int char_x;
    int char_y;
    int char_offs;
    int char_row_offs;
    int char_pix_offs;
    int offs;

    // return value
    int ret = PNG2FONT_ERROR_IO;

    // open files
    src = fopen(src_filename, "rb");
    dst = fopen(dst_filename, "wb+");
    out = fopen(out_filename, "wb+");
    if ((!src) || (!dst) || (!out)) {
        goto _e_0;
    }

    // open source png, allocate memory for image data
    src_png = _png_open_read(src, &src_ppi);
    if (!src_png) {
        goto _e_1;
    }
    src_w = png_get_image_width(src_png, src_ppi);
    src_h = png_get_image_height(src_png, src_ppi);
    src_row_size = png_get_rowbytes(src_png, src_ppi);
    src_pix_size = src_row_size / src_w;
    src_size = src_row_size * src_h;
    src_data = (uint8_t *)malloc(src_size);
    if (!src_data) {
        goto _e_1;
    }
    memset(src_data, 0, src_size);
    src_rows = (uint8_t **)malloc(sizeof(void *) * src_h);
    if (!src_rows) {
        goto _e_1;
    }
    for (i = 0; i < src_h; i++) {
        src_rows[i] = src_data + i * src_row_size;
    }

    // open destination png, allocate memory for image data
    dst_png = _png_open_write(dst, &dst_ppi, src_w, src_h, PNG_COLOR_TYPE_RGB);
    if (!dst_png) {
        goto _e_2;
    }
    dst_w = png_get_image_width(dst_png, dst_ppi);
    dst_h = png_get_image_height(dst_png, dst_ppi);
    dst_row_size = png_get_rowbytes(dst_png, dst_ppi);
    dst_pix_size = dst_row_size / dst_w;
    dst_size = dst_row_size * dst_h;
    dst_data = (uint8_t *)malloc(dst_size);
    if (!dst_data) {
        goto _e_2;
    }
    memset(dst_data, 0, dst_size);
    dst_rows = (uint8_t **)malloc(sizeof(void *) * dst_h);
    if (!dst_rows) {
        goto _e_2;
    }
    for (i = 0; i < dst_h; i++) {
        dst_rows[i] = dst_data + i * dst_row_size;
    }

    // allocate memory for charset data
    charset_data = (uint8_t *)malloc(charset_size);
    if (!charset_data) {
        goto _e_3;
    }
    memset(charset_data, 0, charset_size); // set character data to zero

    // read source png data into memory
    png_read_image(src_png, src_rows);

    // process png data
    for (y = 0; y < src_h; y++) {
        for (x = 0; x < src_w; x++) {
            // source and destination pixel pointers
            src_pp = src_data + src_pix_size * x + src_row_size * y;
            dst_pp = dst_data + dst_pix_size * x + dst_row_size * y;
            // RGB values from src pixel
            r = src_pp[0];
            g = src_pp[1];
            b = src_pp[2];
            lum = (r + g + b) / 3; // luminance
            opa = 255 - lum; // opacity from luminance
            // TODO: opacity from alpha

            opa = opa >> (8 - char_bpp); // round opacity to bpp
            opa8 = 255 * opa / char_pms; // recalc to 8bit (0..255) for preview

            char_code = (x / char_w) + charset_cols * (y / char_h); // character code
            char_offs = char_code * char_size; // character offset in charset [bytes]
            char_x = (x % char_w); // character pixel x-coord (0..char_w-1)
            char_y = (y % char_h); // character pixel y-coord (0..char_h-1)
            char_row_offs = char_y * char_bpr; // character row offset [bytes]
            char_pix_offs = char_x >> char_psr; // character pixel offset [bytes]
            offs = char_offs + char_row_offs + char_pix_offs; // total offset in charset [bytes]
            i = (char_x % char_ppb);
            i = (8 - char_bpp) - (i * char_bpp);
            // i = (8 - char_bpp) - char_bpp * (char_x % (1 << char_bpp)); //shift in current byte
            // opa &= ((1 << char_bpp) - 1); //mask by bpp
            charset_data[offs] |= opa << i; // update character pixel data

#if 0
			//preview data in grayscale (r=g=b=opa8)
			dst_pp[0] = opa8;
			dst_pp[1] = opa8;
			dst_pp[2] = opa8;
#endif
        }
    }

    //	font_preview2(charset_data, dst_data, char_w, char_h, char_bpp, charset_cols, charset_rows);
    font_preview(dst_png, dst_ppi, dst_data, charset_data, char_w, char_h, char_bpp, charset_cols, charset_rows);

    // write destination png data from memory to file
    png_write_image(dst_png, dst_rows);

    // write charset data from memory to file
    fwrite(charset_data, charset_size, 1, out);

    ret = PNG2FONT_OK;

_e_3:
    // free charset memory
    if (charset_data) {
        free(charset_data);
    }
_e_2:
    // free destination png resources and memory
    if (dst_png) {
        png_destroy_write_struct(&dst_png, &dst_ppi);
    }
    if (dst_rows) {
        free(dst_rows);
    }
    if (dst_data) {
        free(dst_data);
    }
_e_1:
    // free source png resources and memory
    if (src_png) {
        png_destroy_read_struct(&src_png, &src_ppi, 0);
    }
    if (src_rows) {
        free(src_rows);
    }
    if (src_data) {
        free(src_data);
    }
_e_0:
    // close files
    if (out) {
        fclose(out);
    }
    if (dst) {
        fclose(dst);
    }
    if (src) {
        fclose(src);
    }
    return ret;
}

void print_char_to_png(png_structp dst_png, png_infop dst_ppi, uint8_t *dst_data, uint8_t *charset_data, int char_w, int char_h, int char_bpp, int charset_cols, int charset_rows, int x0, int y0, int char_code) {
    int i;
    int x;
    int y;
    int dst_w = png_get_image_width(dst_png, dst_ppi);
    int dst_h = png_get_image_height(dst_png, dst_ppi);
    int dst_row_size = png_get_rowbytes(dst_png, dst_ppi);
    int dst_pix_size = dst_row_size / dst_w;
    int dst_size = dst_row_size * dst_h;
    uint8_t *dst_pp = 0;
    int char_ppb = 8 / char_bpp; // pixels per byte (bpp= 1:8, 2:4, 4:2, 8:1)
    int char_pms = ((1 << char_bpp) - 1); // pixel mask (bpp= 1:1, 2:3, 4:15, 8:255)
    int char_psr = (3 - fbs(char_bpp)); // pixel shift right (bpp =1:3, 2:2, 4:1, 8:0)
    int char_bpr = (char_w + char_ppb - 1) >> char_psr; // bytes per row
    int char_size = char_h * char_bpr; // character size in bytes
    int char_count = charset_cols * charset_rows; // character count
    int charset_size = char_size * char_count; // charset size in bytes
    int opa;
    int opa8;
    uint8_t char_data = 0;
    for (y = 0; y < char_h; y++) {
        uint8_t *char_pp = charset_data + (char_size * char_code) + (y * char_bpr);
        for (x = 0; x < char_w; x++) {
            dst_pp = dst_data + (dst_pix_size * (x0 + x)) + (dst_row_size * (y0 + y));
            i = (x % char_ppb);
            if (i == 0) {
                char_data = *(char_pp++);
            }
            opa = (char_data >> (8 - char_bpp)) & char_pms;
            char_data <<= char_bpp;
            opa8 = 255 * opa / char_pms; // recalc to 8bit (0..255) for preview
            // preview data in grayscale (r=g=b=opa8)
            dst_pp[0] = opa8;
            dst_pp[1] = opa8;
            dst_pp[2] = opa8;
        }
    }
}

void font_preview(png_structp dst_png, png_infop dst_ppi, uint8_t *dst_data, uint8_t *charset_data, int char_w, int char_h, int char_bpp, int charset_cols, int charset_rows) {
    int x;
    int y;
    int col;
    int row;
    int char_code;
    for (row = 0; row < charset_rows; row++) {
        for (col = 0; col < charset_cols; col++) {
            x = col * char_w;
            y = row * char_h;
            char_code = col + row * charset_cols;
            print_char_to_png(dst_png, dst_ppi, dst_data, charset_data, char_w, char_h, char_bpp, charset_cols, charset_rows, x, y, char_code);
        }
    }
}
