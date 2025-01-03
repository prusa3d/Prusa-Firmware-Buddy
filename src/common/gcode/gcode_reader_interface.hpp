#pragma once

#include <algorithm>
#include <optional>
#include <span>

#include <gcode_buffer.hpp>
#include <transfers/partial_file.hpp>
#include <transfers/transfer.hpp>

#include "gcode_reader_restore_info.hpp"
#include "gcode_reader_result.hpp"

class IGcodeReader {
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

    /// Result type
    using Result_t = GCodeReaderResult;

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
    /**
     * @brief Start streaming metadata from gcode
     */
    virtual bool stream_metadata_start() = 0;

    /**
     * @brief Start streaming gcodes from .gcode or .bgcode file
     *
     * Unlike the other stream_ functions, this checks CRCs on the file -
     * including the metadata and thumbnails before the actual gcode block.
     * The other functions are left without checking the CRC, because:
     * - Performance (they are being called from many places at arbitrary
     *   times, this one is called at the start of print).
     * - The damage from a corrupt metadata or thumbnail is significantly
     *   smaller than a corruption in print instructions.
     *
     * @param offset if non-zero will skip to specified offset (after powerpanic, pause etc)
     */
    virtual Result_t stream_gcode_start(uint32_t offset = 0) = 0;

    /**
     * @brief Find thumbnail with specified parameters and strart streaming it.
     */
    virtual bool stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) = 0;

    /**
     * @brief Get line from stream specified before by start_xx function
     */
    virtual Result_t stream_get_line(GcodeBuffer &buffer, Continuations) = 0;

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

    virtual StreamRestoreInfo get_restore_info() = 0;

    virtual void set_restore_info(const StreamRestoreInfo &) = 0;

    /**
     * @brief Verify file contents validity (CRC and such). Not available for all formats.
     * @returns nullptr on success, error message on failure
     */
    virtual FileVerificationResult verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer = std::span<uint8_t>()) const = 0;

    /**
     * @brief Get one character from current stream
     * @param out Character that was read
     * @return Result_t status of reading
     */
    virtual Result_t stream_getc(char &out) = 0;

    /**
     * @brief Returns whenever file is valid enough to begin printing it (has metadata and some gcodes)
     */
    virtual bool valid_for_print() = 0;

    /**
     * @brief Load latest validity information from current transfer
     */
    virtual void update_validity(const char *filename) = 0;

    /**
     * Is the file valid in full - completely downloaded?
     */
    virtual bool fully_valid() const = 0;

    /// Returns whether the reader is in an (unrecoverable) error state
    virtual bool has_error() const = 0;

    /// Returns error message if has_error() is true
    virtual const char *error_str() const = 0;
};

/**
 * @brief This is base class for reading gcode files. This defines interface that alows reading of different gcode formats.
 *        User of this class can stream data from different formats without having to deal with what format they are using
 */
class GcodeReaderCommon : public IGcodeReader {

protected:
    // For unittest purposes only.
    GcodeReaderCommon() {}

    GcodeReaderCommon(FILE &f)
        : file(&f) {}

    GcodeReaderCommon(GcodeReaderCommon &&other) = default;

    ~GcodeReaderCommon() = default;

    GcodeReaderCommon &operator=(GcodeReaderCommon &&) = default;

public:
    void set_validity(std::optional<transfers::PartialFile::State> validity) {
        this->validity = validity;
    }

    bool fully_valid() const override {
        return !validity.has_value() || validity->fully_valid();
    }

    Result_t stream_getc(char &out) override {
        return (this->*ptr_stream_getc)(out);
    }

    void update_validity(const char *filename) override;

    bool has_error() const override {
        return error_str_;
    }

    const char *error_str() const override {
        return error_str_;
    }

protected:
    inline void set_error(const char *msg) {
        assert(msg);
        error_str_ = msg;
    }

    IGcodeReader::Result_t stream_get_line_common(GcodeBuffer &b, Continuations line_continuations);

    /// Returns whether the file starts with "GCDE" - mark for recognizing a binary gcode
    /// Can be used as a part of verify_file - even for non-bgcoode files (to check that they're not disguised bgcodes actually)
    /// Modifies the file reader.
    bool check_file_starts_with_BGCODE_magic() const;

    inline StreamMode stream_mode() const {
        return stream_mode_;
    }

protected:
    unique_file_ptr file;

    /// nullopt -> everything available; empty state -> error
    std::optional<transfers::PartialFile::State> validity = std::nullopt;

    using stream_getc_type = IGcodeReader::Result_t (IGcodeReader::*)(char &out);

    // implementation of stream_getc, that will be used for current stream
    stream_getc_type ptr_stream_getc = nullptr;

    StreamMode stream_mode_ = StreamMode::none;

    GcodeReaderCommon &operator=(const GcodeReaderCommon &) = delete;
    GcodeReaderCommon(const GcodeReaderCommon &) = delete;

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
