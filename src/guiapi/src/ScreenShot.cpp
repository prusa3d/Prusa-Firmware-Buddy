#include <fcntl.h>
#include "ScreenShot.hpp"
#include "display.hpp"
#include <unique_file_ptr.hpp>
#include <scope_guard.hpp>
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <guiconfig/GuiDefaults.hpp>
#include <guiconfig/guiconfig.h>

#if HAS_ILI9488_DISPLAY()
    #include "ili9488.hpp"
#elif HAS_ST7789_DISPLAY()
    #include "st7789v.hpp"
#else
    #error
#endif

namespace {

#if HAS_ILI9488_DISPLAY()

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

#elif HAS_ST7789_DISPLAY()

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
        const auto offset1 = row * GuiDefaults::ScreenWidth;
        const auto offset2 = (buffer_rows - row - 1) * GuiDefaults::ScreenWidth;

        for (size_t col = 0; col < GuiDefaults::ScreenWidth; col++) {
            std::swap(buffer[offset1 + col], buffer[offset2 + col]);
        }
    }

    // Apply display-specific pixel data transformations
    for (Pixel *p = buffer, *e = buffer + buffer_rows * GuiDefaults::ScreenWidth; p != e; p++) {
        transform_pixel(*p);
    }
}

enum {
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,
    BMP_HEADER_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE,

    BMP_IMAGE_DATA_SIZE = GuiDefaults::ScreenWidth * GuiDefaults::ScreenHeight * bytes_per_pixel,
    BMP_FILE_SIZE = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + BMP_IMAGE_DATA_SIZE,
    SCREENSHOT_FILE_NAME_MAX_LEN = 30,
    SCREENSHOT_FILE_NAME_BUFFER_LEN = SCREENSHOT_FILE_NAME_MAX_LEN + 3,
};

constexpr const char screenshot_name[] = "/usb/screenshot";
constexpr const char screenshot_format[] = ".bmp";

constexpr const uint8_t bmp_header[BMP_HEADER_SIZE] {
    // [2B] type "BM"
    'B',
    'M',

    // [4B] image file size in bytes
    static_cast<uint8_t>(BMP_FILE_SIZE),
    static_cast<uint8_t>(BMP_FILE_SIZE >> 8),
    static_cast<uint8_t>(BMP_FILE_SIZE >> 16),
    static_cast<uint8_t>(BMP_FILE_SIZE >> 24),

    // [4B] reserved
    0,
    0,
    0,
    0,

    // [4B] Offset from beginning of file to the beginning of the bitmap data
    static_cast<uint8_t>(BMP_HEADER_SIZE),
    0,
    0,
    0,

    // [4B] Size of InfoHeader =40
    static_cast<uint8_t>(BMP_INFO_HEADER_SIZE),
    0,
    0,
    0,

    // [4B] Horizontal width of bitmap in pixels
    static_cast<uint8_t>(GuiDefaults::ScreenWidth),
    static_cast<uint8_t>(GuiDefaults::ScreenWidth >> 8),
    0,
    0,

    // [4B] Vertical height of bitmap in pixels
    static_cast<uint8_t>(GuiDefaults::ScreenHeight),
    static_cast<uint8_t>(GuiDefaults::ScreenHeight >> 8),
    0,
    0,

    // [2B] Number of Planes (=1)
    1,
    0,

    // [2B] Bits per Pixel
    static_cast<uint8_t>(bytes_per_pixel * 8),
    0,

    // [4B] Type of Compression | 0 = BI_RGB   no compression
    0,
    0,
    0,
    0,

    // [4B] (compressed) Size of Image
    static_cast<uint8_t>(BMP_IMAGE_DATA_SIZE),
    static_cast<uint8_t>(BMP_IMAGE_DATA_SIZE >> 8),
    static_cast<uint8_t>(BMP_IMAGE_DATA_SIZE >> 16),
    0,

    // [4B] horizontal resolution: Pixels/meter
    0,
    0,
    0,
    0,

    // [4B] vertical resolution: Pixels/meter
    0,
    0,
    0,
    0,

    // [4B] Number of actually used colors.
    0,
    0,
    0,
    0,

    // [4B] Number of important colors  0 = all
    0,
    0,
    0,
    0,
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

    for (int block = GuiDefaults::ScreenHeight / buffer_rows - 1; block >= 0; block--) {
        const point_ui16_t start = point_ui16(0, block * buffer_rows);
        const point_ui16_t end = point_ui16(GuiDefaults::ScreenWidth - 1, (block + 1) * buffer_rows - 1);
        uint8_t *buffer = display::get_block(start, end); // this pointer is valid only until another display memory write is called
        if (!buffer) {
            return false;
        }

        transform_buffer(reinterpret_cast<Pixel *>(buffer + read_start_offset));

        const int write_size = GuiDefaults::ScreenWidth * buffer_rows * bytes_per_pixel;
        if (fwrite(buffer + read_start_offset, 1, write_size, f.get()) != write_size) {
            return false;
        }
    }

    delete_file_guard.disarm();
    return true;
}
