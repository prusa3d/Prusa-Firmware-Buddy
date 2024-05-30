#include <fcntl.h>
#include "ScreenShot.hpp"
#include "display.h"
#include <unique_file_ptr.hpp>
#include <scope_guard.hpp>
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <guiconfig/GuiDefaults.hpp>
#include <guiconfig/guiconfig.h>

#if defined(USE_ILI9488)
    #include "ili9488.hpp"
#elif defined(USE_ST7789)
    #include "st7789v.hpp"
#else
    #error
#endif

namespace {

#if defined(USE_ILI9488)

// 3 bytes per pixel when reading
using Pixel = uint8_t[3];

constexpr uint8_t bytes_per_pixel = 3;
constexpr uint8_t buffer_rows = ILI9488_BUFF_ROWS;
constexpr uint8_t read_start_offset = 0;

void transform_pixel(Pixel &pixel) {
    // The display has 6 bits per color component, so we have to shift it left by two bits
    pixel[0] <<= 2;
    pixel[1] <<= 2;
    pixel[2] <<= 2;
}

#elif defined(USE_ST7789)

// 3 bytes per pixel when reading
struct Pixel {
    uint8_t a, b, c;
};

constexpr uint8_t bytes_per_pixel = 3;

// For whatever reason, we need to skip two bytes read from the RAMRD command
constexpr uint8_t read_start_offset = 2;

// We cannot use ST7789V_BUFF_COLS, because that is assuming 2 bytes per pixel. But readouts is done in 3 bytes per pixel.
constexpr uint8_t buffer_rows = (ST7789V_BUFFER_SIZE - read_start_offset) / (ST7789V_COLS * 3);

void transform_pixel(Pixel &pixel) {
    // The display has 6 bits per color component, so we have to shift it left by two bits
    // Also do some order swapping
    pixel = Pixel { static_cast<uint8_t>(pixel.c << 2), static_cast<uint8_t>(pixel.b << 2), static_cast<uint8_t>(pixel.a << 2) };
}

#else
    #error
#endif

static_assert(sizeof(Pixel) == bytes_per_pixel);

void transform_buffer(Pixel *buffer) {
    // Y-axis mirror image - because BMP pixel format has base origin in left-bottom corner not in left-top like on displays
    for (int row = 0; row < buffer_rows / 2; row++) {
        const auto offset1 = row * display::GetW();
        const auto offset2 = (buffer_rows - row - 1) * display::GetW();

        for (int col = 0; col < display::GetW(); col++) {
            std::swap(buffer[offset1 + col], buffer[offset2 + col]);
        }
    }

    // Apply display-specific pixel data transformations
    for (Pixel *p = buffer, *e = buffer + buffer_rows * display::GetW(); p != e; p++) {
        transform_pixel(*p);
    }
}

enum {
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,
    BMP_HEADER_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE,

    BMP_IMAGE_DATA_SIZE = display::GetW() * display::GetH() * bytes_per_pixel,
    BMP_FILE_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + BMP_IMAGE_DATA_SIZE,
    SCREENSHOT_FILE_NAME_MAX_LEN = 30,
    SCREENSHOT_FILE_NAME_BUFFER_LEN = SCREENSHOT_FILE_NAME_MAX_LEN + 3,
};

constexpr const char screenshot_name[] = "/usb/screenshot";
constexpr const char screenshot_format[] = ".bmp";

constexpr const uint8_t bmp_header[] = {
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

} // namespace

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
    // Delete the file if the ssshot taking fails
    ScopeGuard delete_file_guard = [&] { unlink(file_name); };

    const unique_file_ptr f(fopen(file_name, "w"));
    if (!f) {
        return false;
    }

    const int header_size = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
    if (fwrite(&bmp_header, 1, header_size, f.get()) != header_size) {
        return false;
    }

    for (int block = display::GetH() / buffer_rows - 1; block >= 0; block--) {
        const point_ui16_t start = point_ui16(0, block * buffer_rows);
        const point_ui16_t end = point_ui16(display::GetW() - 1, (block + 1) * buffer_rows - 1);
        uint8_t *buffer = display::GetBlock(start, end); // this pointer is valid only until another display memory write is called
        if (!buffer) {
            return false;
        }

        transform_buffer(reinterpret_cast<Pixel *>(buffer + read_start_offset));

        const int write_size = display::GetW() * buffer_rows * bytes_per_pixel;
        if (fwrite(buffer + read_start_offset, 1, write_size, f.get()) != write_size) {
            return false;
        }
    }

    delete_file_guard.disarm();
    return true;
}
