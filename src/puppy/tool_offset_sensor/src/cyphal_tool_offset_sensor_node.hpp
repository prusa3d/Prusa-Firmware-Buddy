#pragma once

#include <cyphal_node.hpp>
#include <freertos/mutex.hpp>

// Optimize Cyphal headers for size
#pragma GCC push_options
#pragma GCC optimize("Os")

#include <prusa3d/common/PortIds_0_1.h>
#include <prusa3d/tool_offset_sensor/Status_1_0.h>
#include <prusa3d/tool_offset_sensor/Config_1_0.h>
#include <prusa3d/tool_offset_sensor/Data_1_0.h>
#include <honeybee_shared_fault.hpp>
#include <utils/uncopyable.hpp>

#pragma GCC pop_options

using puppy::fault::SharedFault;

namespace tool_offset_sensor::cyphal {

enum Fault {
    // no specific faults yet
    _specific_count,
    _specific_last = _specific_count - 1,

    // Shared faults
    _shared_first = SharedFault::_shared_first, ///< This is a barrier for puppy specific faults

    can = SharedFault::can,
    mcu_overheat = SharedFault::mcu_overheat,
    pcb_overheat = SharedFault::pcb_overheat,
    data_timeout = SharedFault::data_timeout,
    heartbeat_missing = SharedFault::heartbeat_missing,
    unknown = SharedFault::unknown,
};

/// @brief Specify templated conversion.
static inline Fault from_shared(puppy::fault::SharedFault f) { return puppy::fault::from_shared<Fault>(f); };

using ToolOffsetSensorNodeBase = can::cyphal::Node<
    prusa3d_tool_offset_sensor_Status_1_0_Traits, prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_STATUS,
    prusa3d_tool_offset_sensor_Config_1_0_Traits, prusa3d_common_PortIds_0_1_SRV_TOOL_OFFSET_SENSOR_CONFIG,
    Fault,
    CANARD_NODE_ID_UNSET,
    can::cyphal::defaults::WatchNodes,
    can::cyphal::defaults::Notify,
    2, // node_id_request
    13 // MAX_REGISTERS
    >;

class ToolOffsetSensorNode : public ToolOffsetSensorNodeBase, Uncopyable {

public:
    using UID = std::array<uint8_t, sizeof(uavcan_node_GetInfo_Response_1_0::unique_id)>;

    ToolOffsetSensorNode(const UID &uid);

    struct ChannelConfig {
        bool ch0_enabled = false;
        bool ch1_enabled = false;
        bool operator==(const ChannelConfig &) const = default;
    };

    struct SensorState {
        bool ch0_active = false;
        bool ch1_active = false;
        bool sensor_fault = false;
        uint8_t sensor_errors = 0;
    };

    // Read config snapshot atomically (mutex-guarded)
    ChannelConfig get_config();

    // Read/write sensor state atomically (mutex-guarded)
    SensorState get_sensor_state();
    void set_sensor_state(const SensorState &state);

    // Thread-safe publish methods for main task to call
    void publish_data_ch0(const prusa3d_tool_offset_sensor_Data_1_0 &data);
    void publish_data_ch1(const prusa3d_tool_offset_sensor_Data_1_0 &data);

protected:
    void app_init() override;
    void app_tick(int64_t now_us) override;
    void app_tick_pnp(int64_t now_us) override;
    void write_config(const ConfigTraits::Request::Type &config) override;
    void update_status(StatusTraits::Type &data) override;

private:
    freertos::Mutex config_mutex;
    ChannelConfig config;

    freertos::Mutex sensor_state_mutex;
    SensorState sensor_state;

    can::cyphal::SenderDataTraited<
        prusa3d_tool_offset_sensor_Data_1_0_Traits,
        prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH0>
        data_ch0_sender;

    can::cyphal::SenderDataTraited<
        prusa3d_tool_offset_sensor_Data_1_0_Traits,
        prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH1>
        data_ch1_sender;
};

} // namespace tool_offset_sensor::cyphal
