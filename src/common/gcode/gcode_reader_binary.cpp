#include "gcode_reader_binary.hpp"
#include <utility_extensions.hpp>
#include <md.h>
#include <crc32.h>
#include <logging/log.hpp>
#include "transfers/transfer.hpp"
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <ranges>
#include <optional>
#include <bsod.h>
#include <type_traits>
#include <config_store/store_instance.hpp>
#include <utils/byte_utils.hpp>

#if HAS_E2EE_SUPPORT()
    #include <e2ee/sha256_multiuse.hpp>
    #include <e2ee/utils.hpp>
    #include <e2ee/hmac.hpp>
    #include <e2ee/key.hpp>
#endif

LOG_COMPONENT_DEF(PRUSA_PACK_READER, logging::Severity::info);

using bgcode::core::BlockHeader;
using bgcode::core::EBlockType;
using bgcode::core::EChecksumType;
using bgcode::core::ECompressionType;
using bgcode::core::EGCodeEncodingType;
using bgcode::core::FileHeader;

PrusaPackGcodeReader::PrusaPackGcodeReader(unique_file_ptr &&f, const struct stat &stat_info, bool allow_decryption
#if HAS_E2EE_SUPPORT()
    ,
    e2ee::IdentityCheckLevel identity_check_lvl
#endif
    )
    : GcodeReaderCommon(std::move(f))
    , allow_decryption(allow_decryption)
#if HAS_E2EE_SUPPORT()
    , identity_check_lvl(identity_check_lvl)
#endif
{
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
#if HAS_E2EE_SUPPORT()
        if (symmetric_info.valid && !stream.last_block) {
            log_info(PRUSA_PACK_READER, "No last block found, cropped bgcode file!");
            return Result_t::RESULT_CORRUPT;
        }
#endif
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

IGcodeReader::Result_t PrusaPackGcodeReader::seek_block_header(BlockHeader &block_header, Index::Position position_hint, bool check_crc, stdext::inplace_function<IterateResult_t(BlockHeader &)> search_predicate) {
    if (position_hint == Index::not_present) {
        return Result_t::RESULT_EOF;
    } else if (position_hint == Index::not_indexed) {
        // get first matching block
        const auto res = iterate_blocks(check_crc, search_predicate);

        return std::visit([&](const auto &res) -> Result_t {
            if constexpr (std::is_same_v<decltype(res), const BlockHeader &>) {
                block_header = res;
                return Result_t::RESULT_OK;
            } else if constexpr (std::is_same_v<decltype(res), const Result_t &>) {
                return res;
            } else if constexpr (std::is_same_v<decltype(res), const std::monostate &>) {
                return Result_t::RESULT_ERROR;
            } else {
                static_assert(false, "Not exhaustive");
            }
        },
            res);
    } else {
        if (fseek(file.get(), position_hint, SEEK_SET) != 0) {
            return Result_t::RESULT_ERROR;
        }

        return read_block_header(block_header, check_crc);
    }
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

bool PrusaPackGcodeReader::stream_metadata_start(const Index *index) {
    // Will be set accordingly at the end on success
    stream_mode_ = StreamMode::none;

    {
        BlockHeader header;

        const auto res = seek_block_header(header, index ? index->metadata : Index::not_indexed, false, [this](BlockHeader &block_header) {
            switch (bgcode::core::EBlockType(block_header.type)) {
            case bgcode::core::EBlockType::PrinterMetadata: {
                if (is_readable_metadata(block_header)) {
                    return IterateResult_t::Return;
                } else {
                    return IterateResult_t::Continue;
                }
            }
            case bgcode::core::EBlockType::GCode:
            case bgcode::core::EBlockType::EncryptedBlock:
                // No chance of finding it past this point.
                return IterateResult_t::End;
            default:
                return IterateResult_t::Continue;
            }
        });

        if (res != Result_t::RESULT_OK) {
            return false;
        }

        stream.reset();
        stream.current_plain_block_header = header;
    }

    // Note: We have already checked the encoding previously (either in
    // building the index or in the closure above). But we need to skip it to
    // start at the actual data.
    uint16_t encoding;
    if (fread(&encoding, sizeof(encoding), 1, file.get()) != 1) {
        return false;
    }

    if (encoding != (uint16_t)bgcode::core::EMetadataEncodingType::INI) {
        return false;
    }

    if (static_cast<ECompressionType>(stream.current_plain_block_header.compression) != ECompressionType::None) {
        return false; // no compression supported on metadata
    }
    // return characters directly from file
    ptr_stream_getc = static_cast<stream_getc_type>(&PrusaPackGcodeReader::stream_getc_file);
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_plain_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_plain_block_header.uncompressed_size : stream.current_plain_block_header.compressed_size;
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

void PrusaPackGcodeReader::generate_index(Index &out, bool ignore_crc) {
    out.gcode = Index::not_present;
    out.metadata = Index::not_present;
    for (auto &thumb : out.thumbnails) {
        thumb.position = Index::not_present;
    }

    auto result = iterate_blocks(!ignore_crc, [&](const BlockHeader &header) -> IterateResult_t {
        switch (static_cast<EBlockType>(header.type)) {
        case EBlockType::GCode:
        case EBlockType::EncryptedBlock:
            out.gcode = header.get_position();
            // All the interesting stuff is before actual gcodes. No reason to index further.
            return IterateResult_t::End;
        case EBlockType::PrinterMetadata: {
            auto position = header.get_position();
            if (is_readable_metadata(header)) {
                // Note: We are not really interested in the other metadata blocks.
                out.metadata = position;
            }
            return IterateResult_t::Continue;
        }
        case EBlockType::Thumbnail: {
            const auto details = thumbnail_details(header);
            if (!details.has_value()) {
                return IterateResult_t::Continue;
            }
            for (auto &thumb : out.thumbnails) {
                if (thumb.w == details->width && thumb.h == details->height && thumb.type == details->type) {
                    thumb.position = header.get_position();
                    break;
                }
            }
            return IterateResult_t::Continue;
        }
        case bgcode::core::EBlockType::FileMetadata:
        case bgcode::core::EBlockType::IdentityBlock:
        case bgcode::core::EBlockType::KeyBlock:
        case bgcode::core::EBlockType::PrintMetadata:
        case bgcode::core::EBlockType::SlicerMetadata:;
        }
        return IterateResult_t::Continue;
    });
    if (!std::holds_alternative<std::monostate>(result)) {
        debug_assert(std::holds_alternative<Result_t>(result));
        out = Index();
    }
}

namespace {
template <typename CB>
void block_header_bytes_cb(BlockHeader header, CB callback) {
    callback(reinterpret_cast<uint8_t *>(&header.type), sizeof(header.type));
    callback(reinterpret_cast<uint8_t *>(&header.compression), sizeof(header.compression));
    callback(reinterpret_cast<uint8_t *>(&header.uncompressed_size), sizeof(header.uncompressed_size));
    if ((ECompressionType)header.compression != ECompressionType::None) {
        callback(reinterpret_cast<uint8_t *>(&header.compressed_size), sizeof(header.compressed_size));
    }
}

} // namespace

IGcodeReader::Result_t PrusaPackGcodeReader::stream_gcode_start(uint32_t offset, bool ignore_crc, const Index *index) {
    BlockHeader start_block;
    uint32_t block_decompressed_offset; //< what is offset of first byte inside block that we start streaming from
    uint32_t block_throwaway_bytes; //< How many bytes to throw away from current block (after decompression)

    // Will be set accordingly at the end on success
    stream_mode_ = StreamMode::none;

    auto file = this->file.get();
    const bool verify = config_store().verify_gcode.get();
    const bool check_crc = verify && !ignore_crc;

    if (offset == 0) {
        const auto res = seek_block_header(start_block, index ? index->gcode : Index::not_indexed, check_crc, [this](BlockHeader &block_header) {
            // check if correct type, if so, return this block
            if (is_of_type(block_header, bgcode::core::EBlockType::GCode)) {
                return IterateResult_t::Return;
            }

            return IterateResult_t::Continue;
        });
        if (res != Result_t::RESULT_OK) {
            return res;
        }

        block_throwaway_bytes = 0;
        block_decompressed_offset = 0;
    } else {
        // Index shall not be used when opening at a specific offset.
        debug_assert(index == nullptr);
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

        if (auto res = read_block_header(start_block, check_crc); res != Result_t::RESULT_OK) {
            return res;
        }

        block_throwaway_bytes = offset - restore_block->block_start_offset;
        block_decompressed_offset = restore_block->block_start_offset;
    }

    stream.reset();
#if HAS_E2EE_SUPPORT()
    if (start_block.type == std::to_underlying(EBlockType::EncryptedBlock)) {
        // This is called only on start and resume (not othat often), so we can afford to read the block
        // once more for the hmac check
        if (auto res = check_hmac_and_crc(file, start_block, symmetric_info, false); res != e2ee::CheckResult::OK) {
            if (res == e2ee::CheckResult::CORRUPTED) {
                return Result_t::RESULT_CORRUPT;
            } else if (res == e2ee::CheckResult::ERROR) {
                return Result_t::RESULT_ERROR;
            }
        }
        init_decryption();
        if (auto res = init_encrypted_block_streaming(start_block); res != Result_t::RESULT_OK) {
            return res;
        }
        set_ptr_stream_getc_decrypted(&PrusaPackGcodeReader::stream_getc_decrypted);
    } else
#endif
    {
        set_ptr_stream_getc_decrypted(&PrusaPackGcodeReader::stream_getc_file);
        stream.current_plain_block_header = std::move(start_block);
        if (fread(&stream.encoding, sizeof(stream.encoding), 1, file) != 1) {
            return Result_t::RESULT_ERROR;
        }
    }

    stream.uncompressed_offset = block_decompressed_offset;
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_plain_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_plain_block_header.uncompressed_size : stream.current_plain_block_header.compressed_size;
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

#if HAS_E2EE_SUPPORT()
    const bool encrypted = symmetric_info.valid;
    BlockHeader &skip_block = encrypted ? stream.current_encrypted_block_header : stream.current_plain_block_header;
#else
    const bool encrypted = false;
    BlockHeader &skip_block = stream.current_plain_block_header;
#endif

    // go to next block
    if (bgcode::core::skip_block(*file, file_header, skip_block) != bgcode::core::EResult::Success) {
        return Result_t::RESULT_ERROR;
    }

    // read next block
    BlockHeader new_block;
    bool check_crc = !encrypted && verify;
    if (auto res = read_block_header(new_block, check_crc); res != Result_t::RESULT_OK) {
        return res;
    }

#if HAS_E2EE_SUPPORT()
    if (stream.last_block) {
        log_info(PRUSA_PACK_READER, "Data found after last block, corrupted!");
        return Result_t::RESULT_CORRUPT;
    }

    if (encrypted) {
        if (auto res = check_hmac_and_crc(file, new_block, symmetric_info, (EChecksumType)file_header.checksum_type == EChecksumType::CRC32 && verify); res != e2ee::CheckResult::OK) {
            if (res == e2ee::CheckResult::CORRUPTED) {
                return Result_t::RESULT_CORRUPT;
            } else if (res == e2ee::CheckResult::ERROR) {
                return Result_t::RESULT_ERROR;
            }
        }
        if (new_block.type != std::to_underlying(EBlockType::EncryptedBlock)) {
            return Result_t::RESULT_ERROR;
        }
        BlockHeader old_plain_block_header = stream.current_plain_block_header;
        uint16_t old_plain_block_encoding = stream.encoding;
        if (auto res = init_encrypted_block_streaming(new_block); res != Result_t::RESULT_OK) {
            return res;
        }
        if (stream.encoding != old_plain_block_encoding || stream.current_plain_block_header.type != old_plain_block_header.type || stream.current_plain_block_header.compression != old_plain_block_header.compression) {
            return Result_t::RESULT_ERROR;
        }
    } else
#endif
    {
        // read encoding
        uint16_t encoding;
        if (fread(&encoding, sizeof(encoding), 1, file) != 1) {
            return Result_t::RESULT_ERROR;
        }

        if (stream.encoding != encoding || stream.current_plain_block_header.type != new_block.type || stream.current_plain_block_header.compression != new_block.compression) {
            return Result_t::RESULT_ERROR;
        }
        stream.current_plain_block_header = new_block;
    }

    // update stream
    stream.block_remaining_bytes_compressed = ((bgcode::core::ECompressionType)stream.current_plain_block_header.compression == bgcode::core::ECompressionType::None) ? stream.current_plain_block_header.uncompressed_size : stream.current_plain_block_header.compressed_size;
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
#if HAS_E2EE_SUPPORT()
    if (symmetric_info.valid) {
        stream_restore_info[1].block_file_pos = stream.current_encrypted_block_header.get_position();
    } else
#endif
    {
        stream_restore_info[1].block_file_pos = stream.current_plain_block_header.get_position();
    }
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
    for (size_t i = 0; i < size; i++) {
        char c;
        auto res = (this->*ptr_stream_getc_decrypted)(c);
        if (res != Result_t::RESULT_OK) {
            return res;
        }
        buffer[i] = c;
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

#if HAS_E2EE_SUPPORT()
GcodeReaderCommon::Result_t PrusaPackGcodeReader::init_encrypted_block_streaming(const bgcode::core::BlockHeader &block_header) {
    stream.current_encrypted_block_header = block_header;
    uint16_t encryption;
    if (fread(&encryption, sizeof(encryption), 1, file.get()) != 1) {
        return Result_t::RESULT_ERROR;
    }
    if (encryption != std::to_underlying(bgcode::core::EEncryptedBlockEncryption::AES128_CBC_SHA256_HMAC)) {
        return Result_t::RESULT_ERROR;
    }
    if (fread(&stream.last_block, sizeof(stream.last_block), 1, file.get()) != 1) {
        return Result_t::RESULT_ERROR;
    }

    // Note: This is the size of the EncryptedBlock, so it is the size of encrypted data + HMACs
    stream.decryptor->setup_block(stream.current_encrypted_block_header.get_position(), block_header.uncompressed_size);
    if (!read_encrypted_block_header(file.get(), stream.current_plain_block_header, *stream.decryptor.get())) {
        return Result_t::RESULT_ERROR;
    }
    if (!stream.decryptor->decrypt(file.get(), reinterpret_cast<uint8_t *>(&stream.encoding), sizeof(stream.encoding))) {
        return Result_t::RESULT_ERROR;
    }

    return Result_t::RESULT_OK;
}

IGcodeReader::Result_t PrusaPackGcodeReader::stream_getc_decrypted(char &out) {
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
    if (!stream.decryptor->decrypt(file.get(), reinterpret_cast<uint8_t *>(&out), sizeof(out))) {
        return Result_t::RESULT_ERROR;
    }

    stream.block_remaining_bytes_compressed--;
    return Result_t::RESULT_OK;
}

void PrusaPackGcodeReader::init_decryption() {
    if (!stream.decryptor) {
        // Only if needed...
        stream.decryptor = std::unique_ptr<e2ee::Decryptor>(new e2ee::Decryptor);
        stream.decryptor->set_cipher_info(symmetric_info);
    }
}
#endif

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

WritableBytes PrusaPackGcodeReader::ThumbnailReader::read(WritableBytes buffer) {
    // thumbnail is read as-is, no decompression
    size_t nread = 0;
    if (size > 0) {
        nread = fread(buffer.data(), 1, std::min(size, buffer.size()), file);
        size -= nread;
    }
    return { buffer.data(), nread };
}

std::optional<PrusaPackGcodeReader::ThumbnailDetails> PrusaPackGcodeReader::thumbnail_details(const BlockHeader &block_header) {
    if ((ECompressionType)block_header.compressed_size != bgcode::core::ECompressionType::None) {
        // no compression supported on images, they are already compressed enough
        return std::nullopt;
    }

    bgcode::core::ThumbnailParams thumb_header;
    if (thumb_header.read(*file) != bgcode::core::EResult::Success) {
        return std::nullopt;
    }

    return ThumbnailDetails {
        .width = thumb_header.width,
        .height = thumb_header.height,
        .num_bytes = block_header.uncompressed_size,
        .type = thumbnail_format_to_type(static_cast<bgcode::core::EThumbnailFormat>(thumb_header.format)),
    };
}

bool PrusaPackGcodeReader::is_readable_metadata(const BlockHeader &header) {
    if ((ECompressionType)header.compressed_size != bgcode::core::ECompressionType::None) {
        // We don't support compressed metadata (at least not now). Ignore this
        // particular metadata block.
        return false;
    }

    uint16_t encoding;
    if (fread(&encoding, sizeof(encoding), 1, file.get()) != 1) {
        return false;
    }

    if (fseek(file.get(), -sizeof(encoding), SEEK_CUR) == -1) {
        return false;
    }

    if (encoding != (uint16_t)bgcode::core::EMetadataEncodingType::INI) {
        // We don't know metadata headers of other formats. They could exist,
        // but we don't know what to do about them - ignore this particular
        // block. Maybe there'll be other block with metadata we can read.
        return false;
    }

    return true;
}

AbstractByteReader *PrusaPackGcodeReader::stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger) {

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

        const auto details = thumbnail_details(block_header);

        if (!details.has_value()) {
            return IterateResult_t::Continue;
        }
        if (details->type != params.expected_type) {
            // Other format than what we want
            return IterateResult_t::Continue;
        }

        if (params.expected_height == details->height && params.expected_width == details->width) {
            return IterateResult_t::Return;
        } else if (params.allow_larger && params.expected_height <= details->height && params.expected_width <= details->width) {
            return IterateResult_t::Return;
        } else {
            return IterateResult_t::Continue;
        }
    });

    auto header = std::get_if<BlockHeader>(&res);
    if (header == nullptr) {
        stream_mode_ = StreamMode::none;
        return nullptr;
    }

    set_ptr_stream_getc(&PrusaPackGcodeReader::stream_getc_file);
    stream.reset();
    stream.current_plain_block_header = *header;
    stream.block_remaining_bytes_compressed = header->uncompressed_size; // thumbnail is read as-is, no decompression, so use uncompressed size
    stream_mode_ = StreamMode::thumbnail;
    thumbnail_reader.file = file.get();
    thumbnail_reader.size = header->uncompressed_size;
    return &thumbnail_reader;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size_estimate() {
    auto file = this->file.get();
    long pos = ftell(file); // store file position, so we don't break any running streams

    // Just so we can capture only this one struct and dont need more space in the
    // inplace function
    struct {
        struct {
            uint32_t blocks_read = 0;
            // Start at one, instead of zero, so that if the reading of the file fails
            // in iterate blocks or in the decryption for any reason (USB disconnect?)
            // it at least will not divide by zero, even if the estimate will be just
            // equal to the size of the compressed data. This is very unlikely, so displaying
            // some really bad estimate, but not crashing is probably ok.
            uint32_t gcode_stream_size_compressed = 1;
            uint32_t gcode_stream_size_uncompressed = 1;
            uint32_t first_gcode_block_pos = 0;
        } stats;
        // Have another decryptor for this, so we dont break any ongoing decryption by wiping cache,
        // iv and size
#if HAS_E2EE_SUPPORT()
        e2ee::Decryptor decryptor;
#endif
    } estimate_context;
#if HAS_E2EE_SUPPORT()
    estimate_context.decryptor.set_cipher_info(symmetric_info);
#endif

    // estimate works as follows:
    // first NUM_BLOCKS_TO_ESTIMATE are read, compression ratio of those blocks is calculated. Assuming compression ratio is the same for rest of the file, we guess total gcode stream size
    static constexpr unsigned int NUM_BLOCKS_TO_ESTIMATE = 2;
    iterate_blocks(false, [&file, &estimate_context](BlockHeader &block_header) {
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            estimate_context.stats.gcode_stream_size_uncompressed += block_header.uncompressed_size;
            estimate_context.stats.gcode_stream_size_compressed += ((bgcode::core::ECompressionType)block_header.compression == bgcode::core::ECompressionType::None) ? block_header.uncompressed_size : block_header.compressed_size;
            ++estimate_context.stats.blocks_read;
            if (estimate_context.stats.first_gcode_block_pos == 0) {
                estimate_context.stats.first_gcode_block_pos = ftell(file);
            }
        }
#if HAS_E2EE_SUPPORT()
        else if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::EncryptedBlock) {
            // seek over encrypted block params
            fseek(file, 3, SEEK_CUR);
            estimate_context.decryptor.setup_block(block_header.get_position(), block_header.uncompressed_size);
            BlockHeader decrypted_gcode_header {};
            read_encrypted_block_header(file, decrypted_gcode_header, estimate_context.decryptor);
            estimate_context.stats.gcode_stream_size_uncompressed += decrypted_gcode_header.uncompressed_size;
            estimate_context.stats.gcode_stream_size_compressed += ((bgcode::core::ECompressionType)decrypted_gcode_header.compression == bgcode::core::ECompressionType::None) ? decrypted_gcode_header.uncompressed_size : decrypted_gcode_header.compressed_size;
            ++estimate_context.stats.blocks_read;
            if (estimate_context.stats.first_gcode_block_pos == 0) {
                estimate_context.stats.first_gcode_block_pos = ftell(file);
            }
        }
#endif
        if (estimate_context.stats.blocks_read >= NUM_BLOCKS_TO_ESTIMATE) {
            // after reading NUM_BLOCKS_TO_ESTIMATE blocks, stop
            return IterateResult_t::End;
        }

        return IterateResult_t::Continue;
    });

    float compressionn_ratio = static_cast<float>(estimate_context.stats.gcode_stream_size_compressed) / estimate_context.stats.gcode_stream_size_uncompressed;
    uint32_t compressed_gcode_stream = file_size - estimate_context.stats.first_gcode_block_pos;
    uint32_t uncompressed_file_size = static_cast<uint32_t>(compressed_gcode_stream / compressionn_ratio);

    [[maybe_unused]] auto seek_res = fseek(file, pos, SEEK_SET);
    debug_assert(seek_res == 0);

    return uncompressed_file_size;
}

uint32_t PrusaPackGcodeReader::get_gcode_stream_size() {
    auto file = this->file.get();
    long pos = ftell(file); // store file position, so we don't break any running streams
    struct {
        uint32_t gcode_stream_size_uncompressed = 0;
#if HAS_E2EE_SUPPORT()
        e2ee::Decryptor decryptor;
#endif

    } size_context;
#if HAS_E2EE_SUPPORT()
    size_context.decryptor.set_cipher_info(symmetric_info);
#endif

    iterate_blocks(false, [&size_context, &file](BlockHeader &block_header) {
        if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::GCode) {
            size_context.gcode_stream_size_uncompressed += block_header.uncompressed_size;
        }
#if HAS_E2EE_SUPPORT()
        else if ((bgcode::core::EBlockType)block_header.type == bgcode::core::EBlockType::EncryptedBlock) {
            // seek over encrypted block params
            fseek(file, 3, SEEK_CUR);
            size_context.decryptor.setup_block(block_header.get_position(), block_header.uncompressed_size);
            BlockHeader decrypted_gcode_header;
            if (!read_encrypted_block_header(file, decrypted_gcode_header, size_context.decryptor)) {
                // This should never happen in practise
                debug_assert(false);
            }
            size_context.gcode_stream_size_uncompressed += decrypted_gcode_header.uncompressed_size;
        }
#endif
        return IterateResult_t::Continue;
    });

    [[maybe_unused]] auto seek_res = fseek(file, pos, SEEK_SET);
    debug_assert(seek_res == 0);

    return size_context.gcode_stream_size_uncompressed;
}

bool PrusaPackGcodeReader::init_decompression() {
    // first setup decompression step
    const ECompressionType compression = static_cast<ECompressionType>(stream.current_plain_block_header.compression);
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
        // no compression, setup data returning from decrypt stream, which might decrypt,
        // or just take it straight from file
        set_ptr_stream_getc_decompressed(ptr_stream_getc_decrypted);
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

#if HAS_E2EE_SUPPORT()
namespace {
// So that we have just one pointer to capture in the in_place_function
// and it fits the storage
class ValidationContext {
public:
    ValidationContext(bool full_check)
        : full_check(full_check) {}
    e2ee::SHA256MultiuseHash hash;
    e2ee::BlockSequenceValidator seq_validator;
    e2ee::PrinterPrivateKey printer_pk;
    bool full_check;
};
} // namespace

bool PrusaPackGcodeReader::valid_for_print(bool full_check) {
    symmetric_info = {};
    identity_block_info = {};
    ValidationContext valid_context(full_check);
    if (full_check) {
        // To have the file_header initialized for the hash
        read_and_check_header();
        file_header_sha256(file_header, valid_context.hash);
    }
    auto res = iterate_blocks(false, [&valid_context, this](BlockHeader &block_header) {
        const auto set_error_end = [this](const char *err) __attribute__((always_inline)) {
            set_error(err);
            return IterateResult_t::End;
        };
        if (e2ee::is_metadata_block((EBlockType)block_header.type)) {

            if (auto err = valid_context.seq_validator.metadata_found(file_header, block_header); err != nullptr) {
                return set_error_end(err);
            }
            if (valid_context.full_check) {
                block_sha_256_update(valid_context.hash, block_header, (EChecksumType)file_header.checksum_type, file.get());
            }
        }
        // prusa pack can be printed when we have at least one gcode block
        // all metadata has to be preset at that point, because they are before gcode block
        // check if correct type, if so, return this block
        if ((EBlockType)block_header.type == EBlockType::GCode) {
            if (auto err = valid_context.seq_validator.gcode_block_found(); err != nullptr) {
                return set_error_end(err);
            } else {
                return IterateResult_t::Return;
            }
        }

        if ((EBlockType)block_header.type == EBlockType::EncryptedBlock) {
            if (auto err = valid_context.seq_validator.encrypted_block_found(block_header); err != nullptr) {
                return set_error_end(err);
            }
            uint8_t key_block_hash[e2ee::HASH_SIZE];
            if (valid_context.full_check) {
                valid_context.hash.get_hash(key_block_hash, sizeof(key_block_hash));
                if (memcmp(key_block_hash, identity_block_info.key_block_hash.data(), sizeof(key_block_hash)) != 0) {
                    return set_error_end(e2ee::key_block_hash_mismatch);
                }
            }
            symmetric_info.num_of_hmacs = valid_context.seq_validator.get_num_of_key_blocks();
            if (!symmetric_info.valid) {
                // TODO Revise the texts, they are shown to the user
                return set_error_end(e2ee::encrypted_for_different_printer);
            } else {
                return IterateResult_t::Return;
            }
        }

        if ((EBlockType)block_header.type == EBlockType::IdentityBlock) {
            if (auto err = valid_context.seq_validator.identity_block_found(file_header, block_header); err != nullptr) {
                return set_error_end(err);
            }
            uint8_t intro_hash[e2ee::HASH_SIZE];
            if (valid_context.full_check) {
                valid_context.hash.get_hash(intro_hash, sizeof(intro_hash));
            }
            if (const char *err = e2ee::read_and_verify_identity_block(file.get(), block_header, valid_context.full_check ? intro_hash : nullptr, identity_block_info, valid_context.full_check); err != nullptr) {
                return set_error_end(err);
            } else {
                std::array<char, e2ee::KEY_HASH_STR_BUFFER_LEN> key_hash;
                e2ee::get_key_hash_string(key_hash.data(), e2ee::KEY_HASH_STR_BUFFER_LEN, identity_block_info.identity_pk.get());
                e2ee::IdentityInfo info({ identity_block_info.identity_name, key_hash, identity_block_info.one_time_identity });
                if (!e2ee::is_trusted_identity(info)) {
                    switch (identity_check_lvl) {
                    case e2ee::IdentityCheckLevel::KnownOnly:
                        // TODO: text??!!
                        return set_error_end("Unknown identity!!");
                    case e2ee::IdentityCheckLevel::Ask:
                        set_identity_info(info);
                        break;
                    case e2ee::IdentityCheckLevel::AnyIdentity:
                        e2ee::save_identity_key_temporary(info);
                        break;
                    }
                }
            }
        }
        if ((EBlockType)block_header.type == EBlockType::KeyBlock) {
            if (auto err = valid_context.seq_validator.key_block_found(file_header, block_header); err != nullptr) {
                return set_error_end(err);
            }
            if (valid_context.full_check) {
                block_header_sha256_update(valid_context.hash, block_header);
            }
            if (auto keys_opt = e2ee::decrypt_key_block(file.get(), block_header, *identity_block_info.identity_pk, valid_context.printer_pk.get_printer_private_key(), valid_context.full_check ? &valid_context.hash : nullptr); keys_opt.has_value()) {
                symmetric_info = keys_opt.value();
                symmetric_info.valid = true;
                symmetric_info.hmac_index = valid_context.seq_validator.get_num_of_key_blocks() - 1;
            }
            if ((EChecksumType)file_header.checksum_type == EChecksumType::CRC32) {
                block_crc_sha256_update(valid_context.hash, file.get());
            }
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
#else
bool PrusaPackGcodeReader::valid_for_print([[maybe_unused]] bool full_check) {
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
#endif

void PrusaPackGcodeReader::stream_t::reset() {
    multiblock = false;
    current_plain_block_header = bgcode::core::BlockHeader();
#if HAS_E2EE_SUPPORT()
    current_encrypted_block_header = bgcode::core::BlockHeader();
    last_block = false;
#endif
    encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
    block_remaining_bytes_compressed = 0; //< remaining bytes in current block
    uncompressed_offset = 0; //< offset of next char that will be outputted
    hs_decoder.reset();
    meatpack.reset_state();
}

bool PrusaPackGcodeReader::is_of_type(const bgcode::core::BlockHeader &block_header, bgcode::core::EBlockType type) {
    if (static_cast<bgcode::core::EBlockType>(block_header.type) == type) {
        return true;
    }

#if HAS_E2EE_SUPPORT()
    if (static_cast<bgcode::core::EBlockType>(block_header.type) == bgcode::core::EBlockType::EncryptedBlock) {
        // It is encrypted, it's worth looking inside.
        // Does init only in case we need it.
        //
        // Note: We do _not_ check the integrity of this block right now and
        // here. If we become interested in it, it'll get checked properly
        // later on.
        auto block_start = ftell(file.get());
        init_decryption();
        if (init_encrypted_block_streaming(block_header) != Result_t::RESULT_OK) {
            return false;
        }
        if (fseek(file.get(), block_start, SEEK_SET) != 0) {
            return false;
        }

        return static_cast<bgcode::core::EBlockType>(stream.current_plain_block_header.type) == type;
    }
#endif

    return false;
}
