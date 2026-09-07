/// @file
#pragma once

#include "cyphal_types.hpp"
#include <ac_controller/types.hpp>
#include <tool_offset_sensor/types.hpp>
#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>

namespace cyphal {

class Presentation {
public:
    virtual void transmit_heartbeat(uint32_t uptime, bool healthy) = 0;
    virtual void transmit_pnp_allocation(const UniqueId &unique_id, NodeId node_id) = 0;
    virtual void transmit_diagnostic_record(Severity, const char *text) = 0;
    virtual void transmit_node_get_info_request(NodeId remote_node_id) = 0;
    virtual void transmit_node_execute_command_request(NodeId remote_node_id, Command, Bytes) = 0;
    virtual void transmit_file_read_response(NodeId remote_node_id, TransferId transfer_id, WritableBytes data) = 0;
    virtual void transmit_ac_controller_config_request(NodeId remote_node_id, const ac_controller::Config &) = 0;
    virtual void transmit_ac_controller_leds_config_request(cyphal::NodeId remote_node_id, const ac_controller::LedConfig &r) = 0;
    virtual void transmit_tool_offset_sensor_config_request(NodeId remote_node_id, const tool_offset_sensor::Config &) = 0;

    /// Transmit prusa3d.nfc.command.Request.1
    /// Caller is responsible for properly serializing the message.
    [[nodiscard]] virtual bool transmit_nfc_command_request(NodeId remote_node_id, Bytes) = 0;

    /// Transmit prusa3d.nfc.command.AcceptEvent.1
    /// Caller is responsible for properly serializing the message.
    [[nodiscard]] virtual bool transmit_nfc_command_accept_event(NodeId remote_node_id, Bytes) = 0;

    constexpr auto operator<=>(const Presentation &) const = default;
};

} // namespace cyphal
