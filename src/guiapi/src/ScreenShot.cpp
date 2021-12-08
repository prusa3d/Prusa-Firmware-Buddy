#include <fcntl.h>
//#include <string.h>
#include <unistd.h>
#include "ScreenShot.hpp"
#include "st7789v.hpp"
#include "display.h"
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

enum {
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,

    ST7789V_BYTES_PER_PIXEL = 2, // R(5b) + G(6b) + B(5b) = 16b = 2B
    BMP_FILE_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + display::GetW() * display::GetH() * ST7789V_BYTES_PER_PIXEL,
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
    1, 0,                                                                         /// number of color planes      [2B]
    (unsigned char)(ST7789V_BYTES_PER_PIXEL * 8), 0,                              /// bits per pixel              [2B]
    0, 0, 0, 0,                                                                   /// compression                 [4B]
    (unsigned char)(display::GetW() * display::GetH() * ST7789V_BYTES_PER_PIXEL), /// image size                  [4B]
    (unsigned char)((display::GetW() * display::GetH() * ST7789V_BYTES_PER_PIXEL) >> 8),
    (unsigned char)((display::GetW() * display::GetH() * ST7789V_BYTES_PER_PIXEL) >> 16),
    0,
    0, 0, 0, 0, /// horizontal resolution       [4B]
    0, 0, 0, 0, /// vertical resolution         [4B]
    0, 0, 0, 0, /// colors in color table       [4B]
    0, 0, 0, 0, /// important color count       [4B]
};

static void mirror_buffer(uint8_t *buffer) {
    // Y-axis mirror image - because BMP pixel format has base origin in left-bottom corner and st7789v in left-upper corner

    // TODO: BMP headers have to know that we are using 2B pixels. Now it is only in [bits per pixel] and clearly it's not enough.

    for (int row = 0; row < ST7789V_BUFF_ROWS / 2; row++) {
        for (int col = 0; col < ST7789V_COLS * ST7789V_BYTES_PER_PIXEL; col += ST7789V_BYTES_PER_PIXEL) {
            for (int chan = 0; chan < ST7789V_BYTES_PER_PIXEL; chan++) {
                std::swap(buffer[buffer[row * ST7789V_COLS * ST7789V_BYTES_PER_PIXEL + col + chan]], buffer[(ST7789V_BUFF_ROWS - row - 1) * ST7789V_COLS * ST7789V_BYTES_PER_PIXEL + col + chan]);
            }
        }
    }
}

bool TakeAScreenshot() {
    int fd;
    char file_name[SCREENSHOT_FILE_NAME_BUFFER_LEN];

    bool success;

    // TODO: add written bytes check if needed

    uint32_t inc = 1;
    snprintf(file_name, SCREENSHOT_FILE_NAME_BUFFER_LEN, "%s_%lu%s", screenshot_name, inc, screenshot_format);
    while ((access(file_name, F_OK)) == 0) {
        inc++;
        snprintf(file_name, SCREENSHOT_FILE_NAME_BUFFER_LEN, "%s_%lu%s", screenshot_name, inc, screenshot_format);
    }

    fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT);
    if (fd < 0) {
        return false;
    }

    const int header_size = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
    success = write(fd, &bmp_header, header_size) == header_size;

    if (success) {
        for (uint8_t block = ST7789V_ROWS / ST7789V_BUFF_ROWS - 1; block >= 0 && block < ST7789V_ROWS / ST7789V_BUFF_ROWS; block--) {
            point_ui16_t start = point_ui16(0, block * ST7789V_BUFF_ROWS);
            point_ui16_t end = point_ui16(ST7789V_COLS - 1, (block + 1) * ST7789V_BUFF_ROWS - 1);
            uint8_t *buffer = display::GetBlock(start, end); // this pointer is valid only until another display memory write is called
            if (buffer == NULL) {
                success = false;
                break;
            } else {
                mirror_buffer(buffer);
                const int write_size = ST7789V_COLS * ST7789V_BUFF_ROWS * ST7789V_BYTES_PER_PIXEL;
                if (write(fd, buffer, write_size) != write_size) {
                    success = false;
                    break;
                }
            }
        }
    }

    close(fd);

    if (!success) {
        unlink(file_name);
        return false;
    }

    return true;
}
