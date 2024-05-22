#include <inttypes.h>
#include <cstdlib>

#include "lang.h"

/// main language-table
static const lang_t lang_list[] = {
    { // first item for 'not found'-usage
        LANG_UNDEF,
        "HTTP://HELP.PRUSA3D.COM/",
        "HTTP://INFO.PRUSA3D.COM/",
        "help.prusa3d.com" },
    { LANG_EN,
        "HTTP://HELP.PRUSA3D.COM/",
        "HTTP://INFO.PRUSA3D.COM/",
        "help.prusa3d.com" },
    { LANG_CS,
        "HTTP://HELP.PRUSA3D.CZ/",
        "HTTP://INFO.PRUSA3D.CZ/",
        "help.prusa3d.cz" },
};

static lang_code_t actual_lang = LANG_KLING;

static const size_t LANG_ITEMS = sizeof(lang_list) / sizeof(lang_list[0]);

/// inner function (language-table item finding)
static const lang_t *get_lang_item(lang_code_t lang_code) {
    for (uint32_t i = 1; i < LANG_ITEMS; i++) { // ie. skip first item in language-table
        if (lang_code == lang_list[i].lang_code) {
            return (&lang_list[i]);
        }
    }
    return (&lang_list[0]);
}

/// set actual language
void set_actual_lang(lang_code_t lang_code) {
    actual_lang = lang_code;
}

/// language-table item finding
const lang_t *get_actual_lang(void) {
    return (get_lang_item(actual_lang));
}
