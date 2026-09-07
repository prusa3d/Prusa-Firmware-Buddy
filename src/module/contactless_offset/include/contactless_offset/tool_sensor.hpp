#pragma once
#include <cstdint>
#include <optional>
#include <memory>

namespace tool_offset {

class Sensor {
public:
    enum class Error {
        NONE,
        HW_FAILURE,
        OVERFLOW
    };

    virtual ~Sensor() = default;

    // Start and stop shall initialize the sensor and clear any internal buffers
    // or errors.
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual std::optional<float> get_sample() = 0;
    virtual float sampling_freq() const = 0;

    virtual Error get_last_error() const = 0;
};

inline constexpr uint16_t sensor_data_port_ch0 = 900;
inline constexpr uint16_t sensor_data_port_ch1 = 902;

std::unique_ptr<Sensor> make_sensor(uint16_t accepted_port);

} // namespace tool_offset
