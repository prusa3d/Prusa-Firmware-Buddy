#define DO_NOT_CHECK_ATOMIC_LOCK_FREE

#include "cyphal_anfc_node.hpp"

#include <option/can_bus_type.h>

#include <bsod.h>
#include <cyphal_pnp.hpp>

#include <nfc.hpp>
#include <hal.hpp>
#include <adc.hpp>
#include <freertos/timing.hpp>
#include <main.hpp>
#include <option/nfc_board_has_filament_sensors.h>

#include <prusa3d/nfc/event/Event_1_0.h>
#include <prusa3d/common/CustomExecuteCommand_1_0.h>
#include <prusa3d/common/SharedFault_1_0.h>

namespace anfc::cyphal {

using namespace can::cyphal;

ANFCNode::ANFCNode(const UID &uid)
    : Node(uid.data(),
#if CAN_BUS_TYPE_IS_PUB6() || CAN_BUS_TYPE_IS_UART()
        "cz.prusa3d.honeybee.nfc"
#elif CAN_BUS_TYPE_IS_SLX()
        "cz.prusa3d.slx.nfc"
#else
    #error
#endif
        )
    , request_server {
        [this](const auto &data, [[maybe_unused]] const ProtoSuber::Meta &meta) {
            prusa3d_nfc_command_Request_Response_1_0 response;
            response.status = nfc_task.enqueue_serialized_request(std::span<const uint8_t>(data.serialized_data.data(), data.serialized_size)) ? prusa3d_nfc_command_Request_Response_1_0_OK : prusa3d_nfc_command_Request_Response_1_0_REQUEST_QUEUE_FULL,
            request_server.send_response(response);
        },
        ProtoSender::send_timeout_default,
        ProtoSuber::multipart_timeout_default,
    } //
{
}

void ANFCNode::app_init() {
    const auto setup_server = [&](ServerVoid &server, const char *port_name, const char *data_type) {
        port_list.add(server);
        server.add_to_task();
        registers.add_port_name_set(port_name, data_type, server.get_port_id());
    };

    setup_server(request_server, "srv.request", prusa3d_nfc_command_Request_1_0_FULL_NAME_AND_VERSION_);
    event_publisher.init(port_list, registers);

    debug_assert(registers.get_max_registers() == registers.get_register_count());
}

void ANFCNode::app_tick(int64_t now_us) {
    event_publisher.step();
    hal::set_status_led((now_us / (1024 * 1024)) % 2);
}

void ANFCNode::app_tick_pnp(int64_t now_us) {
    hal::set_status_led((now_us / (128 * 1024)) % 2);
}

void ANFCNode::write_config([[maybe_unused]] const ConfigTraits::Request::Type &config) {
#if NFC_BOARD_HAS_FILAMENT_SENSORS()
    hal::set_fs_led_l_pwm(config.left_led.pwm);
    hal::set_fs_led_r_pwm(config.right_led.pwm);
#endif
}

void ANFCNode::update_status(StatusTraits::Type &data) {
    static_assert(std::to_underlying(hal::BoardOrientation::normal) == prusa3d_nfc_BoardOrientation_1_0_NORMAL);
    static_assert(std::to_underlying(hal::BoardOrientation::left) == prusa3d_nfc_BoardOrientation_1_0_LEFT);
    static_assert(std::to_underlying(hal::BoardOrientation::right) == prusa3d_nfc_BoardOrientation_1_0_RIGHT);
    data.board_orientation.value = std::to_underlying(hal::get_board_orientation());

    const auto mcu_temp = adc::get_mcu_temperature();
    data.board_temp.celsius = adc::get_board_temperature();
    data.mcu_temp.celsius = mcu_temp;

    if (mcu_temp >= 90.0f) {
        data.faults.bitmask |= puppy::fault::SharedFault::mcu_overheat;
    }

    data.fs_1.adc_raw = adc::impl::get_raw(adc::Channel::fs_1).to_raw();
    data.fs_2.adc_raw = adc::impl::get_raw(adc::Channel::fs_2).to_raw();

    data.radio_enabled = nfc_task.is_radio_enabled();
}

} // namespace anfc::cyphal
