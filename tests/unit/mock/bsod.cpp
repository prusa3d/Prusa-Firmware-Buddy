#include <stdexcept>
#include "bsod.h"
void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    throw std::runtime_error(fmt);
}
void fatal_error(const char *error, const char *module) {
    throw std::runtime_error(error);
}
