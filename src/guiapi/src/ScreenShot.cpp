#include "ScreenShot.hpp"
#include "st7789v.h"
#include "display.h"
#include <inttypes.h>
#include "ff.h"

#define BMP_FILE_HEADER_SIZE         14
#define BMP_INFO_HEADER_SIZE         40
#define BYTES_PER_PIXEL              3 // R(1B) + G(1B) + B(1B)
#define BMP_FILE_SIZE                (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + display::GetW() * display::GetH() * BYTES_PER_PIXEL)
#define SCREENSHOT_FILE_NAME_MAX_LEN 30

static const char screenshot_name[] = "/screenshot";
static const char screenshot_format[] = ".bmp";

static const unsigned char bmp_header[] = {
    'B', 'M',                     /// type "BM"                   [2B]
    (unsigned char)BMP_FILE_SIZE, /// image file size in bytes    [4B]
    (unsigned char)(BMP_FILE_SIZE >> 8),
    (unsigned char)(BMP_FILE_SIZE >> 16),
    (unsigned char)(BMP_FILE_SIZE >> 24),
    0, 0, 0, 0,                                                            /// reserved                    [4B]
    (unsigned char)(BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE), 0, 0, 0, /// start of pixel array        [4B]
    (unsigned char)BMP_INFO_HEADER_SIZE, 0, 0, 0,                          /// header size                 [4B]
    (unsigned char)display::GetW(),                                        /// image width                 [4B]
    (unsigned char)(display::GetW() >> 8),
    0,
    0,
    (unsigned char)display::GetH(), /// image height                [4B]
    (unsigned char)(display::GetH() >> 8),
    0,
    0,
    1, 0,                                                                 /// number of color planes      [2B]
    (unsigned char)(BYTES_PER_PIXEL * 8), 0,                              /// bits per pixel              [2B]
    0, 0, 0, 0,                                                           /// compression                 [4B]
    (unsigned char)(display::GetW() * display::GetH() * BYTES_PER_PIXEL), /// image size                  [4B]
    (unsigned char)((display::GetW() * display::GetH() * BYTES_PER_PIXEL) >> 8),
    (unsigned char)((display::GetW() * display::GetH() * BYTES_PER_PIXEL) >> 16),
    0,
    0, 0, 0, 0, /// horizontal resolution       [4B]
    0, 0, 0, 0, /// vertical resolution         [4B]
    0, 0, 0, 0, /// colors in color table       [4B]
    0, 0, 0, 0, /// important color count       [4B]
};

static void mirror_buffer(uint8_t *buffer) {
    // Y-axis mirror image - because BMP pixel format has base origin in left-bottom corner and ili9488 in left-upper corner
    // 6bit color chanals (0xFC) needs to be shifted to 8bit color chanals
    for (int row = 0; row < ILI9488_BUFF_ROWS / 2; row++) {
        for (int col = 0; col < ST7789V_COLS * BYTES_PER_PIXEL; col += BYTES_PER_PIXEL) {
            for (int chan = 0; chan < BYTES_PER_PIXEL; chan++) {
                uint8_t swapper = (buffer[row * ST7789V_COLS * BYTES_PER_PIXEL + col + chan] & 0xFC) << 2;
                buffer[row * ST7789V_COLS * BYTES_PER_PIXEL + col + chan] = (buffer[(ILI9488_BUFF_ROWS - row - 1) * ST7789V_COLS * BYTES_PER_PIXEL + col + chan] & 0xFC) << 2;
                buffer[(ILI9488_BUFF_ROWS - row - 1) * ST7789V_COLS * BYTES_PER_PIXEL + col + chan] = swapper;
            }
        }
    }
}

bool TakeAScreenshot() {

    FIL screenshot_file;
    char file_name[SCREENSHOT_FILE_NAME_MAX_LEN + 1];
    bool init = false;

    bool success = true;

    UINT written_bytes = 0; // TODO: add written bytes check if needed

    if (success) {
        FILINFO file_info;
        FRESULT res;
        uint32_t inc = 1;
        snprintf(file_name, SCREENSHOT_FILE_NAME_MAX_LEN + 1, "%s_%lu%s", screenshot_name, inc, screenshot_format);
        while ((res = f_stat(file_name, &file_info)) != FR_NO_FILE) {
            inc++;
            snprintf(file_name, SCREENSHOT_FILE_NAME_MAX_LEN + 1, "%s_%lu%s", screenshot_name, inc, screenshot_format);
            if (res == FR_NOT_ENABLED || inc == 0) {
                success = false;
                break;
            }
        }
    }

    if (success) {
        success = FR_OK == f_open(&screenshot_file, file_name, FA_WRITE | FA_CREATE_NEW | FA_OPEN_APPEND);
    }

    if (success) {
        init = true;
        success = FR_OK == f_write(&screenshot_file, &bmp_header, BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE, &written_bytes);
    }

    UINT written = 0;

    if (success) {
        for (uint8_t block = ILI9488_ROWS / ILI9488_BUFF_ROWS - 1; block >= 0 && block < ILI9488_ROWS / ILI9488_BUFF_ROWS; block--) {
            point_ui16_t start = point_ui16(0, block * ILI9488_BUFF_ROWS);
            point_ui16_t end = point_ui16(ILI9488_COLS - 1, (block + 1) * ILI9488_BUFF_ROWS - 1);
            uint8_t *buffer = ili9488_get_block(start.x, start.y, end.x, end.y); // this pointer is valid only until another set_pixel or fill_rect is called
            if (buffer == NULL) {
                success = false;
                break;
            } else {
                mirror_buffer(buffer);
                if (FR_OK != f_write(&screenshot_file, buffer, ILI9488_COLS * ILI9488_BUFF_ROWS * BYTES_PER_PIXEL, &written)) {
                    success = false;
                    break;
                }
            }
        }
    }

    if (init) {
        if (success) {
            success = FR_OK == f_close(&screenshot_file);
        } else {
            f_close(&screenshot_file);
        }
    }

    if (init && !success) {
        f_unlink(file_name);
        return false;
    }

    return true;
}
