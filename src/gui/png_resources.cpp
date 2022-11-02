/**
 * @file png_resources.cpp
 */

#include "png_resources.hpp"

static constexpr const char *InternalFlash = "/internal/res/pngs";

static FILE *open_file_and_disable_buff(const char *fname) {
    FILE *file = fopen(fname, "rb");
    if (file)
        setvbuf(file, nullptr, _IONBF, 0);
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
