// Wires CanRemoteSensor to the XBuddyExtension Cyphal bridge.

#include <contactless_offset/can_remote_sensor.hpp>
#include <contactless_offset/tool_sensor.hpp>
#include <puppies/cyphal_bridge.hpp>
#include <puppies/tool_offset_sensor.hpp>
#include <utils/byte_utils.hpp>

namespace tool_offset {

static void on_stream(uint16_t port_id, Bytes payload, void *ctx) {
    auto *sensor = static_cast<CanRemoteSensor *>(ctx);
    if (sensor->accepts_port(port_id)) {
        sensor->handle_data_frame(payload);
    }
}

static void on_start(void *ctx) {
    auto *sensor = static_cast<CanRemoteSensor *>(ctx);
    buddy::puppies::cyphal_bridge.set_stream_callback(on_stream, sensor);
    // Enable only the channel this sensor streams.
    const uint16_t port = sensor->accepted_port();
    buddy::puppies::tool_offset_sensor.set_config(port == sensor_data_port_ch0, port == sensor_data_port_ch1);
}

static void on_stop(void * /*ctx*/) {
    buddy::puppies::tool_offset_sensor.set_config(false, false);
    buddy::puppies::cyphal_bridge.set_stream_callback(nullptr, nullptr);
}

static bool check_hw_error(void *) {
    return buddy::puppies::tool_offset_sensor.has_errors();
}

std::unique_ptr<Sensor> make_sensor(uint16_t accepted_port) {
    auto sensor = std::make_unique<CanRemoteSensor>(accepted_port);
    sensor->set_lifecycle_callbacks(on_start, sensor.get(), on_stop, sensor.get());
    sensor->set_error_check_callback(check_hw_error, nullptr);
    return sensor;
}

} // namespace tool_offset
