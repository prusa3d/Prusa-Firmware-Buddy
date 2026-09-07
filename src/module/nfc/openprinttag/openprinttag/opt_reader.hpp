#pragma once

#include <expected>
#include <span>
#include <utils/byte_utils.hpp>
#include <string_view>

#include <inplace_vector.hpp>
#include <inplace_function.hpp>
#include <utils/enum_array.hpp>
#include <utils/cache.hpp>

#include "util_defines.hpp"
#include "opt_backend.hpp"
#include "opt_defines.hpp"

namespace openprinttag {

/// Higher-level NFC interface, working with the OpenPrintTag format
class OPTReader {

public:
    /// Maximum size of tag we support (have buffers for)
    static constexpr size_t max_tag_size = 512;

    static constexpr size_t max_meta_region_size = 32;

    static constexpr size_t metadata_cache_capacity = 4;

    struct Params {
        std::string_view mime_type = "application/vnd.openprinttag";

        /// Whether the tag has a meta region.
        /// False means the the payload only contains the main section (no aux, no meta)
        bool has_meta_region : 1 = true;
    };

    // Do not change indexes - reflected in the DSDL
    enum class Error : uint8_t {
        /// The field or region is not present on the tag (but the tag is otherwise OK)
        field_not_present = 0,

        /// The field type in the CBOR is not readable with the used function (for example trying to read float using read_field_bool)
        wrong_field_type = 1,

        /// Cannot write to the region, because it is write protected (can be protected on various levels)
        write_protected = 2,

        /// The section we are trying to read from is corrupted.
        /// Re-reading the section won't help, but clear_section() could.
        region_corrupt = 3,

        /// The tag has been determined to be not a valid OpenPrintTag tag (wrong mime, corrupted contents, ...)
        /// Retrying won't help, the chip needs to be re-formatted.
        tag_invalid = 4,

        /// Data is too big and doesn't fit somewhere (tag itself, internal buffers, ...)
        data_too_big = 5,

        /// Other, unspecified error (comm error, ...)
        /// Retrying the operation might help.
        other = 6,

        /// The operation is not implemented (possibly because of the prameter combination)
        not_implemented = 7,

        /// The radio is disabled - cannot do physical IO operation
        radio_disabled = 8,

        _cnt,
    };

    static Error to_reader_error(OPTBackend::IOError error);

    template <typename T>
    using IOResult = std::expected<T, Error>;

    using Event = OPTBackend::Event;

    struct RegionMetadata {
        /// Region span, from the start of the NFC tag
        /// ! Warning - in the meta region CBOR, the offsets are relative to the NDEF payload start
        PayloadSpan span;

        inline bool is_present() const {
            return span.size != 0;
        }
    };

    struct TagMetadata {
        EnumArray<Region, RegionMetadata, 3> region;

        /// Stores whether the tag is a valid Prusa tag
        bool is_valid : 1 = false;

        const RegionMetadata &meta_region() const {
            return region[Region::meta];
        }
        const RegionMetadata &main_region() const {
            return region[Region::main];
        }
        const RegionMetadata &aux_region() const {
            return region[Region::auxiliary];
        }
    };

    struct WriteReport {
        /// Whether the field existed before the write
        bool field_existed;

        /// Anything changed at all by the operation
        bool changed;

        bool operator==(const WriteReport &) const = default;
    };

public:
    OPTReader(OPTBackend &backend);

    const Params &params() const {
        return params_;
    }

    /// Lower-level reader interface
    // If used for writing operations, calling invalidate_cache is recommended.
    /// !!! Use at your own risk.
    OPTBackend &backend() {
        return backend_;
    }
    const OPTBackend &backend() const {
        return backend_;
    }

    /// Configures the reader with the specified parameters
    void set_params(const Params &set);

    /// See OPTBackend::get_event
    [[nodiscard]] bool get_event(Event &event, uint32_t current_time_ms);

    /// Invalidates all the cache records the reader has for the specified tag
    /// To be called if the tag gets somehow changed outside of the OPTReader control
    void invalidate_cache(TagID tag);

    /// Completely forgets the tag and allows the tag ID to be reused
    /// If the tag is still present, a new TagDetected event will be emitted
    void forget_tag(TagID tag);

    /// Completely reset reader state.
    /// Should invalidate all caches and memory.
    /// As a result, all nearby tags should emit TagDetected events
    void reset_state();

public:
    /// \returns metadata for the tag. Employs the cache if possible
    [[nodiscard]] IOResult<const TagMetadata *> read_metadata(TagID tag);

    /// Enumerates fields of section \param section in \param tag and stores the list of the fields into \param result.
    /// \returns Number of fields stored in \p result
    [[nodiscard]] IOResult<std::span<const Field>> enumerate_fields(TagID tag, Region section, const std::span<Field> &result);

    /// Removes the specified field from the tag.
    [[nodiscard]] IOResult<WriteReport> remove_field(const TagField &field);

    [[nodiscard]] IOResult<bool> read_field_bool(const TagField &field);
    [[nodiscard]] IOResult<int32_t> read_field_int32(const TagField &field);
    [[nodiscard]] IOResult<int64_t> read_field_int64(const TagField &field);
    [[nodiscard]] IOResult<float> read_field_float(const TagField &field);
    [[nodiscard]] IOResult<std::string_view> read_field_string(const TagField &field, const std::span<char> &buffer);
    [[nodiscard]] IOResult<std::basic_string_view<std::byte>> read_field_bytes(const TagField &field, const WritableBytes &buffer);
    [[nodiscard]] IOResult<std::span<const uint16_t>> read_field_uint16_array(const TagField &field, const std::span<uint16_t> &buffer);

    [[nodiscard]] IOResult<WriteReport> write_field_bool(const TagField &field, bool value);
    [[nodiscard]] IOResult<WriteReport> write_field_int32(const TagField &field, int32_t value);
    [[nodiscard]] IOResult<WriteReport> write_field_int64(const TagField &field, int64_t value);
    [[nodiscard]] IOResult<WriteReport> write_field_float(const TagField &field, float value);
    [[nodiscard]] IOResult<WriteReport> write_field_string(const TagField &field, const std::string_view &value);
    [[nodiscard]] IOResult<WriteReport> write_field_bytes(const TagField &field, const std::basic_string_view<std::byte> &value);
    [[nodiscard]] IOResult<WriteReport> write_field_uint16_array(const TagField &field, const std::span<const uint16_t> &value);

private: // Reading internal functions
    /// Read the specified span from the NFC tag and stores it into the read buffer
    /// \returns the read data (might not be aligned with the read buffer)
    [[nodiscard]] IOResult<WritableBytes> read_span(const TagPayloadSpan &span);

    /// Defined in the .cpp
    template <typename T, auto f>
    [[nodiscard]] inline IOResult<T> read_field_primitive(const TagField &field);

    /// Defined in the .cpp, to hide the cbor library
    struct CBORValue;

    struct ContinueEnumerating {};

    struct StopEnumerating {};

    using EnumerateCallbackResult = std::variant<ContinueEnumerating, StopEnumerating, Error, int>;

    /// The callback is expected to read or skip every value
    /// \returns result of cbor_read or cbor_skip
    using EnumerateCallback = stdext::inplace_function<EnumerateCallbackResult(Field field, CBORValue v)>;

    /// Calls \p callback for each field in the \p section of \p tag
    /// \returns size of the read CBOR object (if the iteration was not stopped using StopEnumerating)
    [[nodiscard]] IOResult<PayloadPos> enumerate_fields_callback(TagID tag, Section section, const EnumerateCallback &callback);

    /// Calls \p callback for each fielkd in the section at \p span
    /// \returns size of the read CBOR object (if the iteration was not stopped using StopEnumerating)
    [[nodiscard]] IOResult<PayloadPos> enumerate_fields_callback(const TagPayloadSpan &span, const EnumerateCallback &callback);

    using ReadFieldCallbackResult = std::variant<int, Error>;

    using ReadFieldCallback = stdext::inplace_function<ReadFieldCallbackResult(CBORValue v)>;

    /// Calls \p callback over CBOR \p field
    IOResult<void> read_field_impl(const TagField &field, const ReadFieldCallback &callback);

private: // Writing internal functions
    /// Defined in the .cpp, to hide the cbor library
    struct CBOREncoder;

    using WriteFieldCallback = stdext::inplace_function<int(CBOREncoder e)>;

    /// Writes the specified field on the tag.
    /// \param callback is called for writing the field value. If not set, the function will remove the field instead.
    IOResult<WriteReport> write_field_impl(const TagField &field, const WriteFieldCallback &callback);

private:
    /// Lower-level reader that we're using to communicate with the NFC
    OPTBackend &backend_;

private:
    /// Buffer for NFC reading operations
    std::array<std::byte, max_tag_size> read_buffer_;

    /// Buffer for NFC writing operations
    std::array<std::byte, max_tag_size> write_buffer_;

    struct {
        TagID tag = invalid_nfc_tag;

        /// What is currently stored in the read buffer
        PayloadSpan span;

    } read_buffer_cache_;

    /// Cache of basic NFC tag information, so that we don't have to parse it for every access
    Cache<TagID, TagMetadata, metadata_cache_capacity> metadata_cache_;

private:
    Params params_;
};

} // namespace openprinttag
