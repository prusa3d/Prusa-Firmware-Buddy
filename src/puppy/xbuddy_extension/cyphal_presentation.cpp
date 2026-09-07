/// @file
#include "cyphal_presentation.hpp"

#include "cyphal_application.hpp"
#include "cyphal_transport.hpp"
#include <algorithm>
#include <canard.h>
#include <cstddef>
#include <freertos/timing.hpp>
#include <option/has_ac_controller.h>
#include <option/has_anfc.h>
#include <option/has_tool_offset_sensor.h>
#include <span>
#include <utils/byte_utils.hpp>
#include <string.h>
#include <uavcan/diagnostic/Record_1_1.h>
#include <uavcan/file/Read_1_1.h>
#include <uavcan/node/ExecuteCommand_1_3.h>
#include <uavcan/node/GetInfo_1_0.h>
#include <uavcan/node/Heartbeat_1_0.h>
#include <uavcan/pnp/NodeIDAllocationData_2_0.h>

#include <prusa3d/common/PortIds_0_1.h>

#if HAS_AC_CONTROLLER()
    #include <prusa3d/ac_controller/Config_1_0.h>
    #include <prusa3d/ac_controller/Status_1_0.h>
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    #include <prusa3d/tool_offset_sensor/Config_1_0.h>
    #include <prusa3d/tool_offset_sensor/Status_1_0.h>
#endif

#if HAS_ANFC()
    #include <prusa3d/nfc/command/AcceptEvent_1_0.h>
    #include <prusa3d/nfc/command/Request_1_0.h>
    #include <prusa3d/nfc/event/Event_1_0.h>
#endif

// Nunavut code generation is nice, but we would like it to be even better.
// We need to dispatch to various functions and constants solely based
// on the type of the nunavut object, which is why we use this traits approach.
// In future, maybe we could write our own generator or plugin...
template <class T>
struct NunavutTraits;

// Helper function for declaring traits.
#define TRAITS_HELPER(                                                                        \
    TransferKind,                                                                             \
    Type,                                                                                     \
    PortId,                                                                                   \
    Deserialize,                                                                              \
    ExtentBytes,                                                                              \
    Serialize,                                                                                \
    SerializationBufferSizeBytes)                                                             \
    template <>                                                                               \
    struct NunavutTraits<Type> {                                                              \
        static constexpr auto transfer_kind = TransferKind;                                   \
        static constexpr auto port_id = PortId;                                               \
        static constexpr auto deserialize = Deserialize;                                      \
        static constexpr auto extent_bytes = ExtentBytes;                                     \
        static constexpr auto serialize = Serialize;                                          \
        static constexpr auto serialization_buffer_size_bytes = SerializationBufferSizeBytes; \
    };

// Exploit regularity in nunavut generated code.
#define TRAITS(Type, TransferKind) TRAITS_HELPER( \
    TransferKind,                                 \
    Type,                                         \
    Type##_FIXED_PORT_ID_,                        \
    Type##_deserialize_,                          \
    Type##_EXTENT_BYTES_,                         \
    Type##_serialize_,                            \
    Type##_SERIALIZATION_BUFFER_SIZE_BYTES_)

// Workaround irregularity in nunavut generated code.
#define uavcan_file_Read_Request_1_1_FIXED_PORT_ID_            uavcan_file_Read_1_1_FIXED_PORT_ID_
#define uavcan_file_Read_Response_1_1_FIXED_PORT_ID_           uavcan_file_Read_1_1_FIXED_PORT_ID_
#define uavcan_node_ExecuteCommand_Request_1_3_FIXED_PORT_ID_  uavcan_node_ExecuteCommand_1_3_FIXED_PORT_ID_
#define uavcan_node_ExecuteCommand_Response_1_3_FIXED_PORT_ID_ uavcan_node_ExecuteCommand_1_3_FIXED_PORT_ID_
#define uavcan_node_GetInfo_Request_1_0_FIXED_PORT_ID_         uavcan_node_GetInfo_1_0_FIXED_PORT_ID_
#define uavcan_node_GetInfo_Response_1_0_FIXED_PORT_ID_        uavcan_node_GetInfo_1_0_FIXED_PORT_ID_
#if HAS_AC_CONTROLLER()
    #define prusa3d_ac_controller_Config_Request_1_0_FIXED_PORT_ID_ prusa3d_common_PortIds_0_1_SRV_AC_CONTROLLER_SET_CONFIG
    #define prusa3d_ac_controller_Status_1_0_FIXED_PORT_ID_         prusa3d_common_PortIds_0_1_MSG_AC_CONTROLLER_STATUS
    #define prusa3d_common_leds_Config_1_0_FIXED_PORT_ID_           prusa3d_common_PortIds_0_1_MSG_AC_CONTROLLER_LEDS
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    #define prusa3d_tool_offset_sensor_Config_Request_1_0_FIXED_PORT_ID_ prusa3d_common_PortIds_0_1_SRV_TOOL_OFFSET_SENSOR_CONFIG
    #define prusa3d_tool_offset_sensor_Status_1_0_FIXED_PORT_ID_         prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_STATUS
#endif
#if HAS_ANFC()
    #define prusa3d_nfc_command_AcceptEvent_Request_1_0_FIXED_PORT_ID_ prusa3d_common_PortIds_0_1_SRV_NFC_ACCEPT_EVENT
    #define prusa3d_nfc_command_Request_Request_1_0_FIXED_PORT_ID_     prusa3d_common_PortIds_0_1_SRV_NFC_REQUEST
    #define prusa3d_nfc_event_Event_1_0_FIXED_PORT_ID_                 prusa3d_common_PortIds_0_1_MSG_NFC_EVENT
#endif

// Define traits for all transfers we are using.
TRAITS(uavcan_diagnostic_Record_1_1, CanardTransferKindMessage)
TRAITS(uavcan_file_Read_Request_1_1, CanardTransferKindRequest)
TRAITS(uavcan_file_Read_Response_1_1, CanardTransferKindResponse)
TRAITS(uavcan_node_ExecuteCommand_Request_1_3, CanardTransferKindRequest)
TRAITS(uavcan_node_ExecuteCommand_Response_1_3, CanardTransferKindResponse)
TRAITS(uavcan_node_GetInfo_Request_1_0, CanardTransferKindRequest)
TRAITS(uavcan_node_GetInfo_Response_1_0, CanardTransferKindResponse)
TRAITS(uavcan_node_Heartbeat_1_0, CanardTransferKindMessage)
TRAITS(uavcan_pnp_NodeIDAllocationData_2_0, CanardTransferKindMessage)
#if HAS_AC_CONTROLLER()
TRAITS(prusa3d_ac_controller_Config_Request_1_0, CanardTransferKindRequest)
TRAITS(prusa3d_ac_controller_Status_1_0, CanardTransferKindMessage)
TRAITS(prusa3d_common_leds_Config_1_0, CanardTransferKindMessage)

static std::optional<float> convert(const prusa3d_common_TargetTemperature_1_0 &target_temperature) {
    if (target_temperature._tag_ == 0) {
        return {};
    } else {
        return { target_temperature.target_temp.celsius };
    }
}

static ac_controller::Faults convert(const prusa3d_common_SharedFault_1_0 &fault) {
    auto checker = [](ac_controller::Fault fault, uint8_t bit) {
        return static_cast<uint32_t>(ac_controller::Faults { fault }) == 1u << bit;
    };
    static_assert(checker(ac_controller::Faults::RCD_TRIPPED, prusa3d_ac_controller_Status_1_0_FAULT_RCD_TRIPPED));
    static_assert(checker(ac_controller::Faults::POWERPANIC, prusa3d_ac_controller_Status_1_0_FAULT_POWERPANIC));
    static_assert(checker(ac_controller::Faults::OVERHEAT, prusa3d_ac_controller_Status_1_0_FAULT_OVERHEAT));
    static_assert(checker(ac_controller::Faults::PSU_FAN_NOK, prusa3d_ac_controller_Status_1_0_FAULT_PSU_FAN_NOK));
    static_assert(checker(ac_controller::Faults::PSU_NTC_DISCONNECT, prusa3d_ac_controller_Status_1_0_FAULT_PSU_NTC_DISCONNECT));
    static_assert(checker(ac_controller::Faults::PSU_NTC_SHORT, prusa3d_ac_controller_Status_1_0_FAULT_PSU_NTC_SHORT));
    static_assert(checker(ac_controller::Faults::BED_NTC_DISCONNECT, prusa3d_ac_controller_Status_1_0_FAULT_BED_NTC_DISCONNECT));
    static_assert(checker(ac_controller::Faults::BED_NTC_SHORT, prusa3d_ac_controller_Status_1_0_FAULT_BED_NTC_SHORT));
    static_assert(checker(ac_controller::Faults::TRIAC_NTC_DISCONNECT, prusa3d_ac_controller_Status_1_0_FAULT_TRIAC_NTC_DISCONNECT));
    static_assert(checker(ac_controller::Faults::TRIAC_NTC_SHORT, prusa3d_ac_controller_Status_1_0_FAULT_TRIAC_NTC_SHORT));
    static_assert(checker(ac_controller::Faults::BED_FAN0_NOK, prusa3d_ac_controller_Status_1_0_FAULT_BED_FAN0_NOK));
    static_assert(checker(ac_controller::Faults::BED_FAN1_NOK, prusa3d_ac_controller_Status_1_0_FAULT_BED_FAN1_NOK));
    static_assert(checker(ac_controller::Faults::TRIAC_FAN_NOK, prusa3d_ac_controller_Status_1_0_FAULT_TRIAC_FAN_NOK));
    static_assert(checker(ac_controller::Faults::GRID_NOK, prusa3d_ac_controller_Status_1_0_FAULT_GRID_NOK));
    static_assert(checker(ac_controller::Faults::BED_LOAD_NOK, prusa3d_ac_controller_Status_1_0_FAULT_BED_LOAD_NOK));
    static_assert(checker(ac_controller::Faults::CHAMBER_LOAD_NOK, prusa3d_ac_controller_Status_1_0_FAULT_CHAMBER_LOAD_NOK));
    static_assert(checker(ac_controller::Faults::PSU_NOK, prusa3d_ac_controller_Status_1_0_FAULT_PSU_NOK));
    static_assert(checker(ac_controller::Faults::BED_RUNAWAY, prusa3d_ac_controller_Status_1_0_FAULT_BED_RUNAWAY));
    static_assert(checker(ac_controller::Faults::MCU_OVERHEAT, prusa3d_common_SharedFault_1_0_FAULT_MCU_OVERHEAT));
    static_assert(checker(ac_controller::Faults::PCB_OVERHEAT, prusa3d_common_SharedFault_1_0_FAULT_PCB_OVERHEAT));
    static_assert(checker(ac_controller::Faults::DATA_TIMEOUT, prusa3d_common_SharedFault_1_0_FAULT_DATA_TIMEOUT));
    static_assert(checker(ac_controller::Faults::HEARTBEAT_MISSING, prusa3d_common_SharedFault_1_0_FAULT_HEARTBEAT_MISSING));
    static_assert(checker(ac_controller::Faults::UNKNOWN, prusa3d_common_SharedFault_1_0_FAULT_UNKNOWN));

    return ac_controller::Faults { fault.bitmask };
}

static uint16_t convert(const prusa3d_common_FanState_1_0 &fan_state) {
    return static_cast<uint16_t>(fan_state.current_rpm.rpm);
}

#endif

#if HAS_TOOL_OFFSET_SENSOR()
TRAITS(prusa3d_tool_offset_sensor_Config_Request_1_0, CanardTransferKindRequest);
TRAITS(prusa3d_tool_offset_sensor_Status_1_0, CanardTransferKindMessage);
#endif

#if HAS_ANFC()
TRAITS(prusa3d_nfc_command_AcceptEvent_Request_1_0, CanardTransferKindRequest);
TRAITS(prusa3d_nfc_command_Request_Request_1_0, CanardTransferKindRequest);
TRAITS(prusa3d_nfc_event_Event_1_0, CanardTransferKindMessage);
#endif

class CyphalApp : cyphal::Presentation {
private:
    // We use distinct transfer ID for distinct outgoing transfers.
    // This reduces likelihood of collisions and aids debugging.
    struct {
        uint8_t diagnostic_record = 0;
        uint8_t node_execute_command = 0;
        uint8_t node_get_info = 0;
        uint8_t node_heartbeat = 0;
        uint8_t pnp = 0;
#if HAS_AC_CONTROLLER()
        uint8_t ac_controller_config = 0;
        uint8_t leds_config = 0;
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        uint8_t tool_offset_sensor_config = 0;
#endif
#if HAS_ANFC()
        uint8_t nfc_command_accept_event = 0;
        uint8_t nfc_command_request = 0;
#endif
    } transfer_ids;
    cyphal::Application &application = cyphal::application();

public:
    CyphalApp() {
        subscribe<uavcan_file_Read_Request_1_1>();
        subscribe<uavcan_node_ExecuteCommand_Response_1_3>();
        subscribe<uavcan_node_GetInfo_Response_1_0>();
        subscribe<uavcan_node_Heartbeat_1_0>();
        subscribe<uavcan_pnp_NodeIDAllocationData_2_0>();
        subscribe<uavcan_diagnostic_Record_1_1>();
#if HAS_AC_CONTROLLER()
        subscribe<prusa3d_ac_controller_Status_1_0>();
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        subscribe<prusa3d_tool_offset_sensor_Status_1_0>();
        // Subscribe to Data streams for bridging to XBuddy (raw, no deserialization)
        subscribe_raw(CanardTransferKindMessage, prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH0, 63);
        subscribe_raw(CanardTransferKindMessage, prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH1, 63);
#endif
#if HAS_ANFC()
        // We intentionally do not subscribe to NFC command responses here.
        // Each subscription costs RAM. Even if we subscribed to responses,
        // reporting them upstream would be too complicated.
        // Failures should be rare anyway and they will be properly handled
        // by timeout/retry mechanism on the motherboard.

        // We of course need to subscribe to broadcasted events.
        subscribe<prusa3d_nfc_event_Event_1_0>();
#endif
    }

    bool step(const cyphal::TimePoint now) {
        if (CanardRxTransfer transfer; cyphal::transport().receive(transfer)) {
            receive_transfer(now, transfer);
            cyphal::transport().dispose(transfer);
            return true;
        }
        return application.step(*this, now);
    }

private:
    template <class T>
    void subscribe() {
        const CanardTransferKind transfer_kind = NunavutTraits<T>::transfer_kind;
        const CanardPortID port_id = NunavutTraits<T>::port_id;
        const size_t extent = NunavutTraits<T>::extent_bytes;
        if (!cyphal::transport().subscribe(transfer_kind, port_id, extent)) {
            // we are only subscribing at startup, so we may just abort() here
            abort();
        }
    }

    void subscribe_raw(CanardTransferKind kind, CanardPortID port_id, size_t extent) {
        if (!cyphal::transport().subscribe(kind, port_id, extent)) {
            abort();
        }
    }

    /// Following functions dispatch and deserialize incoming transfer.

    void receive_transfer(const cyphal::TimePoint now, CanardRxTransfer &transfer) {
        switch (transfer.metadata.transfer_kind) {
        case CanardTransferKindMessage:
            switch (transfer.metadata.port_id) {
            case uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_:
                return receive_helper<uavcan_node_Heartbeat_1_0>(now, transfer);
            case uavcan_pnp_NodeIDAllocationData_2_0_FIXED_PORT_ID_:
                return receive_helper<uavcan_pnp_NodeIDAllocationData_2_0>(now, transfer);
            case uavcan_diagnostic_Record_1_1_FIXED_PORT_ID_:
                return receive_helper<uavcan_diagnostic_Record_1_1>(now, transfer);
#if HAS_AC_CONTROLLER()
            case prusa3d_ac_controller_Status_1_0_FIXED_PORT_ID_:
                return receive_helper<prusa3d_ac_controller_Status_1_0>(now, transfer);
#endif
#if HAS_TOOL_OFFSET_SENSOR()
            case prusa3d_tool_offset_sensor_Status_1_0_FIXED_PORT_ID_:
                return receive_helper<prusa3d_tool_offset_sensor_Status_1_0>(now, transfer);
            case prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH0:
            case prusa3d_common_PortIds_0_1_MSG_TOOL_OFFSET_SENSOR_DATA_CH1:
                return application.bridge_queue().enqueue(
                    transfer.metadata.port_id,
                    transfer.payload,
                    static_cast<uint8_t>(transfer.payload_size));
#endif
#if HAS_ANFC()
            case prusa3d_nfc_event_Event_1_0_FIXED_PORT_ID_:
                return receive_helper<prusa3d_nfc_event_Event_1_0>(now, transfer);
#endif
            }
            return;
        case CanardTransferKindResponse:
            switch (transfer.metadata.port_id) {
            case uavcan_node_ExecuteCommand_Response_1_3_FIXED_PORT_ID_:
                return receive_helper<uavcan_node_ExecuteCommand_Response_1_3>(now, transfer);
            case uavcan_node_GetInfo_1_0_FIXED_PORT_ID_:
                return receive_helper<uavcan_node_GetInfo_Response_1_0>(now, transfer);
            }
            return;
        case CanardTransferKindRequest:
            switch (transfer.metadata.port_id) {
            case uavcan_file_Read_1_1_FIXED_PORT_ID_:
                return receive_helper<uavcan_file_Read_Request_1_1>(now, transfer);
            }
            return;
        }
    }

    template <class T>
    void receive_helper(const cyphal::TimePoint now, const CanardRxTransfer &transfer) {
        T message;
        size_t buffer_size_bytes = transfer.payload_size;
        if (NunavutTraits<T>::deserialize(&message, (const uint8_t *)transfer.payload, &buffer_size_bytes) == 0) {
            receive(now, transfer, message);
        } else {
            // silently ignore malformed messages
        }
    }

    // Following functions translate from presentation to application layer.
    // Application layer doesn't need to depend on deserialization code.
    // Also, not all fields are actually required by the application.

    void receive(const cyphal::TimePoint, const CanardRxTransfer &, const uavcan_pnp_NodeIDAllocationData_2_0 &message) {
        application.receive_pnp_allocation(
            cyphal::UniqueId { message.unique_id });
    }

    void receive(const cyphal::TimePoint now, const CanardRxTransfer &transfer, const uavcan_node_Heartbeat_1_0 &message) {
        application.receive_node_heartbeat(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            now,
            cyphal::Heartbeat {
                .health = static_cast<cyphal::Health>(message.health.value),
                .mode = static_cast<cyphal::Mode>(message.mode.value),
                .vendor_specific_status_code = message.vendor_specific_status_code,
            });
    }

    void receive(const cyphal::TimePoint, const CanardRxTransfer &transfer, const uavcan_node_ExecuteCommand_Response_1_3 &response) {
        application.receive_node_execute_command_response(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            response.status,
            as_bytes(std::span { response.output.elements, response.output.count }));
    }

    void receive(const cyphal::TimePoint, const CanardRxTransfer &transfer, const uavcan_node_GetInfo_Response_1_0 &response) {
        application.receive_node_get_info_response(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            as_bytes(std::span { response.name.elements, response.name.count }));
    }

    void receive(const cyphal::TimePoint now, const CanardRxTransfer &transfer, const uavcan_file_Read_Request_1_1 &request) {
        application.receive_file_read_request(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            now,
            cyphal::TransferId { transfer.metadata.transfer_id },
            request.offset);
    }

    void receive(const cyphal::TimePoint, const CanardRxTransfer &transfer, const uavcan_diagnostic_Record_1_1 &message) {
        application.receive_diagnostic_record(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            Bytes { (std::byte *)message.text.elements, message.text.count });
    }

#if HAS_AC_CONTROLLER()
    void receive(const cyphal::TimePoint, const CanardRxTransfer &, const prusa3d_ac_controller_Status_1_0 &message) {
        application.receive_ac_controller_status(
            ac_controller::Config {
                convert(message.bed_target_temp),
                message.external_fan0_state.current_pwm.pwm,
                message.psu_fan_state.current_pwm.pwm,
            },
            ac_controller::Status {
                .mcu_temp = message.mcu_temp.celsius,
                .bed_temp = message.bed_temp.celsius,
                .bed_voltage = message.ac_voltage.volt,
                .bed_fan_rpm = {
                    convert(message.external_fan0_state),
                    convert(message.external_fan1_state),
                },
                .psu_fan_rpm = convert(message.psu_fan_state),
                .faults = convert(message.faults),
            });
    }
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    void receive(const cyphal::TimePoint, const CanardRxTransfer &, const prusa3d_tool_offset_sensor_Status_1_0 &message) {
        application.receive_tool_offset_sensor_status(
            tool_offset_sensor::Status {
                .ch0_active = message.ch0_active,
                .ch1_active = message.ch1_active,
                .sensor_fault = message.sensor_fault,
                .sensor_errors = message.sensor_errors,
            });
    }
#endif

#if HAS_ANFC()
    void receive(const cyphal::TimePoint, const CanardRxTransfer &transfer, const prusa3d_nfc_event_Event_1_0 &) {
        application.receive_nfc_event(
            cyphal::NodeId { transfer.metadata.remote_node_id },
            Bytes {
                (const std::byte *)transfer.payload,
                transfer.payload_size });
    }
#endif

    static constexpr cyphal::NodeId anonymous {};
    static_assert(anonymous == cyphal::NodeId { CANARD_NODE_ID_UNSET });
    static_assert(cyphal::TransferId::mask == CANARD_TRANSFER_ID_MAX);

    // Following functions translate from application layer to presentation layer.

    void transmit_heartbeat(uint32_t uptime, bool healthy) override {
        uavcan_node_Heartbeat_1_0 message;
        message.uptime = uptime;
        message.health.value = healthy ? uavcan_node_Health_1_0_NOMINAL : uavcan_node_Health_1_0_WARNING;
        message.mode.value = uavcan_node_Mode_1_0_OPERATIONAL;
        message.vendor_specific_status_code = 0;
        const cyphal::TransferId transfer_id { transfer_ids.node_heartbeat++ };
        (void)serialize_and_transmit(message, anonymous, transfer_id);
    }

    void transmit_node_get_info_request(cyphal::NodeId remote_node_id) override {
        uavcan_node_GetInfo_Request_1_0 request;
        const cyphal::TransferId transfer_id { transfer_ids.node_get_info++ };
        (void)serialize_and_transmit(request, remote_node_id, transfer_id);
    }

    void transmit_node_execute_command_request(cyphal::NodeId remote_node_id, cyphal::Command command, Bytes parameter) override {
        uavcan_node_ExecuteCommand_Request_1_3 request;
        request.command = static_cast<uint16_t>(command);
        request.parameter.count = std::min(parameter.size(), sizeof(request.parameter.elements));
        memcpy(request.parameter.elements, parameter.data(), request.parameter.count);
        const cyphal::TransferId transfer_id { transfer_ids.node_execute_command++ };
        (void)serialize_and_transmit(request, remote_node_id, transfer_id);
    }

    void transmit_diagnostic_record(cyphal::Severity severity, const char *text) override {
        uavcan_diagnostic_Record_1_1 record;
        record.severity.value = static_cast<uint8_t>(severity);
        record.text.count = std::min(strlen(text), sizeof(record.text.elements));
        memcpy(record.text.elements, text, record.text.count);
        const cyphal::TransferId transfer_id { transfer_ids.diagnostic_record++ };
        if (!serialize_and_transmit(record, anonymous, transfer_id)) {
            // Failure to send diagnostic record is not an error.
            // It hurts debugging experience but we can continue as if nothing happened.
        }
    }

    void transmit_pnp_allocation(const cyphal::UniqueId &unique_id, cyphal::NodeId node_id) override {
        uavcan_pnp_NodeIDAllocationData_2_0 message;
        static_assert(unique_id.size() == sizeof(message.unique_id));
        memcpy(message.unique_id, unique_id.data(), sizeof(message.unique_id));
        message.node_id.value = (uint8_t)node_id;
        const cyphal::TransferId transfer_id { transfer_ids.pnp++ };
        if (!serialize_and_transmit(message, anonymous, transfer_id)) {
            // Failure to send allocation response is not an error.
            // Node will retry later and we will hopefully be able to respond then.
        }
    }
    void transmit_file_read_response(cyphal::NodeId remote_node_id, cyphal::TransferId transfer_id, WritableBytes data) override {
        uavcan_file_Read_Response_1_1 response;
        response._error.value = uavcan_file_Error_1_0_OK;
        response.data.value.count = data.size();
        memcpy(response.data.value.elements, data.data(), response.data.value.count);
        (void)serialize_and_transmit(response, remote_node_id, transfer_id);
    }

    void transmit_ac_controller_config_request(cyphal::NodeId remote_node_id, const ac_controller::Config &r) override {
#if HAS_AC_CONTROLLER()
        prusa3d_ac_controller_Config_Request_1_0 request;
        memset(&request, 0, sizeof(request));
        if (r.bed_target_temp.has_value()) {
            request.bed_target_temp._tag_ = 1;
            request.bed_target_temp.target_temp.celsius = *r.bed_target_temp;
        } else {
            request.bed_target_temp._tag_ = 0;
        }

        if (r.bed_fan_pwm.has_value()) {
            request.external_fan_config._tag_ = 1;
            request.external_fan_config.pwm.pwm = r.bed_fan_pwm.value() / 255.f;
        } else {
            request.external_fan_config._tag_ = 0;
        }

        if (r.psu_fan_pwm.has_value()) {
            request.psu_fan_config._tag_ = 1;
            request.psu_fan_config.pwm.pwm = r.psu_fan_pwm.value() / 255.f;
        } else {
            request.psu_fan_config._tag_ = 0;
        }

        const cyphal::TransferId transfer_id { transfer_ids.ac_controller_config++ };
        (void)serialize_and_transmit(request, remote_node_id, transfer_id);
#else
        (void)remote_node_id;
        (void)r;
#endif
    }

    void transmit_ac_controller_leds_config_request([[maybe_unused]] cyphal::NodeId remote_node_id, const ac_controller::LedConfig &r) override {
#if HAS_AC_CONTROLLER()
        prusa3d_common_leds_Config_1_0 request;

        // If we have progress, send progress mode (which includes color)
        if (r.progress_percent.has_value()) {
            request._tag_ = 2; // progress mode
            request.progress.progress = static_cast<float>(r.progress_percent.value()) / 100.0f; // Convert percent to [0, 1]
            // Set color for progress bar (default to black if not specified)
            if (r.color.has_value()) {
                request.progress.color.red = r.color->r;
                request.progress.color.green = r.color->g;
                request.progress.color.blue = r.color->b;
                request.progress.color.white = r.color->w;
            } else {
                request.progress.color.red = 0;
                request.progress.color.green = 0;
                request.progress.color.blue = 0;
                request.progress.color.white = 0;
            }
        } else if (r.color.has_value()) {
            // Static color mode (no progress)
            request._tag_ = 1;
            request.color.red = r.color->r;
            request.color.green = r.color->g;
            request.color.blue = r.color->b;
            request.color.white = r.color->w;
        } else {
            // Off mode
            request._tag_ = 0;
        }

        // LED config is a broadcast message (CanardTransferKindMessage), not a request
        // Therefore we must use CANARD_NODE_ID_UNSET instead of a specific node ID
        const cyphal::TransferId transfer_id { transfer_ids.leds_config++ };
        (void)serialize_and_transmit(request, anonymous, transfer_id);
#else
        (void)remote_node_id;
        (void)r;
#endif
    }

    void transmit_tool_offset_sensor_config_request(cyphal::NodeId remote_node_id, const tool_offset_sensor::Config &r) override {
#if HAS_TOOL_OFFSET_SENSOR()
        prusa3d_tool_offset_sensor_Config_Request_1_0 request;
        std::memset(&request, 0, sizeof(request));
        request.ch0_enabled = r.ch0_enabled.value_or(false);
        request.ch1_enabled = r.ch1_enabled.value_or(false);
        const cyphal::TransferId transfer_id { transfer_ids.tool_offset_sensor_config++ };
        (void)serialize_and_transmit(request, remote_node_id, transfer_id);
#else
        (void)remote_node_id;
        (void)r;
#endif
    }

    bool transmit_nfc_command_request(cyphal::NodeId remote_node_id, Bytes data) override {
#if HAS_ANFC()
        const cyphal::TransferId transfer_id { transfer_ids.nfc_command_request++ };
        return transmit<prusa3d_nfc_command_Request_Request_1_0>(data, remote_node_id, transfer_id);
#else
        (void)remote_node_id;
        (void)data;
        return false;
#endif
    }

    bool transmit_nfc_command_accept_event(cyphal::NodeId remote_node_id, Bytes data) override {
#if HAS_ANFC()
        const cyphal::TransferId transfer_id { transfer_ids.nfc_command_accept_event++ };
        return transmit<prusa3d_nfc_command_AcceptEvent_Request_1_0>(data, remote_node_id, transfer_id);
#else
        (void)remote_node_id;
        (void)data;
        return false;
#endif
    }

    template <class T>
    [[nodiscard]] bool serialize_and_transmit(const T &object, cyphal::NodeId remote_node_id, cyphal::TransferId transfer_id) {
        std::byte buffer[NunavutTraits<T>::serialization_buffer_size_bytes];
        size_t size = sizeof(buffer);
        if (NunavutTraits<T>::serialize(&object, (uint8_t *)buffer, &size) == 0) {
            return transmit<T>({ buffer, size }, remote_node_id, transfer_id);
        } else {
            return false;
        }
    }

    template <class T>
    [[nodiscard]] bool transmit(const Bytes &data, cyphal::NodeId remote_node_id, cyphal::TransferId transfer_id) {
        const CanardTransferMetadata metadata = {
            .priority = CanardPriorityNominal,
            .transfer_kind = NunavutTraits<T>::transfer_kind,
            .port_id = NunavutTraits<T>::port_id,
            .remote_node_id = (uint8_t)remote_node_id,
            .transfer_id = (uint8_t)transfer_id,
        };
        const CanardMicrosecond deadline = 0; // unlimited
        return cyphal::transport().transmit(deadline, metadata, data);
    }
};

void cyphal::run_for_a_while() {
    static CyphalApp cyphal_app;
    const auto now = TimePoint::clock::now();
    while (cyphal_app.step(now)) {
    }
}
