#pragma once

#include <utils/byte_utils.hpp>
#include <variant>
#include <expected>
#include <limits>
#include <inplace_vector.hpp>

#include "util_defines.hpp"

namespace openprinttag {

/// Interface for low-level reading from NFC tags
class OPTBackend {

public:
    /// A new tag has been detected by the reader
    struct TagDetectedEvent {
        TagID tag;

        /// Antenna the tag has been detected on.
        /// If tag gets moved from antenna to antenna, the reader will emit TagLost and new TagDetected (and likely assign a new ID to the tag)
        ReaderAntenna antenna;

        inline bool operator==(const TagDetectedEvent &) const = default;
    };

    /// The reader has lost connection with a tag
    /// Please call \p forget_tag after processing this event to allow reuse of the tag ID
    struct TagLostEvent {
        TagID tag;

        inline bool operator==(const TagLostEvent &) const = default;
    };

    /// Use all antennas, do not enforce a specific one
    static constexpr ReaderAntenna no_antenna_enforce = std::numeric_limits<ReaderAntenna>::max();

    struct Config {
    public:
        /// If set, only the specified antenna will ever be used
        ReaderAntenna enforced_antenna = no_antenna_enforce;

        /// Limits tags per antenna, so that one antenna does not take all the slots
        uint8_t max_known_tags_per_antenna = 1;

        /// How many discoveries a tag can miss before it's considered lost
        /// Count it in missed discoveries rather than straight up ms - there might be long blocking operations between discoveries
        uint8_t tag_max_missed_discoveries = 4;

        /// How often should the discovery sequences run
        /// Please note that in OPT_NFCV, the discoveries alternate per antenna, so the actual interval for each antenna is longer
        uint32_t discovery_interval_ms = 250;

        /// Automatically forget tags on TagLost event
        bool auto_forget_tag : 1 = false;

        /// Whether the radio is enabled. If false, any physical IO operations return radio_disabled
        bool enable_radio : 1 = false;
    };

    struct TagInfo {
        /// Region for TLV records - that's where the OPTReader operates on
        PayloadSpan tlv_span;
    };

    using Event = std::variant<TagDetectedEvent, TagLostEvent>;

    enum class IOError : uint8_t {
        /// Trying to read/write outside of the tag memory
        outside_of_bounds,

        /// TagID is invalid (or already invalidated)
        invalid_id,

        /// The tag has been deemed invalid
        tag_invalid,

        /// Other, unspecified error.
        /// Retrying the operation might help.
        other,

        /// Data is too big and doesn't fit somewhere (tag itself, internal buffers, ...)
        data_too_big,

        /// Radio is disabled. Cannot do physical I/O.
        radio_disabled,

        /// The operation is not implemented (possibly because of the prameter combination)
        not_implemented,
    };

    template <typename T>
    using IOResult = std::expected<T, IOError>;

public:
    /// Reads \p buffer.size() bytes from \p tag into \p buffer, starting at position \p start
    /// \returns whether the operation was successful
    [[nodiscard]] virtual IOResult<void> read(TagID tag, PayloadPos start, const WritableBytes &buffer) = 0;

    /// Writes \p buffer.size() bytes to \p tag, starting at position \p start
    /// \returns whether the operation was successful
    [[nodiscard]] virtual IOResult<void> write(TagID tag, PayloadPos start, const Bytes &data) = 0;

    /// Reads a single event and stores it in \p e
    /// \param timestamp of current time, from function like freertos::millis
    /// \returns false if there is no event
    [[nodiscard]] virtual bool get_event(Event &e, uint32_t current_time_ms) = 0;

    /// Reads tag UID (identifier number hardcoded by the tag manufacturer)
    /// \param result buffer the UID will be written into
    /// \returns size of the read UID in bytes or error
    [[nodiscard]] virtual IOResult<size_t> get_tag_uid(TagID tag, const WritableBytes &buffer) = 0;

    /// Parses the Capability Container of the tag and returns relevant data
    [[nodiscard]] virtual IOResult<void> read_tag_info(TagID tag, TagInfo &target) = 0;

    /// Completely forgets the tag and allows the tag ID to be reused
    /// If the tag is still present, a new TagDetected event will be emitted
    virtual void forget_tag(TagID tag) = 0;

    virtual void reset_state() = 0;

    const Config &config() const {
        return config_;
    }
    virtual void set_config(const Config &config) {
        config_ = config;
    }

public:
    struct InitializeTagParams {
        enum class ProtectionPolicy : uint8_t {
            /// No protection
            none = 0,

            /// Irreversibly lock the registers & memory
            lock = 1,

            /// Password-protect writing
            /// NOTE: Some registers cannot be password-protected, those will be locked
            write_password = 2,

            _cnt,
        };

        /// Password to be used for ProtectionPolicy::write_password
        uint32_t password = 0;

        /// If > 0, first N bytes of the tag will be protected according to the data protection policy
        /// The number has to be aligned to the tag block size
        PayloadPos protect_first_num_bytes = 0;

        /// Determines how the tag should be protected
        /// - Protects first \p data_protection_size bytes of data
        /// - Protects utility registers such as AFI, DSFID, EAS and so on
        ProtectionPolicy protection_policy = ProtectionPolicy::none;

        /// Do not fail immediately on error, try locking/initializing whatever possible
        bool best_effort = false;
    };

    /// Initializes the tag according to the recommended practices:
    /// - Sets up utility registers such as AFI, DSFID, EAS, ...
    /// - (optionally) Locks the registers
    /// - (optionally) Locks the headers data
    /// Might be implemented only for specific chip models and specific parameter configurations
    /// All init-time data should be written to the tag beforehand using \p write
    [[nodiscard]] virtual IOResult<void> initialize_tag(TagID tag, const InitializeTagParams &params) {
        (void)tag, (void)params;
        return std::unexpected(IOError::not_implemented);
    }

    /// Removes write protection for the memory protected by the specified write password
    /// !!! This does not fully undo the password protection, nor does it change the password
    /// !!! Registers and other things will still be protected
    [[nodiscard]] virtual IOResult<void> unlock_tag(TagID tag, uint32_t password) {
        (void)tag, (void)password;
        return std::unexpected(IOError::not_implemented);
    }

protected:
    Config config_;
};

} // namespace openprinttag
