#include <contactless_offset/can_remote_sensor.hpp>

#include <prusa3d/tool_offset_sensor/Data_1_0.h>
#include <prusa3d/common/PortIds_0_1.h>
#include <utils/byte_utils.hpp>

namespace tool_offset {

static_assert(sensor_data_port_ch0 == prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH0);
static_assert(sensor_data_port_ch1 == prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH1);

CanRemoteSensor::CanRemoteSensor(uint16_t accepted_port)
    : accepted_port_(accepted_port) {}

CanRemoteSensor::~CanRemoteSensor() {
    if (running) {
        stop();
    }
}

void CanRemoteSensor::start() {
    last_error = Error::NONE;
    sample_queue.clear();
    expected_sequence = 0;
    first_frame = true;
    cached_frequency = 0;
    running = true;

    // Registration with puppies is done externally via set_stream_callback
    // and set_config calls from marlin_server or feature code that has
    // access to the puppy headers.
    if (on_start_) {
        on_start_(on_start_ctx_);
    }
}

void CanRemoteSensor::stop() {
    running = false;

    if (on_stop_) {
        on_stop_(on_stop_ctx_);
    }

    sample_queue.clear();
}

std::optional<float> CanRemoteSensor::get_sample() {
    uint32_t raw_value;
    if (sample_queue.dequeue(raw_value)) {
        return static_cast<float>(raw_value);
    }
    return std::nullopt;
}

float CanRemoteSensor::sampling_freq() const {
    return cached_frequency.load();
}

CanRemoteSensor::Error CanRemoteSensor::get_last_error() const {
    return last_error;
}

void CanRemoteSensor::set_lifecycle_callbacks(
    LifecycleCallback on_start, void *start_ctx,
    LifecycleCallback on_stop, void *stop_ctx) {
    on_start_ = on_start;
    on_start_ctx_ = start_ctx;
    on_stop_ = on_stop;
    on_stop_ctx_ = stop_ctx;
}

void CanRemoteSensor::set_error_check_callback(ErrorCheckCallback cb, void *ctx) {
    error_check_ = cb;
    error_check_ctx_ = ctx;
}

void CanRemoteSensor::handle_data_frame(Bytes payload) {
    if (!running) {
        return;
    }

    prusa3d_tool_offset_sensor_Data_1_0 data;
    size_t inout_size = payload.size();
    if (prusa3d_tool_offset_sensor_Data_1_0_deserialize_(
            &data,
            reinterpret_cast<const uint8_t *>(payload.data()),
            &inout_size)
        != 0) {
        return;
    }

    // Learn the sequence from the first frame -- the sensor may not
    // reset its counter to 0 when (re)started via config.
    if (first_frame) {
        first_frame = false;
    } else if (data.sequence != expected_sequence) {
        last_error = Error::OVERFLOW;
    }
    expected_sequence = static_cast<uint8_t>(data.sequence + 1);

    cached_frequency = static_cast<float>(data.frequency) / 256.0f;

    int32_t current = data.sample;
    if (!sample_queue.enqueue(static_cast<uint32_t>(current & 0x0FFFFFFF))) {
        last_error = Error::OVERFLOW;
    }

    for (size_t i = 0; i < data.deltas.count; ++i) {
        current += data.deltas.elements[i];
        if (!sample_queue.enqueue(static_cast<uint32_t>(current & 0x0FFFFFFF))) {
            last_error = Error::OVERFLOW;
        }
    }

    if (error_check_ && error_check_(error_check_ctx_)) {
        last_error = Error::HW_FAILURE;
    }
}

} // namespace tool_offset
