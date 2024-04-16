#include "gcode_reader_any.hpp"

#include "lang/i18n.h"
#include "transfers/transfer.hpp"
#include <cassert>
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <type_traits>

AnyGcodeFormatReader::~AnyGcodeFormatReader() {
    close();
}

AnyGcodeFormatReader::AnyGcodeFormatReader(AnyGcodeFormatReader &&other) {
    *this = std::move(other);
}

AnyGcodeFormatReader &AnyGcodeFormatReader::operator=(AnyGcodeFormatReader &&other) {
    storage = std::move(other.storage);

    if (std::holds_alternative<PrusaPackGcodeReader>(storage)) {
        ptr = &std::get<PrusaPackGcodeReader>(storage);
    } else if (std::holds_alternative<PlainGcodeReader>(storage)) {
        ptr = &std::get<PlainGcodeReader>(storage);
    } else {
        assert(false);
    }

    other.close();
    return *this;
}

void AnyGcodeFormatReader::close() {
    ptr = nullptr; // Need to be reset first, so it doesn't point to invalid memory
    storage.emplace<std::monostate>();
}

IGcodeReader *AnyGcodeFormatReader::open(const char *filename) {
    transfers::Transfer::Path path(filename);
    struct stat info {};

    // check if file is partially downloaded file
    bool is_partial = false;

    if (stat(path.as_destination(), &info) != 0) {
        return nullptr;
    }

    if (S_ISDIR(info.st_mode)) {
        if (stat(path.as_partial(), &info) != 0) {
            return nullptr;
        }

        is_partial = true;
    }

    FILE *file = fopen(is_partial ? path.as_partial() : path.as_destination(), "rb");
    if (!file) {
        return nullptr;
    }

    if (filename_is_bgcode(filename)) {
        storage.emplace<PrusaPackGcodeReader>(*file, info);
        ptr = &std::get<PrusaPackGcodeReader>(storage);
    }

    else if (filename_is_plain_gcode(filename)) {
        storage.emplace<PlainGcodeReader>(*file, info);
        ptr = &std::get<PlainGcodeReader>(storage);
    }

    else {
        fclose(file);
        return nullptr;
    }

    if (is_partial) {
        ptr->update_validity(path);
    }

    return ptr;
}
