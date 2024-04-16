#include <fcntl.h>
#include "ScreenShot.hpp"
#include "display.h"
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <guiconfig/GuiDefaults.hpp>
#include <guiconfig/guiconfig.h>

#ifdef USE_ST7789
static const uint8_t bytes_per_pixel = 3;
static const uint8_t buffer_rows = 10;
static const uint8_t read_start_offset = 2;
    #include "st7789v.hpp"
#endif // USE_ST7789
#ifdef USE_ILI9488
static const uint8_t bytes_per_pixel = 3;
static const uint8_t buffer_rows = ILI9488_BUFF_ROWS;
static const uint8_t read_start_offset = 0;
    #include "ili9488.hpp"
#endif // USE_ILI9488

enum {
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,

    BMP_FILE_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + display::GetW() * display::GetH() * bytes_per_pixel,
    SCREENSHOT_FILE_NAME_MAX_LEN = 30,
    SCREENSHOT_FILE_NAME_BUFFER_LEN = SCREENSHOT_FILE_NAME_MAX_LEN + 3,
};

static const char screenshot_name[] = "/usb/screenshot";
static const char screenshot_format[] = ".bmp";

static const unsigned char bmp_header[] = {
    'B', 'M', /// type "BM"                   [2B]
    (unsigned char)BMP_FILE_SIZE, /// image file size in bytes    [4B]
    (unsigned char)(BMP_FILE_SIZE >> 8),
    (unsigned char)(BMP_FILE_SIZE >> 16),
    (unsigned char)(BMP_FILE_SIZE >> 24),
    0, 0, 0, 0, /// reserved                    [4B]
    (unsigned char)(BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE), 0, 0, 0, /// start of pixel array        [4B]
    (unsigned char)BMP_INFO_HEADER_SIZE, 0, 0, 0, /// header size                 [4B]
    (unsigned char)display::GetW(), /// image width                 [4B]
    (unsigned char)(display::GetW() >> 8),
    0,
    0,
    (unsigned char)display::GetH(), /// image height                [4B]
    (unsigned char)(display::GetH() >> 8),
    0,
    0,
    1, 0, /// number of color planes      [2B]
    (unsigned char)(bytes_per_pixel * 8), 0, /// bits per pixel              [2B]
    0, 0, 0, 0, /// compression                 [4B]
    (unsigned char)(display::GetW() * display::GetH() * bytes_per_pixel), /// image size                  [4B]
    (unsigned char)((display::GetW() * display::GetH() * bytes_per_pixel) >> 8),
    (unsigned char)((display::GetW() * display::GetH() * bytes_per_pixel) >> 16),
    0,
    0, 0, 0, 0, /// horizontal resolution       [4B]
    0, 0, 0, 0, /// vertical resolution         [4B]
    0, 0, 0, 0, /// colors in color table       [4B]
    0, 0, 0, 0, /// important color count       [4B]
};

static void mirror_buffer(Pixel *buffer) {
    // Y-axis mirror image - because BMP pixel format has base origin in left-bottom corner not in left-top like on displays
    // BMP headers have to know that we are using 2B / 3B pixels.
    for (int row = 0; row < buffer_rows / 2; row++) {
        for (int col = 0; col < display::GetW(); col++) {
            const int i1 = row * display::GetW() + col;
            const int i2 = (buffer_rows - row - 1) * display::GetW() + col;
#ifdef USE_ST7789
            // we need to swap the colors, because bmp is in BGR color format
            buffer[i1].SwapBlueAndRed();
            buffer[i2].SwapBlueAndRed();
            std::swap(buffer[i1], buffer[i2]);
#elif defined USE_ILI9488
            Pixel swapper = buffer[i1];
            buffer[i1] = buffer[i2]; // move 6 bit input to 8 bit scale
            buffer[i2] = swapper;
            buffer[i1].ShiftColorsUp(2); // move 6 bit input to 8 bit scale
            buffer[i2].ShiftColorsUp(2);

#else
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

    return TakeAScreenshotAs(file_name);
}

bool TakeAScreenshotAs(const char *file_name) {
    FILE *fd = fopen(file_name, "w");
    if (fd == nullptr) {
        return false;
    }

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
            mirror_buffer(reinterpret_cast<Pixel *>(buffer + read_start_offset));
            const int write_size = display::GetW() * buffer_rows * bytes_per_pixel;
            if (fwrite(buffer + read_start_offset, 1, write_size, fd) != write_size) {
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

Pixel::Pixel(const uint8_t *data) {
    red = data[0];
    green = data[1];
    blue = data[2];
}
void Pixel::SwapBlueAndRed() {
    std::swap(red, blue);
}
void Pixel::ShiftColorsUp(int bits) {
    red <<= bits;
    blue <<= bits;
    green <<= bits;
}
