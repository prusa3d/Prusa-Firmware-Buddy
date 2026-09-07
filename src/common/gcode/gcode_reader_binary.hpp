
#pragma once

#include "core/core.hpp"
#include <option/has_e2ee_support.h>
#include "gcode_buffer.hpp"
#include "gcode_reader_interface.hpp"
#include "meatpack.h"
#include <optional>
#include <utils/byte_utils.hpp>
extern "C" {
#include "heatshrink_decoder.h"
}

#include <inplace_function.hpp>

#if HAS_E2EE_SUPPORT()
    #include <e2ee/e2ee.hpp>
    #include <e2ee/decryptor.hpp>
    #include <e2ee/movable_aes_context.hpp>
    #include <e2ee/identity_check_levels.hpp>
#endif

/**
 * @brief Implementation of IGcodeReader for PrusaPack files
 */
class PrusaPackGcodeReader : public GcodeReaderCommon {
public:
    PrusaPackGcodeReader(unique_file_ptr &&f, const struct stat &stat_info, bool allow_decryption = false
#if HAS_E2EE_SUPPORT()
        ,
        e2ee::IdentityCheckLevel identity_check_lvl = e2ee::IdentityCheckLevel::AnyIdentity
#endif
    );
    PrusaPackGcodeReader(PrusaPackGcodeReader &&other) = default;
    PrusaPackGcodeReader &operator=(PrusaPackGcodeReader &&other) = default;

    virtual void generate_index(Index &out, bool ignore_crc) override;
    virtual bool stream_metadata_start(const Index *index = nullptr) override;
    virtual Result_t stream_gcode_start(uint32_t offset = 0, bool ignore_crc = false, const Index *index = nullptr) override;
    virtual AbstractByteReader *stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) override;
    virtual Result_t stream_get_line(GcodeBuffer &buffer, Continuations) override;
    // Call this only after already decrypting the asymmetric stuff for encrypted gcodes
    virtual uint32_t get_gcode_stream_size_estimate() override;
    virtual uint32_t get_gcode_stream_size() override;

    StreamRestoreInfo get_restore_info() override {
        return { .data = stream_restore_info };
    }
    void set_restore_info(const StreamRestoreInfo &restore_info) override {
        // Don't crash if we provide empty restore info - that simply indicates that we don't have any
        if (const auto *ri = std::get_if<StreamRestoreInfo::PrusaPack>(&restore_info.data)) {
            stream_restore_info = *ri;
        } else {
            stream_restore_info = {};
        }
    }

    virtual bool valid_for_print(bool full_check) override;

private:
    uint32_t file_size; ///< Size of PrusaPack file in bytes
    bgcode::core::FileHeader file_header; // cached header
    bool allow_decryption;
#if HAS_E2EE_SUPPORT()
    e2ee::IdentityCheckLevel identity_check_lvl;
#endif

    struct stream_t {
        stream_t() = default;
        stream_t(stream_t &&) = default;
        stream_t &operator=(stream_t &&) = default;

        bool multiblock = false;
        bgcode::core::BlockHeader current_plain_block_header;
        uint16_t encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
        uint32_t block_remaining_bytes_compressed = 0; //< remaining bytes in current block
        uint32_t uncompressed_offset = 0; //< offset of next char that will be outputted
        MeatPack meatpack;
#if HAS_E2EE_SUPPORT()
        bool last_block = false;
        bgcode::core::BlockHeader current_encrypted_block_header;
        std::unique_ptr<e2ee::Decryptor> decryptor;
#endif

        struct HSDecoderDeleter {
            void operator()(heatshrink_decoder *ptr) {
                heatshrink_decoder_free(ptr);
            }
        };
        std::unique_ptr<heatshrink_decoder, HSDecoderDeleter> hs_decoder;

        void reset();
    } stream;

    StreamRestoreInfo::PrusaPack stream_restore_info; //< Restore info for last two blocks
#if HAS_E2EE_SUPPORT()
    e2ee::IdentityBlockInfo identity_block_info;
    e2ee::SymmetricCipherInfo symmetric_info;
#endif

    /// helper enum for iterate_blocks function
    enum class IterateResult_t {
        Continue, //< continue iterating
        Return, //< return current block
        End, //< end search
    };

    // Returns:
    // * monostate if the provided function returns End
    // * The block header if the function returns Return
    // * An error indication in case of error (including EOF)
    std::variant<std::monostate, bgcode::core::BlockHeader, Result_t> iterate_blocks(bool check_crc, stdext::inplace_function<IterateResult_t(bgcode::core::BlockHeader &)> function);

    /// Pointer to function, that will get decompressed character from file, or data directly form file if not compressed
    stream_getc_type ptr_stream_getc_decompressed = nullptr;

    stream_getc_type ptr_stream_getc_decrypted = nullptr;

    /// get raw character from file, possibly compressed and encoded
    Result_t stream_getc_file(char &out);

    /// get raw characters from current block of file, possibly compressed and encoded
    Result_t stream_current_block_read(char *out, size_t size);

    /// Use heatshrink to decompress characted form current file (might still be encoded)
    Result_t stream_getc_decompressed_heatshrink(char &out);

#if HAS_E2EE_SUPPORT()
    Result_t stream_getc_decrypted(char &out);

    Result_t init_encrypted_block_streaming(const bgcode::core::BlockHeader &block_header);

    void init_decryption();
#endif

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

    void set_ptr_stream_getc_decrypted(IGcodeReader::Result_t (PrusaPackGcodeReader::*ptr_stream_getc_decrypted)(char &out)) {
        // this converts PrusaPackGcodeReader::some_getc_function to IGcodeReader::some_function,
        // note that this conversion is only possible if PrusaPackGcodeReader is subclass of IGcodeReader, and class doesn't have multiple parents
        this->ptr_stream_getc_decrypted = static_cast<stream_getc_type>(ptr_stream_getc_decrypted);
    }

    /**
     * @brief Read block header at current position
     * @note Also checks for file validity and will return RESULT_OUT_OF_RANGE if any part of the block is not valid
     */
    Result_t read_block_header(bgcode::core::BlockHeader &block_header, bool check_crc = false);

    /**
     * @brief Read a block header of given type.
     * @param block_header output
     * @param position_hint if not not_found, read a header at this position.
     * @param search_predicate if position hint not given (eg. not_found), use this predicate to search for the right header.
     */
    Result_t seek_block_header(bgcode::core::BlockHeader &block_header, Index::Position position_hint, bool check_crc, stdext::inplace_function<IterateResult_t(bgcode::core::BlockHeader &)> search_predicate);

    /**
     * @brief Reads file header and check its content (for magic, version etc)
     * @return Status of the header.
     */
    Result_t read_and_check_header();

    struct ThumbnailReader final : public AbstractByteReader {
        FILE *file = nullptr;
        size_t size = 0;

        WritableBytes read(WritableBytes) final;
    };
    ThumbnailReader thumbnail_reader;

    std::optional<ThumbnailDetails> thumbnail_details(const bgcode::core::BlockHeader &block_header);

    /**
     * @brief Is this block of the correct type?
     *
     * This considers the inner encrypted type in case we have encrypted blocks.
     *
     * Defaults to false in case there are errors (eg. corrupt block or block
     * we are unable to decrypt isn't of the correct type).
     */
    bool is_of_type(const bgcode::core::BlockHeader &block_header, bgcode::core::EBlockType type);
    /**
     * @brief Is the printer metadata block readable to us?
     *
     * That is, format we can handle, compression we can handle.
     */
    bool is_readable_metadata(const bgcode::core::BlockHeader &block_header);
};
