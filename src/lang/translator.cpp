#include "translator.hpp"

string_view_utf8 gettext(const char *src) {
    // the most simple implementation - return a string view over the source string
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)src);
}
