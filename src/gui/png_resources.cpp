/**
 * @file png_resources.cpp
 */

#include "png_resources.hpp"

static constexpr const char *InternalFlash = "/internal/res/pngs";

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
static constexpr size_t SZ = 128; // png draw is about 5% slower compared to 512. Need to save RAM, good enough for MINI.
#else
static constexpr size_t SZ = 512; // optimal value, double speed compared to 0
#endif

static FILE *open_file_and_disable_buff(const char *fname) {
    FILE *file = fopen(fname, "rb");
    if (file)
        setvbuf(file, nullptr, _IOFBF, SZ);
    return file;
}

static FILE *getDefaultFile() {
    static FILE *file = nullptr;
    if (!file)
        file = open_file_and_disable_buff(InternalFlash);
    return file;
}

FILE *png::Resource::Get() const {
    return file ? file : getDefaultFile();
}
