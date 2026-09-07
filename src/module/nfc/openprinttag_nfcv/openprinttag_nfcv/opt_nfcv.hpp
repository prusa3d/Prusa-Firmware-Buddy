#pragma once

#define DO_NOT_CHECK_ATOMIC_LOCK_FREE

#include <utils/atomic_circular_queue.hpp>
#include <utils/timing/rate_limiter.hpp>
#include <openprinttag/opt_backend.hpp>
#include <nfcv/types.hpp>
#include <nfcv/rw_interface.hpp>
#include <inplace_vector.hpp>
#include <inplace_function.hpp>
#include <utils/byte_utils.hpp>

namespace openprinttag {

class OPTBackend_NFCV final : public OPTBackend {
public:
    static constexpr size_t MAX_ANTENNA_COUNT = 2;
    static constexpr size_t MAX_KNOWN_TAGS = 8;

    OPTBackend_NFCV(nfcv::ReaderWriterInterface &reader, const Config &initial_config = {});

    [[nodiscard]] IOResult<void> read(TagID tag, PayloadPos start, const WritableBytes &buffer) final;

    [[nodiscard]] IOResult<void> write(TagID tag, PayloadPos start, const Bytes &buffer) final;

    [[nodiscard]] bool get_event(Event &e, uint32_t current_time_ms) final;

    [[nodiscard]] IOResult<size_t> get_tag_uid(TagID tag, const WritableBytes &buffer) final;

    [[nodiscard]] virtual IOResult<void> read_tag_info(TagID tag, TagInfo &target) final;

    void forget_tag(TagID tag) final;

    void reset_state() final;

    [[nodiscard]] virtual IOResult<void> initialize_tag(TagID tag, const InitializeTagParams &params) override;

    [[nodiscard]] virtual IOResult<void> unlock_tag(TagID tag, uint32_t password) override;

    virtual void set_config(const Config &config) override;

private:
    enum class TagType : uint8_t {
        slix2,
        unknown,
    };
    struct TagData {
        enum class State : uint8_t {
            free,
            known,
            lost
        };

        nfcv::UID uid;
        nfcv::ReaderWriterInterface::AntennaID antenna;
        uint8_t block_size;
        uint8_t block_count;

        /// How many discoveries the tag wasn't consecutively detected for
        uint8_t missed_discovery_count = 0;
        State state : 2 = State::free;
        TagType tag_type : 2;
    };

    class FieldGuard : public Uncopyable {

    public:
        FieldGuard(OPTBackend_NFCV &backend, ReaderAntenna antenna);
        ~FieldGuard();

        explicit operator bool() const {
            return result.has_value();
        }

        auto error() const {
            return result.error();
        }

        const IOResult<void> result;
    };

    nfcv::ReaderWriterInterface &reader;
    std::array<TagData, MAX_KNOWN_TAGS> tags {};

    /// Stores how many known tags each antenna has
    std::array<uint8_t, MAX_ANTENNA_COUNT> known_tags_per_antenna {};

    RateLimiter<uint32_t> discoveries_limiter;
    nfcv::ReaderWriterInterface::AntennaID discovery_antenna = 0;

    AtomicCircularQueue<Event, uint8_t, 4> events;

    /// Runs a next discovery procedure, where we try to find new tags, or detect tags that are gone.
    void run_next_discovery();

    /// Validates if tag_id is referencing existing NFC tag (eather known or lost) - but the index references valid data
    [[nodiscard]] bool is_valid(TagID tag_id);

    using IOOpFunc = nfcv::Result<void>(const TagData &tag_data);

    /// Helper to intialize the reader for next io operation
    /// TODO: With mutiple readers this should return what reader was used
    [[nodiscard]] IOResult<void> io_op(TagID tag, PayloadPos start, size_t buffer_size, const stdext::inplace_function<IOOpFunc> &impl);

    /// Helper function for write implementation
    [[nodiscard]] nfcv::Result<void> write_impl(const TagData &tag_data, PayloadPos start, const Bytes &buffer);

    /// Helper function for read implementation
    [[nodiscard]] nfcv::Result<void> read_impl(const TagData &tag_data, PayloadPos start, const WritableBytes &buffer);

    /// Attempts to mark the tag as lost
    void try_report_tag_lost(TagID tag_id);
};

} // namespace openprinttag
