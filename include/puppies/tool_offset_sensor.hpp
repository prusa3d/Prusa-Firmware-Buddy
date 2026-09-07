///@file
#pragma once

#include "PuppyModbus.hpp"
#include <atomic>
#include <tool_offset_sensor/modbus.hpp>
#include <freertos/mutex.hpp>
#include <option/has_tool_offset_sensor.h>
#include <xbuddy_extension/shared_enums.hpp>

static_assert(HAS_TOOL_OFFSET_SENSOR());

namespace buddy::puppies {

/// Represents virtual Tool Offset Sensor modbus device
/// This handles synchronization between tasks and caching the data.
class ToolOffsetSensor final {
public:
    xbuddy_extension::NodeState get_node_state() const;
    bool has_sensor_fault() const;
    bool has_errors() const;

    void set_config(bool ch0_enabled, bool ch1_enabled);

    bool is_enabled() const;

    void set_enabled(bool set);

    // These are called from the puppy task.
    CommunicationStatus refresh(PuppyModbus &);
    CommunicationStatus initial_scan(PuppyModbus &);

private:
    mutable freertos::Mutex mutex;

    std::atomic<bool> is_enabled_ = false;
    bool valid = false;
    bool all_valid() const;

    using Status = tool_offset_sensor::modbus::Status;
    ModbusInputRegisterBlock<Status::address, Status> status;

    using Config = tool_offset_sensor::modbus::Config;
    ModbusHoldingRegisterBlock<Config::address, Config> config;

    CommunicationStatus refresh_input(PuppyModbus &, uint32_t max_age);
    CommunicationStatus refresh_holding(PuppyModbus &);
};

extern ToolOffsetSensor tool_offset_sensor;

} // namespace buddy::puppies
