#pragma once

#include <algorithm>
#include <optional>
#include <span>
#include <utils/abstract_byte_reader.hpp>

#include <gcode_buffer.hpp>
#include <transfers/partial_file.hpp>
#include <transfers/transfer.hpp>

#include "gcode_reader_restore_info.hpp"
#include "gcode_reader_result.hpp"
#include <bsod/bsod.h>

#if HAS_E2EE_SUPPORT()
    #include <e2ee/e2ee.hpp>
#endif

class IGcodeReader {
public:
    enum class Continuations : uint8_t {
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
    enum class ImgType : uint8_t {
        Unknown,
        PNG,
        QOI,
    };

    struct ThumbnailDetails {
        uint16_t width;
        uint16_t height;
        unsigned long num_bytes;
        ImgType type;
    };

    using StreamRestoreInfo = GCodeReaderStreamRestoreInfo;

    /// What is currently being streamed (determined by the last stream_XX_start and its success)
    enum class StreamMode : uint8_t {
        none,
        metadata,
        gcode,
        thumbnail,
    };

    /// Positions where things start.
    struct Index {
        /// A position, or not_indexed.
        ///
        /// std::optional would be better, but takes more space.
        ///
        /// Note: This is raw file offset (bytes), not anything related to
        /// stream positions.
        using Position = uint32_t;

        /// Not found, but we are not 100% sure.
        ///
        /// We may have looked or not.
        static constexpr Position not_indexed = 0xFFFFFFFF;

        /// We are sure it is not there. No reason to look again.
        static constexpr Position not_present = not_indexed - 1;

        static constexpr size_t thumbnail_slots = 3;

        Position gcode = not_indexed;
        Position metadata = not_indexed;

        struct Thumbnail {
            // Request section
            uint16_t w = 0;
            uint16_t h = 0;
            ImgType type = ImgType::Unknown;
            // Output
            Position position = not_indexed;
        };

        std::array<Thumbnail, thumbnail_slots> thumbnails;

        bool indexed() const {
            // Simplified. We don't bother with formats that index only thumbnails.
            return gcode != not_indexed || metadata != not_indexed;
        }

        static bool present(Position position) {
            return position != not_indexed && position != not_present;
        }
    };

public:
    /// Pre-index the file.
    ///
    /// If the file / type doesn't support indexing, it doesn't modify the out
    /// parameter at all.
    virtual void generate_index(Index &out, bool ignore_crc = false);

    /**
     * @brief Start streaming metadata from gcode
     */
    virtual bool stream_metadata_start(const Index *index = nullptr) = 0;

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
     * - As per popular demand, we now include option to ignore CRC, which
     *   significantly speeds up print preview screen.
     *
     * @param offset if non-zero will skip to specified offset (after powerpanic, pause etc)
     * @param ignore_crc if non-zero will ignore CRC, which should be used with caution
     * @param index (if present) - preindexed to look up things faster. If not
     *   nullptr, must come from previous call into index() on the same reader.
     */
    virtual Result_t stream_gcode_start(uint32_t offset = 0, bool ignore_crc = false, const Index *index = nullptr) = 0;

    /**
     * @brief Find thumbnail with specified parameters and start streaming it.
     * May return nullptr if thumbnail was not found.
     * Lifetime of returned pointer is tied to the lifetime of `this` gcode
     * reader, you are not supposed to free it. Returned pointer is valid until
     * some other method is invoked on `this` gcode reader.
     * End of data is signaled by byte reader returning less data than
     * the capacity of the provided buffer, eventually returning empty span.
     * Errors are not indicated specially.
     */
    virtual AbstractByteReader *stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) = 0;
#if 0
    // #error dead code found by automatic analyses (see BFW-5461)
    // TODO: Once we need it (do we? Will we?)
    /**
     * Start streaming a thumbnail on given position.
     *
     * Overload for the above, but with a position previously returned as part of index.
     */
    virtual AbstractByteReader *stream_thumbnail_start(Index::Position position);
#endif

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
     * @brief Get one character from current stream
     * @param out Character that was read
     * @return Result_t status of reading
     */
    virtual Result_t stream_getc(char &out) = 0;

    /**
     * @brief Returns whenever file is valid enough to begin printing it (has metadata and some gcodes)
     *
     * Also checks the sequence of the blocks is correct. For encrypted gcodes it also checks if it is encrypted for this printer,
     * that it does not have anything between the blocks. If the full check is true it also verifies the identity block signature
     * and the hashes of metadata and keyblocks are correct.
     */
    virtual bool valid_for_print(bool full_check) = 0;

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

#if HAS_E2EE_SUPPORT()
    virtual e2ee::IdentityInfo get_identity_info() const = 0;
    virtual bool has_identity_info() const = 0;
#endif
};

/**
 * @brief This is base class for reading gcode files. This defines interface that alows reading of different gcode formats.
 *        User of this class can stream data from different formats without having to deal with what format they are using
 */
class GcodeReaderCommon : public IGcodeReader {

protected:
    // For unittest purposes only.
    GcodeReaderCommon() {}

    GcodeReaderCommon(unique_file_ptr &&f)
        : file(std::move(f)) {}

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

#if HAS_E2EE_SUPPORT()
    bool has_identity_info() const override {
        return identity_info.has_value();
    }

    e2ee::IdentityInfo get_identity_info() const override {
        debug_assert(identity_info.has_value());
        return identity_info.value();
    }
#endif

protected:
    inline void set_error(const char *msg) {
        debug_assert(msg);
        error_str_ = msg;
    }

#if HAS_E2EE_SUPPORT()
    void set_identity_info(const e2ee::IdentityInfo &info) {
        identity_info = info;
    }
#endif

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

#if HAS_E2EE_SUPPORT()
    std::optional<e2ee::IdentityInfo> identity_info;
#endif
};
