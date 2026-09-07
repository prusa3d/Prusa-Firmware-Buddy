/// @file
#include "app.hpp"

#include "cyphal_application.hpp"
#include "option/extension_variant.h"
#include "hal.hpp"
#include "hal_mmu_port.hpp"
#include "hal_rs485.hpp"
#include "hal_usb.hpp"
#include "master_activity.hpp"
#include <modbus/modbus.hpp>
#include "temperature.hpp"
#include <ac_controller/modbus.hpp>
#include <anfc/modbus.hpp>
#include <cstdlib>
#include <span>
#include <modbus/traits.hpp>
#include <option/has_ac_controller.h>
#include <option/has_anfc.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_mmu2.h>
#include <xbuddy_extension/modbus.hpp>

#if HAS_TOOL_OFFSET_SENSOR()
    #include <tool_offset_sensor/modbus.hpp>
#endif

#if HAS_MMU2()
    #include "hal_mmu.hpp"
    #include "mmu.hpp"
#endif

namespace {

void read_register_file_callback(xbuddy_extension::modbus::Status &status) {
    status.fan_rpm[0] = hal::fan1::get_rpm();
    status.fan_rpm[1] = hal::fan2::get_rpm();
    status.fan_rpm[2] = hal::fan3::get_rpm();
    // Note: Mainboard expects this in decidegree Celsius.
    status.temperature = static_cast<uint16_t>(10 * temperature::raw_to_celsius(hal::temperature::get_raw()));
    status.gpio_filament_sensor = static_cast<uint16_t>(hal::filament_sensor::get_gpio());

    {
        // Pack all filament sensors into a single register
        using Register = decltype(status.gpio_filament_sensor);
        static_assert(xbuddy_extension::ext_filament_sensor_count * xbuddy_extension::bits_per_fs_state <= sizeof(Register) * 8);

        Register ext_fsensors = 0;
        for (uint8_t i = 0; i < xbuddy_extension::ext_filament_sensor_count; i++) {
            ext_fsensors |= static_cast<Register>(hal::filament_sensor::get_ext(i)) << (i * xbuddy_extension::bits_per_fs_state);
        }
        status.ext_filament_sensors = ext_fsensors;
    }

    const auto flash_data = cyphal::application().request();
    status.chunk_request.file_id = xbuddy_extension::modbus::serialize_file_id(flash_data.flash_request);
    status.chunk_request.offset_lo = static_cast<uint16_t>(flash_data.offset & 0xFFFF);
    status.chunk_request.offset_hi = static_cast<uint16_t>(flash_data.offset >> 16);
    status.digest_request.file_id = xbuddy_extension::modbus::serialize_file_id(flash_data.hash_request);
    status.digest_request.salt_lo = static_cast<uint16_t>(flash_data.hash_salt & 0xFFFF);
    status.digest_request.salt_hi = static_cast<uint16_t>(flash_data.hash_salt >> 16);
    const auto log = cyphal::application().get_log();
    status.log_message_sequence = log.sequence;

    // The read callback fills the raw Modbus response buffer, so every field
    // must be written on every variant — zero where the peripheral is absent.
#if EXTENSION_IS_XL_CAN()
    status.fan_power_fault = static_cast<uint16_t>(hal::fan_power::fault_pin_get());
#else
    status.fan_power_fault = 0;
#endif
}

void read_register_file_callback(xbuddy_extension::modbus::CyphalBridge &bridge) {
    static_assert(std::endian::native == std::endian::little);
    auto bytes = std::as_writable_bytes(std::span { bridge.data });
    bridge.size = static_cast<uint16_t>(cyphal::application().bridge_queue().read_into(bytes.data(), bytes.size()));
    bridge.bytes_available = static_cast<uint16_t>(cyphal::application().bridge_queue().bytes_available());
}

void read_register_file_callback(xbuddy_extension::modbus::LogMessage &log_message) {
    static_assert(std::endian::native == std::endian::little);
    const auto log = cyphal::application().get_log();
    const auto text_size = std::min(sizeof(log_message.text_data), log.text.size());
    log_message.sequence = log.sequence;
    log_message.text_size = text_size;
    memcpy(log_message.text_data.data(), log.text.data(), text_size);
}

bool write_register_file_callback(const xbuddy_extension::modbus::Config &config) {
    hal::fan1::set_pwm(config.fan_pwm[0]);
    hal::fan2::set_pwm(config.fan_pwm[1]);
    hal::fan3::set_pwm(config.fan_pwm[2]);
#if EXTENSION_IS_XL_CAN()
    // The bridge's fan 5 V rail has a dedicated power switch (the "fully
    // disable" mechanism per Electro); follow the commanded duty so PWM 0
    // actually cuts power instead of leaving a stopped-but-powered 4-wire fan.
    hal::fan_power::enable_pin_set(config.fan_pwm[xbuddy_extension::modbus::XL_CAN_FAN_IDX] > 0);
#endif
#if PA6_PIN_DRIVES_W_LED()
    hal::w_led::set_pwm(config.w_led_pwm);
    // Technically, this frequency is common also for some fans. But they seem to work fine.
    hal::w_led::set_frequency(config.w_led_frequency);
#endif
    hal::rgbw_led::set_r_pwm(config.rgbw_led_r_pwm);
    hal::rgbw_led::set_g_pwm(config.rgbw_led_g_pwm);
    hal::rgbw_led::set_b_pwm(config.rgbw_led_b_pwm);
    hal::rgbw_led::set_w_pwm(config.rgbw_led_w_pwm);
#if !EXTENSION_IS_IX()
    hal::usb::power_pin_set(static_cast<bool>(config.usb_power));
#endif
#if HAS_MMU_POWER_PIN()
    hal::mmu_port::power_pin_set(static_cast<bool>(config.mmu_power));
#endif
    hal::mmu_port::nreset_pin_set(static_cast<bool>(config.mmu_nreset));
    // Master's activity. Value that should be changing regularly.
    // If it doesn't change from time to time, it means the master is not properly alive.
    master_activity.store(config.activity, std::memory_order_relaxed);
    return true;
}

bool write_register_file_callback(const xbuddy_extension::modbus::Digest &hash) {
    const auto salt = (uint32_t)hash.request.salt_hi << 16 | (uint32_t)hash.request.salt_lo;
    const auto file = (cyphal::FirmwareFile)hash.request.file_id;
    const auto status = xbuddy_extension::modbus::parse_digest_status(hash.status);
    return cyphal::application().receive_digest(file, salt, status, std::as_bytes(std::span { hash.data }));
}

bool write_register_file_callback(const xbuddy_extension::modbus::Chunk &chunk) {
    const bool last = chunk.size != chunk.data.size() * 2;
    const uint16_t file_id = chunk.request.file_id;
    const uint32_t offset = (uint32_t)chunk.request.offset_hi << 16 | (uint32_t)chunk.request.offset_lo;
    const uint8_t *data_ptr = reinterpret_cast<const uint8_t *>(chunk.data.data());
    return cyphal::application().receive_chunk(data_ptr, chunk.size, last, file_id, offset);
}

#if HAS_AC_CONTROLLER()

void read_register_file_callback(ac_controller::modbus::Status &modbus_status) {
    xbuddy_extension::NodeState node_state;
    ac_controller::Status status;
    cyphal::application().request_ac_controller(node_state, status);

    modbus_status.mcu_temp = static_cast<uint16_t>(status.mcu_temp * 10);
    modbus_status.bed_temp = static_cast<uint16_t>(status.bed_temp * 10);
    modbus_status.bed_voltage = static_cast<uint16_t>(status.bed_voltage * 10);
    modbus_status.bed_fan_rpm = status.bed_fan_rpm;
    modbus_status.psu_fan_rpm = status.psu_fan_rpm;
    const auto faults = static_cast<uint32_t>(status.faults);
    modbus_status.faults_lo = faults & 0xFFFF;
    modbus_status.faults_hi = (faults >> 16) & 0xFFFF;
    modbus_status.node_state = static_cast<uint16_t>(node_state);
}

static std::optional<float> modbus_parse_target_temperature(uint16_t temp) {
    return temp ? std::optional<float> { 0.1f * temp } : std::nullopt;
}

static std::optional<uint8_t> modbus_parse_pwm(uint16_t pwm) {
    return pwm ? std::optional<uint8_t> { pwm } : std::nullopt;
}

bool write_register_file_callback(const ac_controller::modbus::Config &modbus_config) {
    return cyphal::application().receive(ac_controller::Config {
        .bed_target_temp = modbus_parse_target_temperature(modbus_config.bed_target_temp),
        .bed_fan_pwm = modbus_parse_pwm(modbus_config.bed_fan_pwm),
        .psu_fan_pwm = modbus_parse_pwm(modbus_config.psu_fan_pwm) });
}

static ac_controller::AnimationType modbus_parse_animation_type(uint16_t animation_type) {
    if (animation_type > static_cast<uint16_t>(ac_controller::AnimationType::_last)) {
        cyphal::application().log_from_app("ERROR: XBE: Invalid animation type received");
        return ac_controller::AnimationType::OFF;
    }
    return static_cast<ac_controller::AnimationType>(animation_type);
}

static std::optional<uint8_t> modbus_parse_progress(uint16_t progress) {
    if (progress > 100) {
        cyphal::application().log_from_app("ERROR: XBE: Invalid progress percent received");
        return std::nullopt;
    }
    return std::optional<uint8_t> { progress };
}

bool write_register_file_callback(const ac_controller::modbus::LedConfig &modbus_led_config) {
    ac_controller::AnimationType animation_type = modbus_parse_animation_type(modbus_led_config.animation_type);
    std::optional<uint8_t> progress_percent = std::nullopt;
    std::optional<ac_controller::ColorRGBW> color = std::nullopt;

    switch (animation_type) {
    case ac_controller::AnimationType::PROGRESS_PERCENT:
        progress_percent = modbus_parse_progress(modbus_led_config.progress_percent);
        color = ac_controller::ColorRGBW {
            static_cast<uint8_t>(modbus_led_config.led_r),
            static_cast<uint8_t>(modbus_led_config.led_g),
            static_cast<uint8_t>(modbus_led_config.led_b),
            static_cast<uint8_t>(modbus_led_config.led_w),
        };
        break;
    case ac_controller::AnimationType::STATIC_COLOR:
        color = ac_controller::ColorRGBW {
            static_cast<uint8_t>(modbus_led_config.led_r),
            static_cast<uint8_t>(modbus_led_config.led_g),
            static_cast<uint8_t>(modbus_led_config.led_b),
            static_cast<uint8_t>(modbus_led_config.led_w),
        };
        break;
    case ac_controller::AnimationType::OFF:
        break;
    default:
        [[unlikely]] abort(); // ERROR: Invalid animation type, already handled in modbus_parse_animation_type
        break;
    }

    return cyphal::application().receive(ac_controller::LedConfig {
        .color = color,
        .progress_percent = progress_percent,
        .animation_type = std::optional<ac_controller::AnimationType>(animation_type),
    });
}

#endif

#if HAS_TOOL_OFFSET_SENSOR()

void read_register_file_callback(tool_offset_sensor::modbus::Status &modbus_status) {
    xbuddy_extension::NodeState node_state;
    tool_offset_sensor::Status status;
    cyphal::application().request_tool_offset_sensor(node_state, status);
    modbus_status.node_state = static_cast<uint16_t>(node_state);
    using namespace tool_offset_sensor::modbus;
    modbus_status.channel_flags = (status.ch0_active ? channel_flag_ch0_active : 0)
        | (status.ch1_active ? channel_flag_ch1_active : 0)
        | (status.sensor_fault ? channel_flag_sensor_fault : 0);
    modbus_status.sensor_errors = status.sensor_errors;
}

bool write_register_file_callback(const tool_offset_sensor::modbus::Config &modbus_config) {
    return cyphal::application().receive(tool_offset_sensor::Config {
        .ch0_enabled = static_cast<bool>(modbus_config.ch0_enabled),
        .ch1_enabled = static_cast<bool>(modbus_config.ch1_enabled) });
}
#endif

#if HAS_ANFC()

void read_register_file_callback(anfc::modbus::Event &event, anfc::Device device) {
    cyphal::application().get_nfc(device).consume(event);
}

[[nodiscard]] bool write_register_file_callback(const anfc::modbus::AcceptEvent &accept_event, anfc::Device device) {
    return cyphal::application().get_nfc(device).queue(accept_event);
}

[[nodiscard]] bool write_register_file_callback(const anfc::modbus::Request &request, anfc::Device device) {
    return cyphal::application().get_nfc(device).queue(request);
}

#endif

using Status = modbus::Callbacks::Status;

/// Read a register from a struct mapped to Modbus address space.
template <modbus::RegisterFile RF, class... Args>
Status read_single_register_file(uint16_t address, std::span<uint16_t> out, Args &&...args) {
    if (RF::address == address && out.size() == sizeof(RF) / 2) {
        read_register_file_callback(*reinterpret_cast<RF *>(out.data()), std::forward<Args>(args)...);
        return Status::Ok;
    } else {
        return Status::IllegalAddress;
    }
}

/// Read registers from structs mapped to Modbus address space.
template <modbus::RegisterFile... RFs, class... Args>
Status read_register_file(uint16_t address, std::span<uint16_t> out, Args &&...args) {
    Status result = Status::IllegalAddress;
    (void)((result = read_single_register_file<RFs>(address, out, std::forward<Args>(args)...), result != Status::IllegalAddress) || ...);
    return result;
}

/// Write a register to a struct mapped to Modbus address space.
template <modbus::RegisterFile RF, class... Args>
Status write_single_register_file(uint16_t address, std::span<const uint16_t> in, Args &&...args) {
    if (RF::address == address && in.size() == sizeof(RF) / 2) {
        return write_register_file_callback(*reinterpret_cast<const RF *>(in.data()), std::forward<Args>(args)...) ? Status::Ok : Status::SlaveDeviceFailure;
    } else {
        return Status::IllegalAddress;
    }
}

/// Write registers to structs mapped to Modbus address space.
template <modbus::RegisterFile... RFs, class... Args>
Status write_register_file(uint16_t address, std::span<const uint16_t> in, Args &&...args) {
    Status result = Status::IllegalAddress;
    (void)((result = write_single_register_file<RFs>(address, in, std::forward<Args>(args)...), result != Status::IllegalAddress) || ...);
    return result;
}

#if HAS_AC_CONTROLLER()

class AcController final : public modbus::Callbacks {
public:
    modbus::ServerAddress server_address() const final { return modbus::ServerAddress::ac_controller; }

    Status read_registers(uint16_t address, std::span<uint16_t> out) final {
        return read_register_file<ac_controller::modbus::Status>(address, out);
    }

    Status write_registers(uint16_t address, std::span<const uint16_t> in) final {
        return write_register_file<ac_controller::modbus::Config, ac_controller::modbus::LedConfig>(address, in);
    }
};
#endif

#if HAS_TOOL_OFFSET_SENSOR()

class ToolOffsetSensor final : public modbus::Callbacks {
public:
    modbus::ServerAddress server_address() const final { return modbus::ServerAddress::tool_offset_sensor; }

    Status read_registers(uint16_t address, std::span<uint16_t> out) final {
        return read_register_file<tool_offset_sensor::modbus::Status>(address, out);
    }

    Status write_registers(uint16_t address, std::span<const uint16_t> in) final {
        return write_register_file<tool_offset_sensor::modbus::Config>(address, in);
    }
};
#endif

#if HAS_ANFC()

class ANfc final : public modbus::Callbacks {
private:
    anfc::Device device;

public:
    explicit ANfc(anfc::Device device)
        : device { device } {}

    modbus::ServerAddress server_address() const final {
        return anfc::modbus::server_address(device);
    }

    Status read_registers(uint16_t address, std::span<uint16_t> out) final {
        return read_register_file<anfc::modbus::Event>(address, out, device);
    }

    Status write_registers(uint16_t address, std::span<const uint16_t> in) final {
        return write_register_file<anfc::modbus::AcceptEvent, anfc::modbus::Request>(address, in, device);
    }
};

#endif

class Logic final : public modbus::Callbacks {
public:
    modbus::ServerAddress server_address() const final {
#if EXTENSION_IS_XL_CAN()
        return modbus::ServerAddress::xl_can;
#else
        return modbus::ServerAddress::xbuddy_extension;
#endif
    }

    Status read_registers(const uint16_t address, std::span<uint16_t> out) final {
        return read_register_file<xbuddy_extension::modbus::Status, xbuddy_extension::modbus::LogMessage, xbuddy_extension::modbus::CyphalBridge>(address, out);
    }

    Status write_registers(const uint16_t address, std::span<const uint16_t> in) final {
        return write_register_file<xbuddy_extension::modbus::Config, xbuddy_extension::modbus::Chunk, xbuddy_extension::modbus::Digest>(address, in);
    }
};

} // namespace

void app::run() {
    Logic logic;
#if HAS_AC_CONTROLLER()
    AcController ac_controller;
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    ToolOffsetSensor tool_offset_sensor;
#endif
#if HAS_ANFC()
    ANfc anfc0 { anfc::Device::anfc0 };
    ANfc anfc1 { anfc::Device::anfc1 };
#endif
#if HAS_MMU2()
    MMU mmu;
#endif

    auto devices = std::to_array<modbus::Callbacks *>({
        &logic,
#if HAS_AC_CONTROLLER()
            &ac_controller,
#endif
#if HAS_TOOL_OFFSET_SENSOR()
            &tool_offset_sensor,
#endif
#if HAS_ANFC()
            &anfc0,
            &anfc1,
#endif
#if HAS_MMU2()
            &mmu,
#endif
    });
    modbus::Dispatch modbus_dispatch { devices };

    alignas(uint16_t) std::byte response_buffer[256];
    hal::rs485::start_receiving();
    for (;;) {
        const auto request = hal::rs485::receive_timeout(1);

        if (request.empty()) {
            cyphal::run_for_a_while();
        } else {
            const auto response = modbus::handle_transaction(modbus_dispatch, request, response_buffer);
            if (response.size()) {
                hal::rs485::ensure_silent_interval();
                hal::rs485::transmit_and_then_start_receiving(response);
            } else {
                hal::rs485::start_receiving();
            }
        }

        hal::rs485::housekeeping();
    }
}
