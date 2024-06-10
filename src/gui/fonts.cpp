/**
 * @file fonts.cpp
 */

#include "fonts.hpp"
#include "config.h"
#include <guiconfig/guiconfig.h>
#include <option/enable_translation_ja.h>

#ifdef USE_ST7789
    #if ENABLE_TRANSLATION_JA()
        #include "res/cc/font_regular_7x13_full.hpp" //Font::small
        #include "res/cc/font_regular_11x18_full.hpp" //Font::normal
        #include "res/cc/font_regular_9x16_full.hpp" //Font::special
    #else
        #include "res/cc/font_regular_7x13_standard.hpp" //Font::small
        #include "res/cc/font_regular_11x18_standard.hpp" //Font::normal
        #include "res/cc/font_regular_9x16_standard.hpp" //Font::special
    #endif
#endif
#ifdef USE_ILI9488
    #include "res/cc/font_regular_9x16_full.hpp" //Font::small
    #include "res/cc/font_bold_11x19_full.hpp" //Font::normal
    #include "res/cc/font_bold_13x22_full.hpp" //Font::big
    #include "res/cc/font_bold_30x53_digits.hpp" //Font::large
#endif

typedef struct _resource_entry_t {
    const uint8_t *ptr; // 4 bytes - pointer
    const uint16_t size; // 2 bytes - data size
} resource_entry_t;

#define RESOURCE_ENTRY_FNT(v) { (uint8_t *)&v, sizeof(font_t) },

const resource_entry_t resource_table[] = {
// fonts
#ifdef USE_ST7789
    RESOURCE_ENTRY_FNT(font_regular_7x13) // Font::small
    RESOURCE_ENTRY_FNT(font_regular_11x18) // Font::normal
    RESOURCE_ENTRY_FNT(font_regular_11x18) // Font::big (removed to save flash)
    RESOURCE_ENTRY_FNT(font_regular_9x16) // Font::special
#endif // USE_ST7789

#ifdef USE_ILI9488
    RESOURCE_ENTRY_FNT(font_regular_9x16) // Font::small
    RESOURCE_ENTRY_FNT(font_bold_11x19) // Font::normal
    RESOURCE_ENTRY_FNT(font_bold_13x22) // Font::big
    // Special removed from MK4, using SMALL instead. Differences deemed insignificant
    RESOURCE_ENTRY_FNT(font_regular_9x16) // Font::special
    RESOURCE_ENTRY_FNT(font_bold_30x53) // Font::large
#endif // USE_ILI9488

}; // resource_table

#ifdef USE_ST7789
static_assert(resource_font_size(Font::small) == font_size_t { font_regular_7x13.w, font_regular_7x13.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::normal) == font_size_t { font_regular_11x18.w, font_regular_11x18.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::big) == font_size_t { font_regular_11x18.w, font_regular_11x18.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::special) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
#endif /*USE_ST7789*/

#ifdef USE_ILI9488
static_assert(resource_font_size(Font::small) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::normal) == font_size_t { font_bold_11x19.w, font_bold_11x19.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::big) == font_size_t { font_bold_13x22.w, font_bold_13x22.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::special) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
static_assert(resource_font_size(Font::large) == font_size_t { font_bold_30x53.w, font_bold_30x53.h }, "Font size doesn't match");
#endif /*USE_ILI9488*/

const uint16_t resource_table_size = sizeof(resource_table);
const uint16_t resource_count = sizeof(resource_table) / sizeof(resource_entry_t);

font_t *resource_font(Font font) {
    auto id = static_cast<uint8_t>(font);
    if (id < resource_count) {
        return (font_t *)resource_table[id].ptr;
    }
    return 0;
}
