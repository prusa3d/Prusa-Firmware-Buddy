/**
 * @file fonts.cpp
 */

#include "fonts.hpp"
#include "config.h"
#ifdef USE_ST7789
    #include "res/cc/font_regular_7x13.hpp" //IDR_FNT_SMALL
    #include "res/cc/font_regular_11x18.hpp" //IDR_FNT_NORMAL
    #include "res/cc/font_regular_9x16.hpp" //IDR_FNT_SPECIAL
#endif // USE_ST7789
#ifdef USE_ILI9488
    #include "res/cc/font_regular_9x16.hpp" //IDR_FNT_SMALL
    #include "res/cc/font_bold_11x19.hpp" //IDR_FNT_NORMAL
    #include "res/cc/font_bold_13x22.hpp" //IDR_FNT_BIG
    #include "res/cc/font_bold_30x53.hpp" //IDR_FNT_LARGE
#endif // USE_ILI9488

typedef struct _resource_entry_t {
    const uint8_t *ptr; // 4 bytes - pointer
    const uint16_t size; // 2 bytes - data size
} resource_entry_t;

#define RESOURCE_ENTRY_FNT(v) { (uint8_t *)&v, sizeof(font_t) },

const resource_entry_t resource_table[] = {
// fonts
#ifdef USE_ST7789
    RESOURCE_ENTRY_FNT(font_regular_7x13) // IDR_FNT_SMALL
    RESOURCE_ENTRY_FNT(font_regular_11x18) // IDR_FNT_NORMAL
    RESOURCE_ENTRY_FNT(font_regular_11x18) // IDR_FNT_BIG (removed to save flash)
    RESOURCE_ENTRY_FNT(font_regular_9x16) // IDR_FNT_SPECIAL
#endif // USE_ST7789

#ifdef USE_ILI9488
    RESOURCE_ENTRY_FNT(font_regular_9x16) // IDR_FNT_SMALL
    RESOURCE_ENTRY_FNT(font_bold_11x19) // IDR_FNT_NORMAL
    RESOURCE_ENTRY_FNT(font_bold_13x22) // IDR_FNT_BIG
    // Special removed from MK4, using SMALL instead. Differences deemed insignificant
    RESOURCE_ENTRY_FNT(font_regular_9x16) // IDR_FNT_SPECIAL
    RESOURCE_ENTRY_FNT(font_bold_30x53) // IDR_FNT_LARGE
#endif // USE_ILI9488

}; // resource_table

#ifdef USE_ST7789
static_assert(resource_font_size(IDR_FNT_SMALL) == font_size_t { font_regular_7x13.w, font_regular_7x13.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_NORMAL) == font_size_t { font_regular_11x18.w, font_regular_11x18.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_BIG) == font_size_t { font_regular_11x18.w, font_regular_11x18.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_SPECIAL) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
#endif /*USE_ST7789*/

#ifdef USE_ILI9488
static_assert(resource_font_size(IDR_FNT_SMALL) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_NORMAL) == font_size_t { font_bold_11x19.w, font_bold_11x19.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_BIG) == font_size_t { font_bold_13x22.w, font_bold_13x22.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_SPECIAL) == font_size_t { font_regular_9x16.w, font_regular_9x16.h }, "Font size doesn't match");
static_assert(resource_font_size(IDR_FNT_LARGE) == font_size_t { font_bold_30x53.w, font_bold_30x53.h }, "Font size doesn't match");
#endif /*USE_ILI9488*/

const uint16_t resource_table_size = sizeof(resource_table);
const uint16_t resource_count = sizeof(resource_table) / sizeof(resource_entry_t);

font_t *resource_font(ResourceId id) {
    if (id < resource_count) {
        return (font_t *)resource_table[id].ptr;
    }
    return 0;
}
