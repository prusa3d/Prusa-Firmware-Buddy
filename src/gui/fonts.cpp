/// @file
#include <gui/fonts.hpp>

#include <bsod/bsod.h>
#include <printers.h>

const font_t *resource_font(Font font) {
    switch (font) {
#if PRINTER_IS_PRUSA_MINI()
    case Font::small:
        return &font_regular_7x13;
    case Font::normal:
        return &font_regular_11x18;
    case Font::big:
        return &font_regular_11x18;
    case Font::special:
        return &font_regular_9x16;
#else
    case Font::small:
        return &font_regular_9x16;
    case Font::normal:
        return &font_bold_11x19;
    case Font::big:
        return &font_bold_13x22;
    case Font::special:
        return &font_regular_9x16;
    case Font::large:
        return &font_bold_30x53;
#endif
    }
    bsod_unreachable();
}
