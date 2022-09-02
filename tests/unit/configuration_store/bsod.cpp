#include <stdexcept>
#include "bsod.h"
void fatal_error(const char *error, const char *module) {
    throw std::runtime_error(error);
}
