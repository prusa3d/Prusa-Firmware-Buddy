#include "gcode_reader_plaintext.hpp"

#include "lang/i18n.h"
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <type_traits>

PlainGcodeReader::PlainGcodeReader(FILE &f, const struct stat &stat_info)
    : IGcodeReader(f) {
    file_size = stat_info.st_size;
}

bool PlainGcodeReader::stream_metadata_start() {
    bool success = fseek(file.get(), 0, SEEK_SET) == 0;
    stream_mode_ = success ? StreamMode::metadata : StreamMode::none;
    gcodes_in_metadata = 0;
    set_ptr_stream_getc(&PlainGcodeReader::stream_getc_impl);
    return success;
}
bool PlainGcodeReader::stream_gcode_start(uint32_t offset) {
    bool success = fseek(file.get(), offset, SEEK_SET) == 0;
    stream_mode_ = success ? StreamMode::gcode : StreamMode::none;
    set_ptr_stream_getc(&PlainGcodeReader::stream_getc_impl);
    return success;
}
bool PlainGcodeReader::stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger) {
    // search for begining of thumbnail in file
    static const size_t MAX_SEARCH_LINES = 4096;
    // We want to do simple scan through beginning of file, so we use gcode stream for that, it doesn't skip towards end of file like metadata stream
    if (!stream_gcode_start()) {
        return false;
    }

    GcodeBuffer buffer;
    unsigned int lines_searched = 0;
    while (stream_get_line(buffer) == Result_t::RESULT_OK && (lines_searched++) <= MAX_SEARCH_LINES) {
        long unsigned int num_bytes = 0;
        if (IsBeginThumbnail(buffer, expected_width, expected_height, expected_type, allow_larger, num_bytes)) {
            stream_mode_ = StreamMode::thumbnail;
            thumbnail_size = num_bytes;
            base64_decoder.Reset();
            ptr_stream_getc = static_cast<stream_getc_type>(&PlainGcodeReader::stream_getc_thumbnail_impl);
            return true;
        }
    }

    stream_mode_ = StreamMode::none;
    return false;
}

PlainGcodeReader::Result_t PlainGcodeReader::stream_getc_impl(char &out) {
    int iout = fgetc(file.get());
    if (iout == EOF) {
        return Result_t::RESULT_EOF;
    }
    out = iout;
    return Result_t::RESULT_OK;
}
IGcodeReader::Result_t PlainGcodeReader::stream_get_line(GcodeBuffer &buffer) {
    auto pos = ftell(file.get());
    if (!range_valid(pos, pos + 80)) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    // Note: We assume, at least for now, that an incomplete line can happen
    // only in metadata, not in actual gcode.
    const bool previous_incomplete = !buffer.line_complete;

    while (true) {
        // get raw line, then decide if to output it or not
        auto res = IGcodeReader::stream_get_line(buffer);
        if (res != Result_t::RESULT_OK) {
            return res;
        }

        if (previous_incomplete) {
            // This is a continuation of previously incomplete line. That one is already analyzed.

            return Result_t::RESULT_OK;
        }

        // detect if line is metadata (it starts with ;)
        buffer.line.skip_ws();
        if (buffer.line.is_empty()) {
            continue;
        }
        const bool is_metadata = (buffer.line.front() == ';'); // metadata are encoded as comment, this will also pass actual comments that are not medatata, but there is no other way to differentiate between those
        const bool is_gcode = !is_metadata;

        bool output = true;
        if (stream_mode_ == StreamMode::metadata) {
            if (is_gcode) {
                ++gcodes_in_metadata;
            }

            // if we are reading metadata, read first x gcodes, that signals that metadata ends, and after that is done, seek to the end of file and continue streaming there
            // because that is where next interesting metadata is
            if (gcodes_in_metadata == stop_metadata_after_gcodes_num) {
                fseek(file.get(), -search_last_x_bytes, SEEK_END);
                ++gcodes_in_metadata; // to not seek again next time
            }
            output = is_metadata;

        } else if (stream_mode_ == StreamMode::gcode) {
            // if reading gcodes, return everything including metadata, that makes it possible for resume at specified position
            output = true;
        } else {
            return Result_t::RESULT_ERROR;
        }
        if (output) {
            return Result_t::RESULT_OK;
        }
    }
    return Result_t::RESULT_EOF;
}

IGcodeReader::Result_t PlainGcodeReader::stream_getc_thumbnail_impl(char &out) {
    auto file = this->file.get();
    long pos = ftell(file);
    while (true) {
        if (thumbnail_size == 0) {
            return Result_t::RESULT_EOF;
        }
        if (!range_valid(pos, pos + 1)) {
            return Result_t::RESULT_OUT_OF_RANGE;
        }
        int c = fgetc(file);
        if (feof(file)) {
            return Result_t::RESULT_EOF;
        }
        if (ferror(file) && errno == EAGAIN) {
            return Result_t::RESULT_ERROR;
        }
        pos++;

        // skip non-base64 characters
        if (c == '\r' || c == '\n' || c == ' ' || c == ';') {
            continue;
        }
        --thumbnail_size;

        // c now contains valid base64 character - decode and return it
        switch (base64_decoder.ConsumeChar(c, reinterpret_cast<uint8_t *>(&out))) {
        case 1:
            return Result_t::RESULT_OK;
        case 0:
            continue;
        case -1:
            return Result_t::RESULT_ERROR;
        }
    }
}

PlainGcodeReader::Result_t PlainGcodeReader::stream_get_block(char *out_data, size_t &size) {
    if (stream_mode_ != StreamMode::gcode) {
        size = 0;
        return Result_t::RESULT_ERROR;
    }

    auto file = this->file.get();
    long pos = ftell(file);
    if (!range_valid(pos, pos + size)) {
        size = 0;
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    size_t res = fread(out_data, 1, size, file);
    if (res == size) {
        size = res;
        return Result_t::RESULT_OK;
    } else if (feof(file)) {
        size = res;
        return Result_t::RESULT_EOF;
    } else {
        size = 0;
        if (ferror(file) && errno == EAGAIN) {
            return Result_t::RESULT_TIMEOUT;
        }
        return Result_t::RESULT_ERROR;
    }
}

uint32_t PlainGcodeReader::get_gcode_stream_size_estimate() {
    return file_size;
}

uint32_t PlainGcodeReader::get_gcode_stream_size() {
    return file_size;
}

bool PlainGcodeReader::IsBeginThumbnail(GcodeBuffer &buffer, uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larder, unsigned long &num_bytes) const {
    constexpr const char thumbnailBegin_png[] = "; thumbnail begin "; // pozor na tu mezeru na konci
    constexpr const char thumbnailBegin_qoi[] = "; thumbnail_QOI begin "; // pozor na tu mezeru na konci

    const char *thumbnailBegin = nullptr;
    size_t thumbnailBeginSizeof = 0;
    switch (expected_type) {
    case ImgType::PNG:
        thumbnailBegin = thumbnailBegin_png;
        thumbnailBeginSizeof = sizeof(thumbnailBegin_png);
        break;
    case ImgType::QOI:
        thumbnailBegin = thumbnailBegin_qoi;
        thumbnailBeginSizeof = sizeof(thumbnailBegin_qoi);
        break;
    default:
        return false;
    }
    // pokud zacina radka na ; thumbnail, lze se tim zacit zabyvat
    // nemuzu pouzivat zadne pokrocile algoritmy, musim vystacit se strcmp
    const char *lc = buffer.line.begin; // jen quli debuggeru, abych do toho videl...
    // ta -1 na size ma svuj vyznam - chci, aby strncmp NEporovnavalo ten null
    // znak na konci, cili abych se nemusel srat s tim, ze vstupni string je
    // delsi, cili aby to emulovalo chovani boost::starts_with()
    if (!strncmp(lc, thumbnailBegin, thumbnailBeginSizeof - 1)) {
        // zacatek thumbnailu
        unsigned int x, y;
        lc = lc + thumbnailBeginSizeof - 1;
        int ss = sscanf(lc, "%ux%u %lu", &x, &y, &num_bytes);
        if (ss == 3) { // 3 uspesne prectene itemy - rozliseni
            // je to platny zacatek thumbnailu, je to ten muj?
            if ((x == expected_width && y == expected_height) || (allow_larder && x >= expected_width && y >= expected_height)) {
                // je to ten muj, ktery chci
                return true;
            }
        }
    }
    return false;
}

IGcodeReader::FileVerificationResult PlainGcodeReader::verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer) const {
    // If plain gcode starts with bgcode magic, that means it's most like a binary gcode -> it's not a valid plain gcode
    if (check_file_starts_with_BGCODE_magic()) {
        return { .error_str = N_("The file seems to be a binary gcode with a wrong suffix.") };
    }

    // Plain GCode does not have CRC checking
    (void)crc_calc_buffer;

    // No more checks for different levels for now
    (void)level;

    return { .is_ok = true };
}

bool PlainGcodeReader::valid_for_print() {
    // if entire file valid (for short files), or head and tail valid (for long files)
    uint32_t tail_start = (file_size > search_last_x_bytes) ? (file_size - search_last_x_bytes) : 0;
    return range_valid(0, file_size) || (range_valid(0, header_metadata_size) && range_valid(tail_start, file_size));
}
