#include "gcode_reader_binary.hpp"

#include "lang/i18n.h"
#include "transfers/transfer.hpp"
#include <cassert>
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <ranges>
#include <type_traits>
#include <config_store/store_instance.hpp>

using bgcode::core::BlockHeader;
using bgcode::core::EBlockType;
using bgcode::core::ECompressionType;
using bgcode::core::EGCodeEncodingType;

PrusaPackGcodeReader::PrusaPackGcodeReader(FILE &f, const struct stat &stat_info)
    : GcodeReaderCommon(f) {
    file_size = stat_info.st_size;
}

IGcodeReader::Result_t PrusaPackGcodeReader::read_and_check_header() {
    if (!range_valid(0, sizeof(file_header))) {
        // Do not set error, the file is not downloaded enough yet
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    rewind(file.get());

    if (bgcode::core::read_header(*file, file_header, nullptr) != bgcode::core::EResult::Success) {
        set_error(N_("Invalid BGCODE file header"));
        return Result_t::RESULT_ERROR;
    }

    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::read_block_header(BlockHeader &block_header, bool check_crc) {
    auto file = this->file.get();
    auto block_start = ftell(file);

    // first need to check if block header is in valid range
    if (!range_valid(block_start, block_start + sizeof(block_header))) {
        return Result_t::RESULT_OUT_OF_RANGE;
    }

    // How large can we afford? Bigger is better, but we need to fit to the current stack
    // (and no, we don't want to have a static buffer allocated all the time).
    constexpr size_t crc_buffer_size = 128;
    uint8_t crc_buffer[crc_buffer_size];
    auto res = read_next_block_header(*file, file_header, block_header, check_crc ? crc_buffer : nullptr, check_crc ? crc_buffer_size : 0);
    if (res == bgcode::core::EResult::ReadError && feof(file)) {
        // END of file reached, end
        return Result_t::RESULT_EOF;

    } else if (res == bgcode::core::EResult::InvalidChecksum) {
        // As a side effect how the partial files work, a checksum verification
        // can read data that were not yet written. In such case, it is very
        // likely going to result in a wrong checksum. But in that case, we
        // want it to result in out of range, so post-processing check to make
        // distinction from really damaged file.

        if (range_valid(block_start, block_start + block_header.get_size() + block_content_size(file_header, block_header))) {
            return Result_t::RESULT_CORRUPT;
        } else {
            return Result_t::RESULT_OUT_OF_RANGE;
        }
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

std::variant<std::monostate, BlockHeader, PrusaPackGcodeReader::Result_t> PrusaPackGcodeReader::iterate_blocks(bool check_crc, stdext::inplace_function<IterateResult_t(BlockHeader &)> function) {
    if (auto res = read_and_check_header(); res != Result_t::RESULT_OK) {
        return res;
    }

    while (true) {
        BlockHeader block_header;
        auto res = read_block_header(block_header, check_crc);
        if (res != Result_t::RESULT_OK) {
            return res;
        }

        // now pass the block to provided funciton, if its the one we are looking for, end now
        switch (function(block_header)) {

        case IterateResult_t::Return:
            return block_header;

        case IterateResult_t::End:
            return std::monostate {};

        case IterateResult_t::Continue:
            break;
        }

        // move to next block header
        if (skip_block(*file, file_header, block_header) != bgcode::core::EResult::Success) {
            // The skip block fails on read errors only.
            return Result_t::RESULT_ERROR;
        }
    }
}

bool PrusaPackGcodeReader::stream_metadata_start() {
    // Will be set accordingly at the end on success
    stream_mode_ = StreamMode::none;

    auto res = iterate_blocks(false, [](BlockHeader &block_header) {
        if (bgcode::core::EBlockType(block_header.type) == bgcode::core::EBlockType::PrinterMetadata) {
            return IterateResult_t::Return;
        }

        return IterateResult_t::Continue;
    });

    if (!std::holds_alternative<BlockHeader>(res)) {
        return false;
    }

    stream.reset();
    stream.current_block_header = get<BlockHeader>(res);

    uint16_t encoding;
    if (fread(&encoding, 1, sizeof(encoding), file.get()) != sizeof(encoding)) {
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
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_block_header.uncompressed_size : stream.current_block_header.compressed_size;
    stream_mode_ = StreamMode::metadata;
    return true;
}

const PrusaPackGcodeReader::StreamRestoreInfo::PrusaPackRec *PrusaPackGcodeReader::get_restore_block_for_offset(uint32_t offset) {
    for (const auto &block : std::ranges::reverse_view(stream_restore_info)) {
        if (block.block_file_pos != 0 && block.block_start_offset <= offset) {
            return &block;
        }
    }

    return nullptr;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_gcode_start(uint32_t offset) {
    BlockHeader start_block;
    uint32_t block_decompressed_offset; //< what is offset of first byte inside block that we start streaming from
    uint32_t block_throwaway_bytes; //< How many bytes to throw away from current block (after decompression)

    // Will be set accordingly at the end on success
    stream_mode_ = StreamMode::none;

    auto file = this->file.get();
    const bool verify = config_store().verify_gcode.get();

    if (offset == 0) {
        // get first gcode block
        auto res = iterate_blocks(verify, [](BlockHeader &block_header) {
            // check if correct type, if so, return this block
            if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
                return IterateResult_t::Return;
            }

            return IterateResult_t::Continue;
        });

        auto header = std::get_if<BlockHeader>(&res);
        if (header == nullptr) {
            if (auto status = std::get_if<Result_t>(&res); status != nullptr) {
                return *status;
            } else {
                // monostate should be returned only when the inner function returns End and we don't do that.
                assert(false);
                return Result_t::RESULT_ERROR;
            }
        }

        start_block = *header;
        block_throwaway_bytes = 0;
        block_decompressed_offset = 0;

    } else {
        // offset > 0 - we are starting from arbitrary offset, find nearest block from cache
        if (auto res = read_and_check_header(); res != Result_t::RESULT_OK) {
            return res; // need to check file header somewhere
        }

        // pick nearest restore block from restore info
        const auto *restore_block = get_restore_block_for_offset(offset);
        if (restore_block == nullptr) {
            return Result_t::RESULT_ERROR;
        }

        if (fseek(file, restore_block->block_file_pos, SEEK_SET) != 0) {
            return Result_t::RESULT_ERROR;
        }

        if (auto res = read_block_header(start_block, /*check_crc=*/verify); res != Result_t::RESULT_OK) {
            return res;
        }

        block_throwaway_bytes = offset - restore_block->block_start_offset;
        block_decompressed_offset = restore_block->block_start_offset;
    }

    stream.reset();
    stream.current_block_header = std::move(start_block);
    if (fread(&stream.encoding, 1, sizeof(stream.encoding), file) != sizeof(stream.encoding)) {
        return Result_t::RESULT_ERROR;
    }

    stream.uncompressed_offset = block_decompressed_offset;
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_block_header.uncompressed_size : stream.current_block_header.compressed_size;
    stream.multiblock = true;
    if (!init_decompression()) {
        return Result_t::RESULT_ERROR;
    }

    stream_restore_info.fill({});
    store_restore_block();

    while (block_throwaway_bytes--) {
        char c;
        if (auto res = stream_getc(c); res != IGcodeReader::Result_t::RESULT_OK) {
            return res;
        }
    }

    stream_mode_ = StreamMode::gcode;
    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::switch_to_next_block() {
    auto file = this->file.get();
    const bool verify = config_store().verify_gcode.get();

    // go to next block
    if (bgcode::core::skip_block(*file, file_header, stream.current_block_header) != bgcode::core::EResult::Success) {
        return Result_t::RESULT_ERROR;
    }

    // read next block
    BlockHeader new_block;
    if (auto res = read_block_header(new_block, /*check_crc=*/verify); res != Result_t::RESULT_OK) {
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
        heatshrink_decoder_reset(stream.hs_decoder.get());
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
    int iout = fgetc(file.get());
    if (iout == EOF) {
        // if fgetc returned EOF, there is something wrong, because that means EOF was found in the middle of block
        return Result_t::RESULT_ERROR;
    }
    out = iout;
    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_current_block_read(char *buffer, size_t size) {
    auto read_res = fread(buffer, 1, size, file.get());
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
        auto poll_res = heatshrink_decoder_poll(stream.hs_decoder.get(), reinterpret_cast<uint8_t *>(&out), sizeof(out), &poll_size);
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
                if (heatshrink_decoder_finish(stream.hs_decoder.get()) != HSD_finish_res::HSDR_FINISH_DONE) {
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

IGcodeReader::Result_t PrusaPackGcodeReader::stream_get_line(GcodeBuffer &buffer, Continuations line_continations) {
    return stream_get_line_common(buffer, line_continations);
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

    const struct params {
        uint16_t expected_width;
        uint16_t expected_height;
        ImgType expected_type;
        bool allow_larger;
    } params {
        .expected_width = expected_width,
        .expected_height = expected_height,
        .expected_type = expected_type,
        .allow_larger = allow_larger,
    };

    auto res = iterate_blocks(false, [this, &params](BlockHeader &block_header) {
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
        if (thumbnail_format_to_type(static_cast<bgcode::core::EThumbnailFormat>(thumb_header.format)) != params.expected_type) {
            return IterateResult_t::Continue;
        }

        if (params.expected_height == thumb_header.height && params.expected_width == thumb_header.width) {
            return IterateResult_t::Return;
        } else if (params.allow_larger && params.expected_height <= thumb_header.height && params.expected_width <= thumb_header.width) {
            return IterateResult_t::Return;
        } else {
            return IterateResult_t::Continue;
        }
    });

    auto header = std::get_if<BlockHeader>(&res);
    if (header == nullptr) {
        stream_mode_ = StreamMode::none;
        return false;
    }

    set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_file);
    stream.reset();
    stream.current_block_header = *header;
    stream.block_remaining_bytes_compressed = header->uncompressed_size; // thumbnail is read as-is, no decompression, so use uncompressed size
    stream_mode_ = StreamMode::thumbnail;
    return true;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size_estimate() {
    auto file = this->file.get();
    long pos = ftell(file); // store file position, so we don't break any running streams

    struct {
        uint32_t blocks_read = 0;
        uint32_t gcode_stream_size_compressed = 0;
        uint32_t gcode_stream_size_uncompressed = 0;
        uint32_t first_gcode_block_pos = 0;
    } stats;

    // estimate works as follows:
    // first NUM_BLOCKS_TO_ESTIMATE are read, compression ratio of those blocks is calculated. Assuming compression ratio is the same for rest of the file, we guess total gcode stream size
    static constexpr unsigned int NUM_BLOCKS_TO_ESTIMATE = 2;
    iterate_blocks(false, [&file, &stats](BlockHeader &block_header) {
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            stats.gcode_stream_size_uncompressed += block_header.uncompressed_size;
            stats.gcode_stream_size_compressed += ((bgcode::core::ECompressionType)block_header.compression == bgcode::core::ECompressionType::None) ? block_header.uncompressed_size : block_header.compressed_size;
            ++stats.blocks_read;
            if (stats.first_gcode_block_pos == 0) {
                stats.first_gcode_block_pos = ftell(file);
            }
        }
        if (stats.blocks_read >= NUM_BLOCKS_TO_ESTIMATE) {
            // after reading NUM_BLOCKS_TO_ESTIMATE blocks, stop
            return IterateResult_t::End;
        }

        return IterateResult_t::Continue;
    });

    float compressionn_ratio = static_cast<float>(stats.gcode_stream_size_compressed) / stats.gcode_stream_size_uncompressed;
    uint32_t compressed_gcode_stream = file_size - stats.first_gcode_block_pos;
    uint32_t uncompressed_file_size = compressed_gcode_stream / compressionn_ratio;

    [[maybe_unused]] auto seek_res = fseek(file, pos, SEEK_SET);
    assert(seek_res == 0);

    return uncompressed_file_size;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size() {
    auto file = this->file.get();
    long pos = ftell(file); // store file position, so we don't break any running streams
    uint32_t gcode_stream_size_uncompressed = 0;

    iterate_blocks(false, [&](BlockHeader &block_header) {
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
        rewind(file.get());
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
        stream.hs_decoder.reset(heatshrink_decoder_alloc(INPUT_BUFFER_SIZE, hs_window_sz, hs_lookahead_sz));
        if (!stream.hs_decoder) {
            return false;
        }

        set_ptr_stream_getc_decompressed(&PrusaPackGcodeReader::stream_getc_decompressed_heatshrink);
    } else {
        // no compression, setup data returning directly from file
        set_ptr_stream_getc_decompressed(&PrusaPackGcodeReader::stream_getc_file);
    }

    const auto encoding = static_cast<EGCodeEncodingType>(stream.encoding);
    if (encoding == EGCodeEncodingType::MeatPack || encoding == EGCodeEncodingType::MeatPackComments) {
        set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_decode_meatpacked);

    } else if (encoding == bgcode::core::EGCodeEncodingType::None) {
        set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_decode_none);

    } else {
        return false;
    }
    return true;
}

bool PrusaPackGcodeReader::valid_for_print() {
    // prusa pack can be printed when we have at least one gcode block
    // all metadata has to be preset at that point, because they are before gcode block
    auto res = iterate_blocks(false, [](BlockHeader &block_header) {
        // check if correct type, if so, return this block
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            return IterateResult_t::Return;
        }

        return IterateResult_t::Continue;
    });

    if (auto err = std::get_if<Result_t>(&res); err != nullptr) {
        switch (*err) {
        case Result_t::RESULT_EOF:
            set_error(N_("File doesn't contain any print instructions"));
            break;
        case Result_t::RESULT_CORRUPT:
            set_error(N_("File corrupt"));
            break;
        case Result_t::RESULT_ERROR:
            set_error(N_("Unknown file error"));
            break;
        default:
            // All the rest (OK, Timeout, out of range) don't prevent this
            // file from being printable in the future, so don't set any
            // error.
            break;
        }
    }

    return std::holds_alternative<BlockHeader>(res);
}

void PrusaPackGcodeReader::stream_t::reset() {
    multiblock = false;
    current_block_header = bgcode::core::BlockHeader();
    encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
    block_remaining_bytes_compressed = 0; //< remaining bytes in current block
    uncompressed_offset = 0; //< offset of next char that will be outputted
    hs_decoder.reset();
    meatpack.reset_state();
}
