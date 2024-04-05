
#pragma once

#include "core/core.hpp"
#include "gcode_buffer.hpp"
#include "gcode_reader_interface.hpp"
#include "meatpack.h"
#include <functional>
#include <optional>
extern "C" {
#include "heatshrink_decoder.h"
}

/**
 * @brief Implementation of IGcodeReader for PrusaPack files
 */
class PrusaPackGcodeReader final : public IGcodeReader {
public:
    PrusaPackGcodeReader(FILE &f, const struct stat &stat_info);
    PrusaPackGcodeReader(PrusaPackGcodeReader &&other) = default;
    PrusaPackGcodeReader &operator=(PrusaPackGcodeReader &&other) = default;

    virtual bool stream_metadata_start() override;
    virtual bool stream_gcode_start(uint32_t offset = 0) override;
    virtual bool stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) override;
    virtual Result_t stream_get_block(char *out_data, size_t &size) override;
    virtual uint32_t get_gcode_stream_size_estimate() override;
    virtual uint32_t get_gcode_stream_size() override;

    virtual FileVerificationResult verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer) const override;

    StreamRestoreInfo get_restore_info() override {
        return { .data = stream_restore_info };
    }
    void set_restore_info(const StreamRestoreInfo &restore_info) override {
        stream_restore_info = std::get<StreamRestoreInfo::PrusaPack>(restore_info.data);
    }

    virtual bool valid_for_print() override;

private:
    uint32_t file_size; ///< Size of PrusaPack file in bytes
    bgcode::core::FileHeader file_header; // cached header

    struct stream_t {
        stream_t() = default;
        stream_t(stream_t &&) = default;
        stream_t &operator=(stream_t &&) = default;

        bool multiblock = false;
        bgcode::core::BlockHeader current_block_header;
        uint16_t encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
        uint32_t block_remaining_bytes_compressed = 0; //< remaining bytes in current block
        uint32_t uncompressed_offset = 0; //< offset of next char that will be outputted
        MeatPack meatpack;

        struct HSDecoderDeleter {
            void operator()(heatshrink_decoder *ptr) {
                heatshrink_decoder_free(ptr);
            }
        };
        std::unique_ptr<heatshrink_decoder, HSDecoderDeleter> hs_decoder;

        void reset();
    } stream;

    StreamRestoreInfo::PrusaPack stream_restore_info; //< Restore info for last two blocks

    /// helper enum for iterate_blocks function
    enum class IterateResult_t {
        Continue, //< continue iterating
        Return, //< return current block
        End, //< end search
    };

    std::optional<bgcode::core::BlockHeader> iterate_blocks(std::function<IterateResult_t(bgcode::core::BlockHeader &)> function);

    /// Pointer to function, that will get decompressed character from file, or data directly form file if not compressed
    stream_getc_type ptr_stream_getc_decompressed = nullptr;

    /// get raw character from file, possibly compressed and encoded
    Result_t stream_getc_file(char &out);

    /// get raw characters from current block of file, possibly compressed and encoded
    Result_t stream_current_block_read(char *out, size_t size);

    /// Use heatshrink to decompress characted form current file (might still be encoded)
    Result_t stream_getc_decompressed_heatshrink(char &out);

    // Decode one character from file, when no encoding is enabled
    Result_t stream_getc_decode_none(char &out);

    // Decode one character from file using meatpack encoding
    Result_t stream_getc_decode_meatpacked(char &out);

    /// switch to next block in file
    Result_t switch_to_next_block();

    /// store current block position in file, for future restoration
    void store_restore_block();

    // initialize decompression depending on parameters in stream
    bool init_decompression();

    // Sink data from current block to headshrink decoder
    Result_t heatshrink_sink_data();

    // find restore info for given offset
    const StreamRestoreInfo::PrusaPackRec *get_restore_block_for_offset(uint32_t offset);

    void set_ptr_stream_getc(IGcodeReader::Result_t (PrusaPackGcodeReader::*ptr_stream_getc)(char &out)) {
        // this converts PrusaPackGcodeReader::some_getc_function to IGcodeReader::some_function,
        // note that this conversion is only possible if PrusaPackGcodeReader is subclass of IGcodeReader, and class doesn't have multiple parents
        this->ptr_stream_getc = static_cast<stream_getc_type>(ptr_stream_getc);
    }

    void set_ptr_stream_getc_decompressed(IGcodeReader::Result_t (PrusaPackGcodeReader::*ptr_stream_getc_decompressed)(char &out)) {
        // this converts PrusaPackGcodeReader::some_getc_function to IGcodeReader::some_function,
        // note that this conversion is only possible if PrusaPackGcodeReader is subclass of IGcodeReader, and class doesn't have multiple parents
        this->ptr_stream_getc_decompressed = static_cast<stream_getc_type>(ptr_stream_getc_decompressed);
    }

    /**
     * @brief Read block header at current position
     * @note Also checks for file validity and will return RESULT_OUT_OF_RANGE if any part of the block is not valid
     */
    Result_t read_block_header(bgcode::core::BlockHeader &block_header);

    /**
     * @brief Reads file header and check its content (for magic, version etc)
     * @return false when header invalid - file shouldn't be used in that case.
     */
    bool read_and_check_header();
};
