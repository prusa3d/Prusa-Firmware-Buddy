#include <type_traits>
#include <sys/stat.h>
#include <errno.h> // for EAGAIN
#include <cassert>

#include "gcode_reader.hpp"
#include "transfers/transfer.hpp"
#include "lang/i18n.h"
#include <filename_type.hpp>

using bgcode::core::BlockHeader;
using bgcode::core::EBlockType;
using bgcode::core::ECompressionType;
using bgcode::core::EGCodeEncodingType;

IGcodeReader::~IGcodeReader() {
    if (file) {
        fclose(file);
    }
}

IGcodeReader &IGcodeReader::operator=(IGcodeReader &&other) {
    if (file) {
        fclose(file);
    }
    file = other.file;
    other.file = nullptr;
    validity = other.validity;
    return *this;
}

IGcodeReader::Result_t IGcodeReader::stream_get_line(GcodeBuffer &b) {
    b.line.begin = begin(b.buffer);
    b.line.end = begin(b.buffer);

    for (;;) {
        char c;
        Result_t result = stream_getc(c);
        if (result == Result_t::RESULT_EOF) {
            // null terminate => safe atof+atol
            (b.line.end == end(b.buffer)) ? *(b.line.end - 1) = '\0' : *b.line.end = '\0';
            return b.line.is_empty() ? Result_t::RESULT_EOF : Result_t::RESULT_OK;
        }
        if (c == '\r' || c == '\n') {
            if (b.line.is_empty()) {
                continue; // skip blank lines
            } else {
                // null terminate => safe atof+atol
                (b.line.end == end(b.buffer)) ? *(b.line.end - 1) = '\0' : *b.line.end = '\0';
                return Result_t::RESULT_OK;
            }
        }
        if (b.line.end != end(b.buffer)) {
            *b.line.end++ = c;
        }
    }
}

bool IGcodeReader::range_valid(size_t start, size_t end) const {
    assert(start <= end);
    if (start == end) {
        // 0-sized range.
        return true;
    }

    if (!validity.has_value()) {
        // Whole file valid
        return true;
    }

    // Cap them into the range of the file to the range of the total file
    end = std::min(validity->total_size, end);

    auto inside = [&](const auto &part) -> bool {
        if (part.has_value()) {
            return start >= part->start && end <= part->end;
        } else {
            // The part doesn't exist at all, it can't be inside it
            return false;
        }
    };

    return inside(validity->valid_head) || inside(validity->valid_tail);
}

void IGcodeReader::update_validity(transfers::Transfer::Path &filename) {
#if !defined(UNITTESTS) // validity update is disabled for unit tests, because it drags in lots of dependencies
    using transfers::PartialFile;
    using transfers::Transfer;

    const auto transfer_state = Transfer::load_state(filename.as_destination());
    const auto new_validity = std::visit(
        [this](const auto &arg) -> std::optional<PartialFile::State> {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PartialFile::State>) {
                return arg;

            } else if constexpr (std::is_same_v<T, Transfer::Error>) {
                set_error(arg.msg ?: N_("File read error"));
                // State saying "nothing available"
                return PartialFile::State();

            } else if constexpr (std::is_same_v<T, Transfer::Complete>) {
                // Whole file available -> no restrictions on what ranges of files it can access.
                return std::nullopt;
            }
        },
        transfer_state);

    set_validity(new_validity);
#endif
}

bool IGcodeReader::check_file_starts_with_BGCODE_magic() const {
    // Todo respect file availability?
    rewind(file);

    static constexpr int magicSize = bgcode::core::MAGIC.size();
    char check_buffer[magicSize];
    if (!fread(check_buffer, magicSize, 1, file)) {
        return false;
    }

    if (memcmp(check_buffer, bgcode::core::MAGIC.data(), magicSize)) {
        return false;
    }

    return true;
}

PlainGcodeReader::PlainGcodeReader(FILE &f, const struct stat &stat_info)
    : IGcodeReader(f) {
    file_size = stat_info.st_size;
}

bool PlainGcodeReader::stream_metadata_start() {
    bool success = fseek(file, 0, SEEK_SET) == 0;
    output_type = output_type_t::metadata;
    gcodes_in_metadata = 0;
    set_ptr_stream_getc(&PlainGcodeReader::stream_getc_impl);
    return success;
}
bool PlainGcodeReader::stream_gcode_start(uint32_t offset) {
    bool success = fseek(file, offset, SEEK_SET) == 0;
    output_type = output_type_t::gcode;
    set_ptr_stream_getc(&PlainGcodeReader::stream_getc_impl);
    return success;
}
bool PlainGcodeReader::stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger) {
    // search for begining of thumbnail in file
    static const size_t MAX_SEARCH_LINES = 2048;
    // We want to do simple scan through beginning of file, so we use gcode stream for that, it doesn't skip towards end of file like metadata stream
    stream_gcode_start();
    GcodeBuffer buffer;
    unsigned int lines_searched = 0;
    while (stream_get_line(buffer) == Result_t::RESULT_OK && (lines_searched++) <= MAX_SEARCH_LINES) {
        long unsigned int num_bytes = 0;
        if (IsBeginThumbnail(buffer, expected_width, expected_height, expected_type, allow_larger, num_bytes)) {
            output_type = output_type_t::thumbnail;
            thumbnail_size = num_bytes;
            base64_decoder.Reset();
            ptr_stream_getc = static_cast<stream_getc_type>(&PlainGcodeReader::stream_getc_thumbnail_impl);
            return true;
        }
    }
    return false;
}

PlainGcodeReader::Result_t PlainGcodeReader::stream_getc_impl(char &out) {
    int iout = fgetc(file);
    if (iout == EOF) {
        return Result_t::RESULT_EOF;
    }
    out = iout;
    return Result_t::RESULT_OK;
}
IGcodeReader::Result_t PlainGcodeReader::stream_get_line(GcodeBuffer &buffer) {
    auto pos = ftell(file);
    if (!range_valid(pos, pos + 80)) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    while (true) {
        // get raw line, then decide if to output it or not
        auto res = IGcodeReader::stream_get_line(buffer);
        if (res != Result_t::RESULT_OK) {
            return res;
        }

        // detect if line is metadata (it starts with ;)
        buffer.line.skip_ws();
        if (buffer.line.is_empty()) {
            continue;
        }
        const bool is_metadata = (buffer.line.front() == ';'); // metadata are encoded as comment, this will also pass actual comments that are not medatata, but there is no other way to differentiate between those
        const bool is_gcode = !is_metadata;

        bool output = true;
        if (output_type == output_type_t::metadata) {
            if (is_gcode) {
                ++gcodes_in_metadata;
            }

            // if we are reading metadata, read first x gcodes, that signals that metadata ends, and after that is done, seek to the end of file and continue streaming there
            // because that is where next interesting metadata is
            if (gcodes_in_metadata == stop_metadata_after_gcodes_num) {
                fseek(file, -search_last_x_bytes, SEEK_END);
                ++gcodes_in_metadata; // to not seek again next time
            }
            output = is_metadata;

        } else if (output_type == output_type_t::gcode) {
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
    if (output_type != output_type_t::gcode) {
        size = 0;
        return Result_t::RESULT_ERROR;
    }

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

PrusaPackGcodeReader::PrusaPackGcodeReader(FILE &f, const struct stat &stat_info)
    : IGcodeReader(f) {
    file_size = stat_info.st_size;
}

bool PrusaPackGcodeReader::read_and_check_header() {
    if (!range_valid(0, sizeof(file_header))) {
        // Do not set error, the file is not downloaded enough yet
        return false;
    }

    rewind(file);

    if (bgcode::core::read_header(*file, file_header, nullptr) != bgcode::core::EResult::Success) {
        set_error(N_("Invalid BGCODE file header"));
        return false;
    }

    return true;
}

IGcodeReader::Result_t PrusaPackGcodeReader::read_block_header(BlockHeader &block_header) {
    auto block_start = ftell(file);

    // first need to check if block header is in valid range
    if (!range_valid(block_start, block_start + sizeof(block_header))) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    auto res = read_next_block_header(*file, file_header, block_header);
    if (res == bgcode::core::EResult::ReadError && feof(file)) {
        // END of file reached, end
        return Result_t::RESULT_EOF;

    } else if (res != bgcode::core::EResult::Success) {
        // some read error
        return Result_t::RESULT_ERROR;
    }

    // now check if also block content is in valid range
    if (!range_valid(block_start, block_start + block_header.get_size() + block_content_size(file_header, block_header))) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    return Result_t::RESULT_OK;
}

std::optional<BlockHeader> PrusaPackGcodeReader::iterate_blocks(std::function<IterateResult_t(BlockHeader &)> function) {
    if (!read_and_check_header()) {
        return std::nullopt;
    }

    while (true) {
        BlockHeader block_header;
        auto res = read_block_header(block_header);
        if (res != Result_t::RESULT_OK) {
            return std::nullopt;
        }

        // now pass the block to provided funciton, if its the one we are looking for, end now
        switch (function(block_header)) {

        case IterateResult_t::Return:
            return block_header;
            break;

        case IterateResult_t::End:
            return std::nullopt;
            break;

        case IterateResult_t::Continue:
            break;
        }

        // move to next block header
        if (skip_block(*file, file_header, block_header) != bgcode::core::EResult::Success) {
            return std::nullopt;
        }
    }
}

bool PrusaPackGcodeReader::stream_metadata_start() {
    auto res = iterate_blocks([](BlockHeader &block_header) {
        if (bgcode::core::EBlockType(block_header.type) == bgcode::core::EBlockType::PrinterMetadata) {
            return IterateResult_t::Return;
        }

        return IterateResult_t::Continue;
    });

    if (!res.has_value()) {
        return false;
    }

    stream.reset();
    stream.current_block_header = res.value();

    uint16_t encoding;
    if (fread(&encoding, 1, sizeof(encoding), file) != sizeof(encoding)) {
        return false;
    }

    if (encoding != (uint16_t)bgcode::core::EMetadataEncodingType::INI) {
        return false;
    }

    if (static_cast<ECompressionType>(stream.current_block_header.compression) != ECompressionType::None) {
        return false; // no compression supported on metadata
    }
    // return characters directly from file
    ptr_stream_getc = static_cast<stream_getc_type>(&PrusaPackGcodeReader::stream_getc_file);
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_block_header.compression == bgcode::core::ECompressionType::None) ? res->uncompressed_size : res->compressed_size;
    return true;
}

PrusaPackGcodeReader::stream_restore_info_rec_t *PrusaPackGcodeReader::get_restore_block_for_offset(uint32_t offset) {
    static_assert((sizeof(stream_restore_info) / sizeof(stream_restore_info[0])) == 2); // this is written for two blocks
    if (stream_restore_info[1].block_file_pos != 0 && stream_restore_info[1].block_start_offset <= offset) {
        return &stream_restore_info[1];
    } else if (stream_restore_info[0].block_file_pos != 0 && stream_restore_info[0].block_start_offset <= offset) {
        return &stream_restore_info[0];
    }
    return nullptr;
}

bool PrusaPackGcodeReader::stream_gcode_start(uint32_t offset) {
    BlockHeader start_block;
    uint32_t block_decompressed_offset; //< what is offset of first byte inside block that we start streaming from
    uint32_t block_throwaway_bytes; //< How many bytes to throw away from current block (after decompression)

    if (offset == 0) {
        // get first gcode block
        auto res = iterate_blocks([](BlockHeader &block_header) {
            // check if correct type, if so, return this block
            if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
                return IterateResult_t::Return;
            }

            return IterateResult_t::Continue;
        });
        if (!res.has_value()) {
            return false;
        }

        start_block = res.value();
        block_throwaway_bytes = 0;
        block_decompressed_offset = 0;

    } else {
        // offset > 0 - we are starting from arbitrary offset, find nearest block from cache
        if (!read_and_check_header()) {
            return false; // need to check file header somewhere
        }

        // pick nearest restore block from restore info
        stream_restore_info_rec_t *restore_block = get_restore_block_for_offset(offset);
        if (restore_block == nullptr) {
            return false;
        }

        if (fseek(file, restore_block->block_file_pos, SEEK_SET) != 0) {
            return false;
        }

        if (auto res = read_block_header(start_block); res != Result_t::RESULT_OK) {
            return false;
        }

        block_throwaway_bytes = offset - restore_block->block_start_offset;
        block_decompressed_offset = restore_block->block_start_offset;
    }

    stream.reset();
    stream.current_block_header = std::move(start_block);
    if (fread(&stream.encoding, 1, sizeof(stream.encoding), file) != sizeof(stream.encoding)) {
        return false;
    }

    stream.uncompressed_offset = block_decompressed_offset;
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_block_header.uncompressed_size : stream.current_block_header.compressed_size;
    stream.multiblock = true;
    if (!init_decompression()) {
        return false;
    }

    stream_restore_info.fill(stream_restore_info_rec_t());
    store_restore_block();

    while (block_throwaway_bytes--) {
        char c;
        if (stream_getc(c) != IGcodeReader::Result_t::RESULT_OK) {
            return false;
        }
    }

    return true;
}

PlainGcodeReader::Result_t PrusaPackGcodeReader::switch_to_next_block() {
    // go to next block
    if (bgcode::core::skip_block(*file, file_header, stream.current_block_header) != bgcode::core::EResult::Success) {
        return Result_t::RESULT_ERROR;
    }

    // read next block
    BlockHeader new_block;
    if (auto res = read_block_header(new_block); res != Result_t::RESULT_OK) {
        return res;
    }

    // read encoding
    uint16_t encoding;
    if (fread(&encoding, 1, sizeof(encoding), file) != sizeof(encoding)) {
        return Result_t::RESULT_ERROR;
    }

    if (stream.encoding != encoding || stream.current_block_header.type != new_block.type || stream.current_block_header.compression != new_block.compression) {
        return Result_t::RESULT_ERROR;
    }

    // update stream
    stream.current_block_header = new_block;
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_block_header.uncompressed_size : stream.current_block_header.compressed_size;
    stream.meatpack.reset_state();
    if (stream.hs_decoder) {
        heatshrink_decoder_reset(stream.hs_decoder);
    }
    store_restore_block();
    return Result_t::RESULT_OK;
}

void PrusaPackGcodeReader::store_restore_block() {
    // shift away oldest restore info
    stream_restore_info[0] = stream_restore_info[1];
    // and store new restore info
    stream_restore_info[1].block_file_pos = stream.current_block_header.get_position();
    stream_restore_info[1].block_start_offset = stream.uncompressed_offset;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_getc_file(char &out) {
    if (stream.block_remaining_bytes_compressed == 0) {
        if (stream.multiblock) {
            auto res = switch_to_next_block();
            if (res != Result_t::RESULT_OK) {
                return res;
            }
        } else {
            return Result_t::RESULT_EOF;
        }
    }
    stream.block_remaining_bytes_compressed--; // assume 1 byte was read, it might return EOF/ERROR, but at this point it doesn't matter as stream is done anyway
    int iout = fgetc(file);
    if (iout == EOF) {
        // if fgetc returned EOF, there is something wrong, because that means EOF was found in the middle of block
        return Result_t::RESULT_ERROR;
    }
    out = iout;
    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_current_block_read(char *buffer, size_t size) {
    auto read_res = fread(buffer, 1, size, file);
    stream.block_remaining_bytes_compressed -= size;
    if (read_res != size) {
        return IGcodeReader::Result_t::RESULT_ERROR;
    }
    return IGcodeReader::Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::heatshrink_sink_data() {
    // this is alternative to heatshrink_decoder_sink, but this implementation reads reads data directly to heatshrink input buffer.
    // This way we avoid allocating extra buffer for reading and extra copy of data.

    // size to sink - space of sink buffer of decoder, or remaining bytes in current block
    uint32_t to_sink = std::min(static_cast<uint32_t>(HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(stream.hs_decoder) - stream.hs_decoder->input_size),
        stream.block_remaining_bytes_compressed);

    // where to sink data
    char *sink_ptr = reinterpret_cast<char *>(&stream.hs_decoder->buffers[stream.hs_decoder->input_size]);

    if (to_sink == 0) {
        return Result_t::RESULT_ERROR;
    }

    if (stream_current_block_read(sink_ptr, to_sink) != Result_t::RESULT_OK) {
        return Result_t::RESULT_ERROR;
    }

    stream.hs_decoder->input_size += to_sink;

    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_getc_decompressed_heatshrink(char &out) {
    while (true) {
        size_t poll_size;
        auto poll_res = heatshrink_decoder_poll(stream.hs_decoder, reinterpret_cast<uint8_t *>(&out), sizeof(out), &poll_size);
        if (poll_res == HSDR_POLL_ERROR_NULL || poll_res == HSDR_POLL_ERROR_UNKNOWN) {
            return IGcodeReader::Result_t::RESULT_ERROR;
        }
        // we have our byte, return it
        if (poll_size == sizeof(out)) {
            return IGcodeReader::Result_t::RESULT_OK;
        }

        // byte not yet available, need to sink more data
        if (poll_res == HSDR_POLL_EMPTY) {
            // switch to next block, if needed first
            if (stream.block_remaining_bytes_compressed == 0) {
                // all data should be polled by now, if there are some data left in decompressor, something is wrong
                if (heatshrink_decoder_finish(stream.hs_decoder) != HSD_finish_res::HSDR_FINISH_DONE) {
                    return IGcodeReader::Result_t::RESULT_ERROR;
                }
                if (stream.multiblock) {
                    auto res = switch_to_next_block();
                    if (res != Result_t::RESULT_OK) {
                        return res;
                    }
                } else {
                    return Result_t::RESULT_EOF;
                }
            }

            auto sink_res = heatshrink_sink_data();
            if (sink_res != Result_t::RESULT_OK) {
                return sink_res;
            }
        }
    }
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_getc_decode_meatpacked(char &out) {
    while (true) {
        if (stream.meatpack.has_result_char()) {
            // character is ready, return it
            out = stream.meatpack.get_result_char();
            ++stream.uncompressed_offset;
            return IGcodeReader::Result_t::RESULT_OK;
        }

        // no character, uncompress next character
        char mp_char;
        auto res = (this->*ptr_stream_getc_decompressed)(mp_char);
        if (res != Result_t::RESULT_OK) {
            return res;
        }

        stream.meatpack.handle_rx_char(mp_char);
    }
}
IGcodeReader::Result_t PrusaPackGcodeReader::stream_getc_decode_none(char &out) {
    auto res = (this->*ptr_stream_getc_decompressed)(out);
    if (res == Result_t::RESULT_OK) {
        ++stream.uncompressed_offset;
    }

    return res;
}

constexpr PrusaPackGcodeReader::ImgType thumbnail_format_to_type(bgcode::core::EThumbnailFormat type) {
    switch (type) {
    case bgcode::core::EThumbnailFormat::PNG:
        return PrusaPackGcodeReader::ImgType::PNG;
    case bgcode::core::EThumbnailFormat::QOI:
        return PrusaPackGcodeReader::ImgType::QOI;
    default:
        return PrusaPackGcodeReader::ImgType::Unknown;
    }
}

bool PrusaPackGcodeReader::stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger) {

    auto res = iterate_blocks([&](BlockHeader &block_header) {
        if ((EBlockType)block_header.type == EBlockType::GCode) {
            // if gcode block was found, we can end search, Thumbnail is supposed to be before gcode block
            return IterateResult_t::End;
        }

        if ((EBlockType)block_header.type != EBlockType::Thumbnail) {
            return IterateResult_t::Continue;
        }
        if ((ECompressionType)block_header.compressed_size != bgcode::core::ECompressionType::None) {
            // no compression supported on images, they are already compressed enough
            return IterateResult_t::Continue;
        }

        bgcode::core::ThumbnailParams thumb_header;
        if (thumb_header.read(*file) != bgcode::core::EResult::Success) {
            return IterateResult_t::Continue;
        }

        // format not valid
        if (thumbnail_format_to_type(static_cast<bgcode::core::EThumbnailFormat>(thumb_header.format)) != expected_type) {
            return IterateResult_t::Continue;
        }

        if (expected_height == thumb_header.height && expected_width == thumb_header.width) {
            return IterateResult_t::Return;
        } else if (allow_larger && expected_height <= thumb_header.height && expected_width <= thumb_header.width) {
            return IterateResult_t::Return;
        } else {
            return IterateResult_t::Continue;
        }
    });

    if (res.has_value()) {
        set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_file);
        stream.reset();
        stream.current_block_header = res.value();
        stream.block_remaining_bytes_compressed = res->uncompressed_size; // thumbnail is read as-is, no decompression, so use uncompressed size
        return true;
    }
    return false;
}

PrusaPackGcodeReader::Result_t PrusaPackGcodeReader::stream_get_block(char *out_data, size_t &size) {
    auto orig_size = size;
    size = 0;
    while (size != orig_size) {
        auto res = stream_getc(*(out_data++));
        if (res != IGcodeReader::Result_t::RESULT_OK) {
            return res;
        }
        ++size;
    }
    return Result_t::RESULT_OK;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size_estimate() {
    long pos = ftell(file); // store file position, so we don't break any running streams
    uint32_t blocks_read = 0;
    uint32_t gcode_stream_size_compressed = 0;
    uint32_t gcode_stream_size_uncompressed = 0;
    uint32_t first_gcode_block_pos = 0;

    // estimate works as follows:
    // first NUM_BLOCKS_TO_ESTIMATE are read, compression ratio of those blocks is calculated. Assuming compression ratio is the same for rest of the file, we guess total gcode stream size
    static constexpr unsigned int NUM_BLOCKS_TO_ESTIMATE = 2;
    iterate_blocks([&](BlockHeader &block_header) {
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            gcode_stream_size_uncompressed += block_header.uncompressed_size;
            gcode_stream_size_compressed += ((bgcode::core::ECompressionType)block_header.compression == bgcode::core::ECompressionType::None) ? block_header.uncompressed_size : block_header.compressed_size;
            ++blocks_read;
            if (first_gcode_block_pos == 0) {
                first_gcode_block_pos = ftell(file);
            }
        }
        if (blocks_read >= NUM_BLOCKS_TO_ESTIMATE) {
            // after reading NUM_BLOCKS_TO_ESTIMATE blocks, stop
            return IterateResult_t::End;
        }

        return IterateResult_t::Continue;
    });

    float compressionn_ratio = static_cast<float>(gcode_stream_size_compressed) / gcode_stream_size_uncompressed;
    uint32_t compressed_gcode_stream = file_size - first_gcode_block_pos;
    uint32_t uncompressed_file_size = compressed_gcode_stream / compressionn_ratio;

    [[maybe_unused]] auto seek_res = fseek(file, pos, SEEK_SET);
    assert(seek_res == 0);

    return uncompressed_file_size;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size() {
    long pos = ftell(file); // store file position, so we don't break any running streams
    uint32_t gcode_stream_size_uncompressed = 0;

    iterate_blocks([&](BlockHeader &block_header) {
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            gcode_stream_size_uncompressed += block_header.uncompressed_size;
        }
        return IterateResult_t::Continue;
    });

    [[maybe_unused]] auto seek_res = fseek(file, pos, SEEK_SET);
    assert(seek_res == 0);

    return gcode_stream_size_uncompressed;
}

IGcodeReader::FileVerificationResult PrusaPackGcodeReader::verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer) const {
    // Every binary gcode has to start a magic sequence
    if (!check_file_starts_with_BGCODE_magic()) {
        return { .error_str = N_("The file is not a valid bgcode file.") };
    }

    // Further checks are for FileVerificationLevel::full
    if (int(level) < int(FileVerificationLevel::full)) {
        return { .is_ok = true };
    }

    // Check CRC
    {
        // todo: this doesn't respect file validity
        rewind(file);
        if (bgcode::core::is_valid_binary_gcode(*file, true, crc_calc_buffer.data(), crc_calc_buffer.size()) != bgcode::core::EResult::Success) {
            return { .error_str = N_("The file is not a valid bgcode file.") };
        }
    }

    return { .is_ok = true };
}

bool PrusaPackGcodeReader::init_decompression() {
    // first setup decompression step
    const ECompressionType compression = static_cast<ECompressionType>(stream.current_block_header.compression);
    uint8_t hs_window_sz = 0;
    uint8_t hs_lookahead_sz = 0;
    if (compression == ECompressionType::None) {
        // no compression, dont init decompressor
    } else if (compression == ECompressionType::Heatshrink_11_4) {
        hs_window_sz = 11;
        hs_lookahead_sz = 4;
    } else if (compression == ECompressionType::Heatshrink_12_4) {
        hs_window_sz = 12;
        hs_lookahead_sz = 4;
    } else {
        return false;
    }

    if (hs_window_sz) {
        // compression enabled, setup heatshrink
        static constexpr size_t INPUT_BUFFER_SIZE = 64;
        stream.hs_decoder = heatshrink_decoder_alloc(INPUT_BUFFER_SIZE, hs_window_sz, hs_lookahead_sz);
        if (stream.hs_decoder == nullptr) {
            return false;
        }

        set_ptr_stream_getc_decompressed(&PrusaPackGcodeReader::stream_getc_decompressed_heatshrink);
    } else {
        // no compression, setup data returning directly from file
        set_ptr_stream_getc_decompressed(&PrusaPackGcodeReader::stream_getc_file);
    }

    if (static_cast<EGCodeEncodingType>(stream.encoding) == EGCodeEncodingType::MeatPack || static_cast<EGCodeEncodingType>(stream.encoding) == EGCodeEncodingType::MeatPackComments) {
        set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_decode_meatpacked);
    } else if (static_cast<EGCodeEncodingType>(stream.encoding) == bgcode::core::EGCodeEncodingType::None) {
        set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_decode_none);
    } else {
        return false;
    }
    return true;
}

bool PrusaPackGcodeReader::valid_for_print() {
    // prusa pack can be printed when we have at least one gcode block
    // all metadata has to be preset at that point, because they are before gcode block
    auto res = iterate_blocks([](BlockHeader &block_header) {
        // check if correct type, if so, return this block
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            return IterateResult_t::Return;
        }

        return IterateResult_t::Continue;
    });

    return res.has_value();
}

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

    other.ptr = nullptr;
    return *this;
}

void AnyGcodeFormatReader::close() {
    ptr = nullptr; // Need to be reset first, so it doesn't point to invalid memory
    storage.emplace<std::monostate>();
}

IGcodeReader *AnyGcodeFormatReader::open(const char *filename) {
    FILE *file = nullptr;
    bool is_partial = false;
    struct stat info {};

    // check if file is partially downloaded file
    transfers::Transfer::Path path(filename);
    if (stat(path.as_destination(), &info) != 0) {
        return nullptr;
    } else {
        if (S_ISDIR(info.st_mode)) {
            if (stat(path.as_partial(), &info) != 0) {
                return nullptr;
            }
            is_partial = true;
        }
    }

    file = fopen(is_partial ? path.as_partial() : path.as_destination(), "rb");
    if (file) {
        if (filename_is_bgcode(filename)) {
            storage.emplace<PrusaPackGcodeReader>(*file, info);
            ptr = &std::get<PrusaPackGcodeReader>(storage);

            if (is_partial) {
                ptr->update_validity(path);
            }

            return ptr;

        } else if (filename_is_plain_gcode(filename)) {
            storage.emplace<PlainGcodeReader>(*file, info);
            ptr = &std::get<PlainGcodeReader>(storage);

            if (is_partial) {
                ptr->update_validity(path);
            }

            return ptr;
        }
    }
    if (file) {
        fclose(file);
    }

    return nullptr;
}

void PrusaPackGcodeReader::stream_t::reset() {
    multiblock = false;
    current_block_header = bgcode::core::BlockHeader();
    encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
    block_remaining_bytes_compressed = 0; //< remaining bytes in current block
    uncompressed_offset = 0; //< offset of next char that will be outputted
    if (hs_decoder) {
        heatshrink_decoder_free(hs_decoder);
        hs_decoder = nullptr;
    }
    meatpack.reset_state();
}

PrusaPackGcodeReader::stream_t::~stream_t() {
    if (hs_decoder) {
        heatshrink_decoder_free(hs_decoder);
    }
    hs_decoder = nullptr;
}
