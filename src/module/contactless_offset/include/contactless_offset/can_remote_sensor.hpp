#pragma once

#define DO_NOT_CHECK_ATOMIC_LOCK_FREE

#include <contactless_offset/tool_sensor.hpp>
#include <utils/atomic_circular_queue.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>

namespace tool_offset {

// Sensor implementation that receives data from a remote LDC1612
// over the CyphalBridgeQueue (Sensor Board -> XBE -> XBuddy).
//
// The sensor doesn't directly depend on puppy headers. Instead,
// lifecycle callbacks are set externally to wire up the puppies.
class CanRemoteSensor : public Sensor {
public:
    static constexpr size_t QUEUE_SIZE = 256;

    using LifecycleCallback = void (*)(void *ctx);

    explicit CanRemoteSensor(uint16_t accepted_port);
    ~CanRemoteSensor() override;

    void start() override;
    void stop() override;
    std::optional<float> get_sample() override;
    float sampling_freq() const override;
    Error get_last_error() const override;

    // Set callbacks invoked on start()/stop() to wire up the puppy layer.
    void set_lifecycle_callbacks(
        LifecycleCallback on_start, void *start_ctx,
        LifecycleCallback on_stop, void *stop_ctx);

    // Called from handle_data_frame() to check for HW errors on the remote sensor.
    using ErrorCheckCallback = bool (*)(void *ctx);
    void set_error_check_callback(ErrorCheckCallback cb, void *ctx);

    // Returns true if this port_id is the channel this sensor streams.
    bool accepts_port(uint16_t port_id) const { return port_id == accepted_port_; }

    // The Cyphal data port this sensor consumes (selects the channel). The glue
    // uses it to enable the matching channel on start.
    uint16_t accepted_port() const { return accepted_port_; }

    // Called from the puppy task context via XBuddyExtension stream callback.
    void handle_data_frame(Bytes payload);

private:
    AtomicCircularQueue<uint32_t, uint32_t, QUEUE_SIZE> sample_queue;
    std::atomic<float> cached_frequency { 0 };
    const uint16_t accepted_port_;
    uint8_t expected_sequence = 0;
    bool first_frame = true;
    std::atomic<bool> running { false };
    std::atomic<Error> last_error { Error::NONE };

    LifecycleCallback on_start_ = nullptr;
    void *on_start_ctx_ = nullptr;
    LifecycleCallback on_stop_ = nullptr;
    void *on_stop_ctx_ = nullptr;

    ErrorCheckCallback error_check_ = nullptr;
    void *error_check_ctx_ = nullptr;
};

} // namespace tool_offset
