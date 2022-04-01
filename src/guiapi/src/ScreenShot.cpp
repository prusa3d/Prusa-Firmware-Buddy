#include <fcntl.h>
#include <unistd.h>
#include "ScreenShot.hpp"
#include "display.h"
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#ifdef USE_ST7789
static const uint8_t bytes_per_pixel = 2;
static const uint8_t buffer_rows = ST7789V_BUFF_ROWS;
    #include "st7789v.hpp"
#endif // USE_ST7789

enum {
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,

    BMP_FILE_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + display::GetW() * display::GetH() * bytes_per_pixel,
    SCREENSHOT_FILE_NAME_MAX_LEN = 30,
    SCREENSHOT_FILE_NAME_BUFFER_LEN = SCREENSHOT_FILE_NAME_MAX_LEN + 1,
};

static const char screenshot_name[] = "/usb/screenshot";
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
    (unsigned char)(bytes_per_pixel * 8), 0,                              /// bits per pixel              [2B]
    0, 0, 0, 0,                                                           /// compression                 [4B]
    (unsigned char)(display::GetW() * display::GetH() * bytes_per_pixel), /// image size                  [4B]
    (unsigned char)((display::GetW() * display::GetH() * bytes_per_pixel) >> 8),
    (unsigned char)((display::GetW() * display::GetH() * bytes_per_pixel) >> 16),
    0,
    0, 0, 0, 0, /// horizontal resolution       [4B]
    0, 0, 0, 0, /// vertical resolution         [4B]
    0, 0, 0, 0, /// colors in color table       [4B]
    0, 0, 0, 0, /// important color count       [4B]
};

static void mirror_buffer(uint8_t *buffer) {
    // Y-axis mirror image - because BMP pixel format has base origin in left-bottom corner not in left-top like on displays
    for (int row = 0; row < buffer_rows / 2; row++) {
        for (int col = 0; col < display::GetW() * bytes_per_pixel; col++) {
#ifdef USE_ST7789
            const int i1 = row * display::GetW() * bytes_per_pixel + col;
            const int i2 = (buffer_rows - row - 1) * display::GetW() * bytes_per_pixel + col;
            std::swap(buffer[i1], buffer[i2]);
#else // USE_ST7789
    #error "Unsupported display for screenshot."
#endif
        }
    }
}

bool TakeAScreenshot() {
    char file_name[SCREENSHOT_FILE_NAME_BUFFER_LEN];
    uint32_t inc = 1;
    snprintf(file_name, SCREENSHOT_FILE_NAME_BUFFER_LEN, "%s_%lu%s", screenshot_name, inc, screenshot_format);
    while ((access(file_name, F_OK)) == 0) {
        inc++;
        snprintf(file_name, SCREENSHOT_FILE_NAME_BUFFER_LEN, "%s_%lu%s", screenshot_name, inc, screenshot_format);
    }

    FILE *fd = fopen(file_name, "w");
    if (fd == nullptr)
        return false;

    const int header_size = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
    bool success = fwrite(&bmp_header, 1, header_size, fd) == header_size;

    if (success) {
        for (int block = display::GetH() / buffer_rows - 1; block >= 0; block--) {
            point_ui16_t start = point_ui16(0, block * buffer_rows);
            point_ui16_t end = point_ui16(display::GetW() - 1, (block + 1) * buffer_rows - 1);
            uint8_t *buffer = display::GetBlock(start, end); // this pointer is valid only until another display memory write is called
            if (buffer == NULL) {
                success = false;
                break;
            }
            mirror_buffer(buffer);
            const int write_size = display::GetW() * buffer_rows * bytes_per_pixel;
            if (fwrite(buffer, 1, write_size, fd) != write_size) {
                success = false;
                break;
            }
        }
    }

    fclose(fd);

    if (!success) {
        unlink(file_name);
        return false;
    }

    return true;
}
