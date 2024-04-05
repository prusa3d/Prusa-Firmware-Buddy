#pragma once

#include <algorithm>
#include <optional>
#include <span>

#include <gcode_buffer.hpp>
#include <transfers/partial_file.hpp>
#include <transfers/transfer.hpp>

#include "gcode_reader_restore_info.hpp"

/**
 * @brief This is base class for reading gcode files. This defines interface that alows reading of different gcode formats.
 *        User of this class can stream data from different formats without having to deal with what format they are using
 */
class IGcodeReader {

protected:
    // For unittest purposes only.
    IGcodeReader() {}

    IGcodeReader(FILE &f)
        : file(&f) {}

    IGcodeReader(IGcodeReader &&other) = default;

    ~IGcodeReader() = default;

    IGcodeReader &operator=(IGcodeReader &&) = default;

public:
    enum class Continuations {
        /// Anything over the limit is discarded.
        ///
        /// If anything was discarded can be checked with line_complete.
        Discard,
        /// Too long line is returned in multiple chunks. The last chunk is
        /// marked with line_complete being true.
        Split,
    };

    Continuations line_continuations = Continuations::Discard;

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

    using StreamRestoreInfo = GCodeReaderStreamRestoreInfo;

    /// What is currently being streamed (determined by the last stream_XX_start and its success)
    enum class StreamMode {
        none,
        metadata,
        gcode,
        thumbnail,
    };

public:
    inline StreamMode stream_mode() const {
        return stream_mode_;
    }

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

    virtual StreamRestoreInfo get_restore_info() { return {}; }

    virtual void set_restore_info(const StreamRestoreInfo &) {}

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
        assert(ptr_stream_getc);
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
    unique_file_ptr file;

    /// nullopt -> everything available; empty state -> error
    std::optional<transfers::PartialFile::State> validity = std::nullopt;

    using stream_getc_type = IGcodeReader::Result_t (IGcodeReader::*)(char &out);

    // implementation of stream_getc, that will be used for current stream
    stream_getc_type ptr_stream_getc = nullptr;

    StreamMode stream_mode_ = StreamMode::none;

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
