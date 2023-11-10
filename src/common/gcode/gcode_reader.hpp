
#pragma once
#include "gcode_buffer.hpp"
#include <variant>
#include <algorithm>
#include <core/core.hpp> // libbgcode
#include <optional>
#include <transfers/partial_file.hpp>
#include "base64_stream_decoder.h"
#include "transfers/transfer.hpp"
#include <functional>
#include <span>
#include "meatpack.h"
extern "C" {
#include "heatshrink_decoder.h"
}

/**
 * @brief This is base class for reading gcode files. This defines interface that alows reading of different gcode formats.
 *        User of this class can stream data from different formats without having to deal with what format they are using
 */
class IGcodeReader {
public:
    IGcodeReader(FILE &f)
        : file(&f) {}

    IGcodeReader(IGcodeReader &&other)
        : file(nullptr) {
        *this = std::move(other);
    }

    IGcodeReader &operator=(IGcodeReader &&);

    virtual ~IGcodeReader();

    /// Result type
    enum class Result_t {
        RESULT_OK,
        RESULT_EOF,
        RESULT_TIMEOUT, // low level USB function might return timeout in case they can't get mutex in time
        RESULT_ERROR,
        RESULT_OUT_OF_RANGE, // Outside of the validity range
    };

    /// Expected image format
    enum class ImgType {
        Unknown,
        PNG,
        QOI,
    };

    enum class FileVerificationLevel {
        /// Quick verification, does not perform full CRC check,
        /// just checks for some markers that the file is valid (for exapmle GCDE at the beginning)
        quick,

        /// Everything from quick verify plus full CRC check, if the format supports it
        full,
    };

    struct FileVerificationResult {
        bool is_ok = false;
        const char *error_str = nullptr; ///< Oblitagory error message

        inline explicit operator bool() const {
            return is_ok;
        }
    };

    /**
     * @brief Start streaming metadata from gcode
     */
    virtual bool stream_metadata_start() = 0;

    /**
     * @brief Start streaming gcodes from .gcode file
     *
     * @param offset if non-zero will skip to specified offset (after powerpanic, pause etc)
     */
    virtual bool stream_gcode_start(uint32_t offset = 0) = 0;

    /**
     * @brief Find thumbnail with specified parameters and strart streaming it.
     */
    virtual bool stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) = 0;

    /**
     * @brief Get line from stream specified before by start_xx function
     */
    virtual Result_t stream_get_line(GcodeBuffer &buffer);

    /**
     * @brief Get block of data with specified size.
     * @param out_data buffer where data will be stored
     * @param size input size to get, output size really gotten, must be set to 0 on error
     */
    virtual Result_t stream_get_block(char *out_data, size_t &size) = 0;

    /**
     * @brief Get total size of gcode stream, but this will just return estimate, as with PrusaPack its expensive to get real size
     * @note Estimate is extrapolating compression ratio of first few block to entire file - so it  might be bad and used with that in mind.
     *
     */
    virtual uint32_t get_gcode_stream_size_estimate() = 0;

    /**
     * @brief Get total size of gcode stream
     */
    virtual uint32_t get_gcode_stream_size() = 0;

    /**
     * @brief Verify file contents validity (CRC and such). Not available for all formats.
     * @returns nullptr on success, error message on failure
     */
    virtual FileVerificationResult verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer = std::span<uint8_t>()) const = 0;

    /* @brief Sets what part of file are already valid.
     *
     * During a download, a file might be valid only in certain ranges. This
     * sets the already available ranges so the reader can check it is in
     * range.
     *
     * nullopt = whole file is valid (the default on construction if this is not set).
     */
    void set_validity(std::optional<transfers::PartialFile::State> validity) {
        this->validity = validity;
    }

    /**
     * @brief Get one character from current stream
     * @param out Character that was read
     * @return Result_t status of reading
     */
    inline Result_t stream_getc(char &out) {
        return (this->*ptr_stream_getc)(out);
    }

    /**
     * @brief Returns whenever file is valid enough to begin printing it (has metadata and some gcodes)
     */
    virtual bool valid_for_print() = 0;

    /**
     * @brief Load latest validity information from current transfer
     */
    void update_validity(transfers::Transfer::Path &filename);

    /// Returns whtether the reader is in an (unrecoverable) error state
    inline bool has_error() const {
        return error_str_;
    }

    /// Returns error message if has_error() is true
    inline const char *error_str() const {
        return error_str_;
    }

protected:
    inline void set_error(const char *msg) {
        assert(msg);
        error_str_ = msg;
    }

protected:
    /// Returns whether the file starts with "GCDE" - mark for recognizing a binary gcode
    /// Can be used as a part of verify_file - even for non-bgcoode files (to check that they're not disguised bgcodes actually)
    /// Modifies the file reader.
    bool check_file_starts_with_BGCODE_magic() const;

protected:
    FILE *file;

    /// nullopt -> everything available; empty state -> error
    std::optional<transfers::PartialFile::State> validity = std::nullopt;

    typedef IGcodeReader::Result_t (IGcodeReader::*stream_getc_type)(char &out);
    // implementation of stream_getc, that will be used for current stream
    stream_getc_type ptr_stream_getc = nullptr;

    IGcodeReader &operator=(const IGcodeReader &) = delete;
    IGcodeReader(const IGcodeReader &) = delete;

    /**
     * @brief Is the given range already downloaded, according to what's set with @c set_validity?
     *
     * * Start is inclusive, end is exclusive.
     * * If end is past the end of the file, it is clamped. That is, in case
     *   end is past the end of file and the "tail" of the file is valid, this
     *   returns true. Similarly, if validity is set to nullopt (fully
     *   downloaded file), the function always returns true.
     */
    bool range_valid(size_t start, size_t end) const;

private:
    /// If set to not null, the reader is considered to be in an unrecoverable error state
    const char *error_str_ = nullptr;
};

/**
 * @brief Implementation of IGcodeReader for plaintext gcodes
 */
class PlainGcodeReader : public IGcodeReader {
public:
    PlainGcodeReader(FILE &f, const struct stat &stat_info);
    PlainGcodeReader(PlainGcodeReader &&other) = default;
    PlainGcodeReader &operator=(PlainGcodeReader &&other) = default;

    virtual bool stream_metadata_start() override;
    virtual bool stream_gcode_start(uint32_t offset = 0) override;
    virtual bool stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) override;
    virtual Result_t stream_get_line(GcodeBuffer &buffer) override;
    virtual Result_t stream_get_block(char *out_data, size_t &size) override;
    virtual uint32_t get_gcode_stream_size_estimate() override;
    virtual uint32_t get_gcode_stream_size() override;
    virtual FileVerificationResult verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer) const override;
    virtual bool valid_for_print() override;

private:
    enum class output_type_t {
        metadata,
        gcode,
        thumbnail,
    };

    // Size of header that have to be valid before we start printing
    // One big file was observed to have header with size of 428 kB, so this adds some headroom
    static constexpr const size_t header_metadata_size = 512 * 1024;

    // when reading metadata and we encounter this number of gcodes, skip to end of file to search further
    static constexpr const uint32_t stop_metadata_after_gcodes_num = 1;

    // Search this many last bytes for "metadata" comments.
    // With increasing size of the comment section, this will have to be increased as well
    static constexpr const size_t search_last_x_bytes = 50000;

    output_type_t output_type = output_type_t::gcode;
    uint32_t gcodes_in_metadata = 0;
    uint32_t thumbnail_size = 0;
    Base64StreamDecoder base64_decoder;
    uint32_t file_size = 0;
    // when scanning for metadata, this will be set to position of first gcode, so subsequent calls to stream_gcode_start will start directly from gcodes
    uint32_t first_gcode_pos;

    Result_t stream_getc_impl(char &out);
    Result_t stream_getc_thumbnail_impl(char &out);

    /**
     * @brief Check if the line is thumbnail begin.
     * @param expected_width expected image width
     * @param expected_height expected image height
     * @param expected_type expected image format
     * @param allow_larger allow larger images
     * @param num_bytes number of bytes of thumbnail
     * @return true if the line is thumbnail begin
     */
    bool IsBeginThumbnail(GcodeBuffer &buffer, uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger, unsigned long &num_bytes) const;

    void set_ptr_stream_getc(IGcodeReader::Result_t (PlainGcodeReader::*ptr_stream_getc)(char &out)) {
        // this converts PlainGcodeReader::some_getc_function to IGcodeReader::some_function,
        // note that this conversion is only possible if PlainGcodeReader is subclass of IGcodeReader, and class doesn't have multiple parents
        this->ptr_stream_getc = static_cast<stream_getc_type>(ptr_stream_getc);
    }
};

/**
 * @brief Implementation of IGcodeReader for PrusaPack files
 */
class PrusaPackGcodeReader : public IGcodeReader {
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

    struct stream_restore_info_rec_t {
        uint32_t block_file_pos = 0; //< Of block header in file
        uint32_t block_start_offset = 0; //< Offset of decompressed data on start of the block
    };
    typedef std::array<stream_restore_info_rec_t, 2> stream_restore_info_t;

    auto get_restore_info() { return stream_restore_info; }
    void set_restore_info(const stream_restore_info_t &restore_info) { stream_restore_info = restore_info; }

    virtual bool valid_for_print() override;

private:
    uint32_t file_size; ///< Size of PrusaPack file in bytes
    bgcode::core::FileHeader file_header; // cached header

    struct stream_t {
        bool multiblock = false;
        bgcode::core::BlockHeader current_block_header;
        uint16_t encoding = (uint16_t)bgcode::core::EGCodeEncodingType::None;
        uint32_t block_remaining_bytes_compressed = 0; //< remaining bytes in current block
        uint32_t uncompressed_offset = 0; //< offset of next char that will be outputted
        MeatPack meatpack;
        heatshrink_decoder *hs_decoder = nullptr;

        void reset();
        ~stream_t();
    } stream;

    stream_restore_info_t stream_restore_info; //< Restore info for last two blocks

    /// helper enum for iterate_blocks function
    enum class IterateResult_t {
        Continue, //< continue iterating
        Return, //< return current block
        End, //< end search
    };

    std::optional<bgcode::core::BlockHeader> iterate_blocks(std::function<IterateResult_t(bgcode::core::BlockHeader &)> function);

    /// Pointer to function, that will get decompressed character from file, or data directly form file if not compressed
    Result_t (IGcodeReader::*ptr_stream_getc_decompressed)(char &out);

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
    stream_restore_info_rec_t *get_restore_block_for_offset(uint32_t offset);

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

/**
 * @brief Container that can open and read any gcode regardless of what type it is.
 *        Stores Plain/PrusaPack reader inside, so dynamic alocation is not needed.
 *        Also handles destruction & closing of files.
 */
class AnyGcodeFormatReader {
public:
    AnyGcodeFormatReader()
        : ptr(nullptr) {}
    AnyGcodeFormatReader(const char *filename)
        : ptr(nullptr) {
        open(filename);
    }
    AnyGcodeFormatReader(AnyGcodeFormatReader &&other);
    AnyGcodeFormatReader &operator=(AnyGcodeFormatReader &&other);
    ~AnyGcodeFormatReader();

    /**
     * @brief Open specified file
     *
     * @return nullptr if file cannot be opened
     */
    IGcodeReader *open(const char *filename);

    /**
     * @brief Get IGcodeReader that will read data from this gcode
     *
     * @return IGcodeReader*
     */
    IGcodeReader *get() const {
        return ptr;
    }

    /**
     * @brief Special getter, that returns pointer to PrusaPackGcodeReader, or null if current file is not prusaPack format
     *
     */
    PrusaPackGcodeReader *get_prusa_pack() {
        return std::get_if<PrusaPackGcodeReader>(&storage);
    }

    /**
     * @brief Return true if openning was successfull
     *
     * @return true
     * @return false
     */
    bool is_open() const {
        return ptr != nullptr;
    }

    /**
     * @brief Close the file
     */
    void close();

private:
    typedef std::variant<std::monostate, PlainGcodeReader, PrusaPackGcodeReader> storage_type_t;
    storage_type_t storage;
    IGcodeReader *ptr;

    AnyGcodeFormatReader &operator=(const AnyGcodeFormatReader &) = delete;
    AnyGcodeFormatReader(const AnyGcodeFormatReader &) = delete;
};
