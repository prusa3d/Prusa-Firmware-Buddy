#include "gcode_reader_plaintext.hpp"

#include "lang/i18n.h"
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <cinttypes>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

PlainGcodeReader::PlainGcodeReader(unique_file_ptr &&f, const struct stat &stat_info)
    : GcodeReaderCommon(std::move(f)) {
    this->ptr_stream_getc = static_cast<stream_getc_type>(&PlainGcodeReader::stream_getc_impl);
    file_size = stat_info.st_size;
}

bool PlainGcodeReader::stream_metadata_start([[maybe_unused]] const Index *index) {
    debug_assert(index == nullptr || !index->indexed());
    bool success = fseek(file.get(), 0, SEEK_SET) == 0;
    stream_mode_ = success ? StreamMode::metadata : StreamMode::none;
    gcodes_in_metadata = 0;
    return success;
}

IGcodeReader::Result_t PlainGcodeReader::stream_gcode_start(uint32_t offset, bool ignore_crc, [[maybe_unused]] const Index *index) {
    debug_assert(index == nullptr || !index->indexed());
    // There is no CRC in plaintext G-Code.
    (void)ignore_crc;

    bool success = fseek(file.get(), offset, SEEK_SET) == 0;
    stream_mode_ = success ? StreamMode::gcode : StreamMode::none;
    return success ? Result_t::RESULT_OK : Result_t::RESULT_ERROR;
}
AbstractByteReader *PlainGcodeReader::stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger) {
    // search for begining of thumbnail in file
    static const size_t MAX_SEARCH_LINES = 2048;
    // We want to do simple scan through beginning of file, so we use gcode stream for that, it doesn't skip towards end of file like metadata stream
    if (stream_gcode_start() != IGcodeReader::Result_t::RESULT_OK) {
        return nullptr;
    }

    GcodeBuffer buffer;
    unsigned int lines_searched = 0;
    while (stream_get_line(buffer, Continuations::Discard) == Result_t::RESULT_OK && (lines_searched++) <= MAX_SEARCH_LINES) {
        long unsigned int num_bytes = 0;
        if (IsBeginThumbnail(buffer, expected_width, expected_height, expected_type, allow_larger, num_bytes)) {
            stream_mode_ = StreamMode::thumbnail;
            thumbnail_reader.gcode_reader = this;
            thumbnail_reader.thumbnail_size = num_bytes;
            thumbnail_reader.base64_decoder.reset();
            return &thumbnail_reader;
        }
    }

    stream_mode_ = StreamMode::none;
    return nullptr;
}

PlainGcodeReader::Result_t PlainGcodeReader::stream_getc_impl(char &out) {
    auto pos = ftell(file.get());
    if (!range_valid(pos, pos + 1)) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    int iout = fgetc(file.get());
    if (iout == EOF) {
        return feof(file.get()) ? Result_t::RESULT_EOF : Result_t::RESULT_ERROR;
    }
    out = iout;
    return Result_t::RESULT_OK;
}
IGcodeReader::Result_t PlainGcodeReader::stream_get_line(GcodeBuffer &buffer, Continuations line_continations) {
    auto pos = ftell(file.get());
    if (!range_valid(pos, pos + 80)) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    // Note: We assume, at least for now, that an incomplete line can happen
    // only in metadata, not in actual gcode.
    const bool previous_incomplete = !buffer.line_complete;

    while (true) {
        // get raw line, then decide if to output it or not
        auto res = stream_get_line_common(buffer, line_continations);
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

WritableBytes PlainGcodeReader::ThumbnailReader::read(WritableBytes buffer) {
    // TODO implement reading multiple bytes at a time
    size_t n = buffer.size();
    size_t pos = 0;
    auto data = buffer.data();
    while (n != pos) {
        char c;
        if (getc(c) == IGcodeReader::Result_t::RESULT_OK) {
            data[pos++] = (std::byte)c;
        } else {
            break;
        }
    }
    return { data, pos };
}

IGcodeReader::Result_t PlainGcodeReader::ThumbnailReader::getc(char &out) {
    FILE *file = gcode_reader->file.get();
    long pos = ftell(file);
    while (true) {
        if (thumbnail_size == 0) {
            return Result_t::RESULT_EOF;
        }
        if (!gcode_reader->range_valid(pos, pos + 1)) {
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

        using Result = base64::Base64Decoder::DecodeResult;
        switch (base64_decoder.decode(c, reinterpret_cast<std::byte &>(out))) {

        case Result::error:
            return Result_t::RESULT_ERROR;

        case Result::new_output:
            return Result_t::RESULT_OK;

        case Result::no_output:
            // Continue loop
            break;
        }
    }
}

uint32_t PlainGcodeReader::get_gcode_stream_size_estimate() {
    return file_size;
}

uint32_t PlainGcodeReader::get_gcode_stream_size() {
    return file_size;
}

std::optional<IGcodeReader::ThumbnailDetails> PlainGcodeReader::thumbnail_details(GcodeBuffer::String comment) {
    // The trailing space is present on purpose
    constexpr const char thumbnailBegin_png[] = "; thumbnail begin ";
    constexpr const char thumbnailBegin_qoi[] = "; thumbnail_QOI begin ";
    ThumbnailDetails result;
    if (comment.skip_prefix(thumbnailBegin_png)) {
        result.type = ImgType::PNG;
    } else if (comment.skip_prefix(thumbnailBegin_qoi)) {
        result.type = ImgType::QOI;
    } else {
        return std::nullopt;
    }

    int ss = sscanf(comment.c_str(), "%" SCNu16 "x%" SCNu16 "%lu", &result.width, &result.height, &result.num_bytes);

    if (ss != 3) {
        return std::nullopt;
    }

    return result;
}

bool PlainGcodeReader::IsBeginThumbnail(GcodeBuffer &buffer, uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larder, unsigned long &num_bytes) const {
    const auto details = thumbnail_details(buffer.line);
    if (!details.has_value()) {
        return false;
    }

    if (expected_type != details->type) {
        return false;
    }

    const bool width_matches = (expected_width == details->width || (allow_larder && expected_width < details->width));
    const bool height_matches = (expected_height == details->height || (allow_larder && expected_height < details->height));

    if (!width_matches || !height_matches) {
        return false;
    }

    num_bytes = details->num_bytes;
    return true;
}

bool PlainGcodeReader::valid_for_print([[maybe_unused]] bool full_check) {
    // if entire file valid (for short files), or head and tail valid (for long files)
    uint32_t tail_start = (file_size > search_last_x_bytes) ? (file_size - search_last_x_bytes) : 0;
    return range_valid(0, file_size) || (range_valid(0, header_metadata_size) && range_valid(tail_start, file_size));
}
