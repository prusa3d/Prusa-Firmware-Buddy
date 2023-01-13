/**
 * @file png_resources.cpp
 */

#include "png_resources.hpp"

static constexpr const char *InternalFlash = "/internal/res/pngs";
bool png::Resource::enabled = false;

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
    return file ? file : (enabled ? getDefaultFile() : nullptr);
}

png::ResourceSingleFile::ResourceSingleFile(const char *name)
    : Resource(name, 0, 0, 0, 0) {
    file = open_file_and_disable_buff(name);

    if (file) {
        uint8_t data[32] { 0 };
        const uint8_t *ptr = data;

        size_t bytes_read = fread(&data[0], 1, 32, file);
        if (bytes_read == 32) {
            //OK
            fseek(file, 0, SEEK_SET); // return to begin
            size_ui16_t sz = icon_size(ptr);
            w = sz.w;
            h = sz.h;
        } else {
            //NOK
            fclose(file);
            file = nullptr; // without this dtor would try to close it again
        }
    }
}

png::ResourceSingleFile::~ResourceSingleFile() {
    if (file) {
        fclose(file);
    }
}
