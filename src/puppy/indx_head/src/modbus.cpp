
#include "critical_section.hpp"
#include "timing.hpp"
#include "hal.hpp"
#include "hal_crc.hpp"
#include "watchdog.hpp"
#include "spi_task.hpp"
#include "app.hpp"
#include "hotend_temp_compensation.hpp"

#include <indx_head/modbus.hpp>
#include <modbus/modbus.hpp>
#include <fifo_coder/fifo_encoder.hpp>

#include <array>
#include <cstring>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

using namespace indx_head::modbus;

namespace modbus {

namespace {
    struct State {
        Status status_regs;
        Config config_regs;
    };

    State state {};
    bool accel_fifo_enabled = false;
    bool loadcell_fifo_enabled = false;

    template <typename T>
    void sync_app_state_to_read() = delete;

    template <>
    void sync_app_state_to_read<indx_head::modbus::Status>() {
        // Lets update all the status data here.
        // WARNING: be aware that this will block the modbus communication (also with other puppies), so prefere reading from cached values

        state.status_regs.fault_status = hal::get_fault_status();

        // !!! MUST be before reading the temperatures themselves to avoid race condition
        state.status_regs.temps_valid = app::get_temps_valid();

        state.status_regs.hotend_measured_temperature_uncompensated_c100 = app::get_nozzle_temp_uncompensated_c100();
        state.status_regs.hotend_measured_temperature_compensated_c100 = app::get_nozzle_temp_compensated_c100();
        state.status_regs.hotend_temp_raw_c100_dt_s = app::get_hotend_temp_raw_c100_dt_s();
        state.status_regs.tpis_ambient_temperature_c100 = app::get_tpis_ambient_temp_c100();

        state.status_regs.hotend_pwm_averaged = app::get_hotend_pwm_averaged();

        const uint32_t power_int = app::get_hotend_energy_consumed_uJ();
        state.status_regs.hotend_energy_consumed_uJ_lo = uint16_t(power_int);
        state.status_regs.hotend_energy_consumed_uJ_hi = uint16_t(power_int >> 16);

        state.status_regs.board_temperature = hal::adc::get_board_temp();
        state.status_regs.mcu_temperature = hal::adc::get_mcu_temp();

        // System monitoring
        state.status_regs.system_24V_mV = hal::adc::get_input_voltage_mV();

        // Print fan status
        const uint8_t print_pwm = app::get_printfan_pwm();
        const uint16_t print_rpm = hal::tim::get_printfan_rpm_counter();
        state.status_regs.print_fan_pwm = print_pwm;
        state.status_regs.print_fan_state = (print_pwm > 0) ? 3 : 0; // 3=running, 0=idle
        state.status_regs.print_fan_rpm = print_rpm;
        state.status_regs.print_fan_is_rpm_ok = app::is_printfan_rpm_ok();

        // Heatbreak fan status - PID controlled
        const uint8_t heatbreak_pwm = app::get_heatbreak_fan_pwm();
        const uint16_t heatbreak_rpm = hal::tim::get_heatbreak_fan_rpm_counter();
        state.status_regs.heatbreak_fan_pwm = heatbreak_pwm;
        state.status_regs.heatbreak_fan_state = (heatbreak_pwm > 0) ? 3 : 0; // 3=running, 0=idle
        state.status_regs.heatbreak_fan_rpm = heatbreak_rpm;
        state.status_regs.heatbreak_fan_is_rpm_ok = app::is_heatbreak_fan_rpm_ok();

        uint32_t time_us = static_cast<uint32_t>(timing::get_timestamp_us());
        state.status_regs.time_sync_lo = time_us & 0xffff;
        state.status_regs.time_sync_hi = time_us >> 16;

        state.status_regs.nozzle_present = std::to_underlying(app::get_nozzle_present());
        state.status_regs.nozzle_invalidation_ack = app::get_nozzle_invalidation_ack();

        state.status_regs.ringdown_decay = app::get_ringdown_decay();
        state.status_regs.heater_current_mA = app::get_heater_current_mA();
    }

    template <typename T>
    void sync_app_state_to_write(const T &new_state) = delete;

    template <>
    void sync_app_state_to_write<indx_head::modbus::Config>(const indx_head::modbus::Config &new_status) {
        // Update all of your tasks values with current config registers
        // The previous state is still stored in state, so you can compare values if needed

        // Update heater target temperature
        if (new_status.nozzle_target_temperature != state.config_regs.nozzle_target_temperature) {
            app::set_nozzle_target_temp(new_status.nozzle_target_temperature);
        }

        if (new_status.hotend_temperature_compensation_c100 != state.config_regs.hotend_temperature_compensation_c100) {
            hotend_temp_compensation::set_target_compensation_c100(new_status.hotend_temperature_compensation_c100);
        }

        // Update fan PWMs
        if (new_status.print_fan_pwm.value != state.config_regs.print_fan_pwm.value) {
            app::set_printfan_pwm(new_status.print_fan_pwm.value);
        }

        // Set selftest mode - set heatbreak fan (autofan) pwm to 255
        if (new_status.selftest_mode != state.config_regs.selftest_mode) {
            app::set_selftest_mode(new_status.selftest_mode != 0);
        }

        // Update LED color
        if (new_status.leds != state.config_regs.leds) {
            app::set_led_config(new_status.leds);
        }

        // Invalidate nozzle presence — buddy sends a token, head resets and sends token back
        if (new_status.invalidate_nozzle_presence != state.config_regs.invalidate_nozzle_presence) {
            app::invalidate_nozzle_presence(new_status.invalidate_nozzle_presence);
        }

        if (new_status.loadcell_enabled != state.config_regs.loadcell_enabled) {
            if (new_status.loadcell_enabled) {
                // Drain samples from local FIFO before enabling modbus FIFO.
                // Samples contain stale data.
                // Master doesn't like that at all.
                fifo_coder::LoadcellRecord dummy;
                while (spi_task::loadcell_samples.dequeue(dummy)) {
                }
            }
            loadcell_fifo_enabled = new_status.loadcell_enabled;
        }

        if (new_status.clear_fault_status != 0) {
            hal::clear_fault_status(static_cast<indx_head::errors::FaultStatusMask>(new_status.clear_fault_status));
        }

        if (new_status.accelerometer_enabled != state.config_regs.accelerometer_enabled) {
            if (new_status.accelerometer_enabled) {
                // Drain samples from local FIFO before enabling modbus FIFO.
                // Samples contain stale data and/or error bit.
                // Master doesn't like that at all.
                uint32_t dummy;
                while (spi_task::accel_samples.dequeue(dummy)) {
                }
            }
            accel_fifo_enabled = new_status.accelerometer_enabled;
        }
    }

    void build_fifo_response(WritableBytes &out) {
        using namespace fifo_coder;
        // Reserve 4 bytes for header (byte_count + fifo_count)
        constexpr size_t header_size = 4;
        debug_assert(out.size() >= header_size + fifo_coder::MODBUS_FIFO_LEN * sizeof(uint16_t));

        std::array<uint16_t, fifo_coder::MODBUS_FIFO_LEN> fifo_buffer {};
        Encoder encoder(fifo_buffer);
        bool encoded = true;
        static int acc_sample_counter = 0;
        while (encoded) {
            encoded = false;
            if (encoder.can_encode<AccelerometerFastData>() && spi_task::accel_samples.count() >= std::tuple_size_v<AccelerometerFastData>) {
                AccelerometerFastData samples {};
                for (auto &sample : samples) {
                    if (!spi_task::accel_samples.dequeue(sample)) [[unlikely]] {
                        std::abort();
                    }
                    ++acc_sample_counter;
                }
                if (accel_fifo_enabled) {
                    encoder.encode(samples);
                }
                encoded = true;
            }
            if (spi_task::loadcell_samples.count() > 0 && encoder.can_encode<LoadcellRecord>()) {
                LoadcellRecord sample {};
                if (spi_task::loadcell_samples.dequeue(sample)) {
                    if (loadcell_fifo_enabled) {
                        encoder.encode(sample);
                    }
                    encoded = true;
                }
            }
            if (acc_sample_counter >= 100) {
                acc_sample_counter = 0;
                if (accel_fifo_enabled && encoder.can_encode<AccelerometerSamplingRate>()) {
                    AccelerometerSamplingRate sample = { .frequency = spi_task::measured_sampling_rate() };
                    if (encoder.encode(sample)) {
                        encoded = true;
                    }
                }
            }
        }
        encoder.padd();
        const size_t fifo_count = encoder.position();
        const size_t fifo_byte_size = fifo_count * sizeof(uint16_t);

        // Function code 24 response format:
        // - Byte count (2 bytes, big-endian): total bytes following (2 + fifo_count * 2)
        // - FIFO count (2 bytes, big-endian): number of registers
        // - FIFO data (N x 2 bytes, big-endian)
        const uint16_t byte_count = static_cast<uint16_t>(2 + fifo_byte_size);
        out[0] = std::byte(byte_count >> 8);
        out[1] = std::byte(byte_count & 0xff);
        out[2] = std::byte(fifo_count >> 8);
        out[3] = std::byte(fifo_count & 0xff);

        // Copy FIFO data with byte swapping (to big-endian)
        for (size_t i = 0; i < fifo_count; ++i) {
            out[header_size + i * 2] = std::byte(fifo_buffer[i] >> 8);
            out[header_size + i * 2 + 1] = std::byte(fifo_buffer[i] & 0xff);
        }
        out = out.subspan(0, header_size + fifo_byte_size);
    }

    template <typename T>
    concept CoilStorage = requires {
        typename T::IndexType;
    };

    template <typename Block>
        requires(!CoilStorage<Block>)
    constexpr bool is_op_in_range(const uint16_t address, const size_t count) {
        return address >= Block::address && (address + count) <= (Block::address + sizeof(Block) / 2);
    }

    template <typename Block>
        requires CoilStorage<Block>
    constexpr bool is_op_in_range(const uint16_t address, const size_t count) {
        return address >= Block::address && (address + count) <= (Block::address + static_cast<size_t>(Block::IndexType::_cnt));
    }

    struct Logic final : public modbus::Callbacks {
    public:
        [[nodiscard]] modbus::ServerAddress server_address() const final { return modbus::ServerAddress::indx_head; }

        Status read_registers([[maybe_unused]] const uint16_t address, [[maybe_unused]] std::span<uint16_t> out) final {
            if (is_op_in_range<indx_head::modbus::Status>(address, out.size())) {
                sync_app_state_to_read<indx_head::modbus::Status>();
                const auto offset = address - indx_head::modbus::Status::address;
                const auto byte_offset = offset * 2;
                const auto byte_size = out.size() * 2;
                {
                    CriticalSection cs;
                    memcpy(out.data(), reinterpret_cast<const uint8_t *>(&state.status_regs) + byte_offset, byte_size);
                }
                return Status::Ok;
            }
            return Status::IllegalAddress;
        }

        Status write_registers([[maybe_unused]] const uint16_t address, [[maybe_unused]] std::span<const uint16_t> in) final {
            if (is_op_in_range<indx_head::modbus::Config>(address, in.size())) {
                const auto offset = address - indx_head::modbus::Config::address;
                const auto byte_offset = offset * 2;
                const auto byte_size = in.size() * 2;
                indx_head::modbus::Config new_config = state.config_regs;
                {
                    CriticalSection cs;
                    memcpy(reinterpret_cast<uint8_t *>(&new_config) + byte_offset, in.data(), byte_size);
                }
                sync_app_state_to_write<indx_head::modbus::Config>(new_config);
                state.config_regs = new_config;
                return Status::Ok;
            }
            return Status::IllegalAddress;
        }

        static constexpr uint8_t FIFO_CODE = 0x18; // Modbus function code 24 (Read FIFO Queue)
        Status custom_function(uint8_t func_code, [[maybe_unused]] Bytes in, WritableBytes &out) final {
            switch (func_code) {
            case FIFO_CODE: {
                build_fifo_response(out);
                return Status::Ok;
            } break;
            default:
                break;
            }
            return Status::IllegalFunction;
        }
    };

} // namespace

void run() {
    Logic logic;
    auto handlers = std::to_array<modbus::Callbacks *>({ &logic });
    modbus::Dispatch dispatch(std::span { handlers });
    static std::array<std::byte, 256> response_buffer;

    WritableBytes response;
    FOREVER_WITH_WATCHDOG(900) {
        const auto request = hal::rs485::maybe_transmit_and_then_receive(response);
        response = modbus::handle_transaction(dispatch, request, response_buffer, hal::crc::compute_crc16_modbus);
    }
}

} // namespace modbus
