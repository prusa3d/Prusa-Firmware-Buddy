#define DO_NOT_CHECK_ATOMIC_LOCK_FREE

#include "cyphal_tool_offset_sensor_node.hpp"

#include <cyphal_pnp.hpp>
#include <hal.hpp>
#include <freertos/timing.hpp>
#include <mutex>
#include <prusa3d/common/CustomExecuteCommand_1_0.h>
#include <prusa3d/common/SharedFault_1_0.h>
#include <bsod/bsod.h>

namespace tool_offset_sensor::cyphal {

using namespace can::cyphal;

ToolOffsetSensorNode::ToolOffsetSensorNode(const UID &uid)
    : ToolOffsetSensorNodeBase(uid.data(),
        "cz.prusa3d.honeybee.tool_offset_sensor") {
}

void ToolOffsetSensorNode::app_init() {
    // Register data senders
    data_ch0_sender.add_to_task();
    data_ch1_sender.add_to_task();
    port_list.add(data_ch0_sender);
    port_list.add(data_ch1_sender);
    registers.add_port_name_set("pub.data_ch0",
        prusa3d_tool_offset_sensor_Data_1_0_Traits::full_name_and_version,
        data_ch0_sender.get_port_id());
    registers.add_port_name_set("pub.data_ch1",
        prusa3d_tool_offset_sensor_Data_1_0_Traits::full_name_and_version,
        data_ch1_sender.get_port_id());

    debug_assert(registers.get_max_registers() == registers.get_register_count());
}

void ToolOffsetSensorNode::app_tick(int64_t now_us) {
    // blink status LED slowly to indicate address assigned
    hal::set_status_led((now_us / (1024 * 1024)) % 2);
}

void ToolOffsetSensorNode::app_tick_pnp(int64_t now_us) {
    // blink status LED quickly to indicate PnP discovery mode
    hal::set_status_led((now_us / (128 * 1024)) % 2);
}

void ToolOffsetSensorNode::write_config(const ConfigTraits::Request::Type &cfg) {
    std::lock_guard lock(config_mutex);
    config.ch0_enabled = cfg.ch0_enabled;
    config.ch1_enabled = cfg.ch1_enabled;
}

ToolOffsetSensorNode::ChannelConfig ToolOffsetSensorNode::get_config() {
    std::lock_guard lock(config_mutex);
    return config;
}

ToolOffsetSensorNode::SensorState ToolOffsetSensorNode::get_sensor_state() {
    std::lock_guard lock(sensor_state_mutex);
    return sensor_state;
}

void ToolOffsetSensorNode::set_sensor_state(const SensorState &state) {
    std::lock_guard lock(sensor_state_mutex);
    sensor_state = state;
}

void ToolOffsetSensorNode::update_status(StatusTraits::Type &status) {
    auto state = get_sensor_state();
    status.ch0_active = state.ch0_active;
    status.ch1_active = state.ch1_active;
    status.sensor_fault = state.sensor_fault;
    status.sensor_errors = state.sensor_errors;
}

void ToolOffsetSensorNode::publish_data_ch0(const prusa3d_tool_offset_sensor_Data_1_0 &data) {
    data_ch0_sender.set_data(data);
}

void ToolOffsetSensorNode::publish_data_ch1(const prusa3d_tool_offset_sensor_Data_1_0 &data) {
    data_ch1_sender.set_data(data);
}

} // namespace tool_offset_sensor::cyphal
