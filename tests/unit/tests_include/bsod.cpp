#include <catch2/catch_test_macros.hpp>
#include <bsod/bsod.h>
#include <stdexcept>

extern "C" void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    throw std::runtime_error(fmt);
}

extern "C" void fatal_error(const char *error, const char *module) {
    throw std::runtime_error(error);
}
