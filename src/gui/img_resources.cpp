#include "img_resources.hpp"

namespace img {

static bool enabled = false;
static FILE *file = nullptr;

#if PRINTER_IS_PRUSA_MINI()
static constexpr size_t SZ = 128; // png draw is about 5% slower compared to 512. Need to save RAM, good enough for MINI.
#else
static constexpr size_t SZ = 512; // optimal value, double speed compared to 0
#endif

static FILE *open_file_and_disable_buff(const char *fname) {
    FILE *file = fopen(fname, "rb");
    if (file) {
        setvbuf(file, nullptr, _IOFBF, SZ);
    }
    return file;
}

void enable_resource_file() {
    enabled = true;
}

FILE *get_resource_file() {
    if (!file && enabled) {
        file = open_file_and_disable_buff("/internal/res/qoi.data");
    }
    return file;
}

img::ResourceSingleFile::ResourceSingleFile(const char *name)
    : Resource(0, 0, 0, 0) {
    file = open_file_and_disable_buff(name);
}

img::ResourceSingleFile::~ResourceSingleFile() {
    if (file) {
        fclose(file);
    }
}

} // namespace img
