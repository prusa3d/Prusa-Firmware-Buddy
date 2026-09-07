///@file
#include <puppies/ac_controller.hpp>

#include <modbus/server_address.hpp>
#include <utils/led_color.hpp>

using Lock = std::unique_lock<freertos::Mutex>;
using xbuddy_extension::NodeState;

static constexpr uint8_t unit = std::to_underlying(modbus::ServerAddress::ac_controller);

namespace buddy::puppies {

std::optional<float> AcController::get_mcu_temp() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return static_cast<float>(mcu_temp_dc.load()) / 10.0f;
}

std::optional<float> AcController::get_bed_temp() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return static_cast<float>(bed_temp_dc.load()) / 10.0f;
}

std::optional<float> AcController::get_bed_voltage() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return static_cast<float>(bed_voltage_dv.load()) / 10.0f;
}

std::optional<std::array<uint16_t, 2>> AcController::get_bed_fan_rpm() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return std::array<uint16_t, 2> { bed_fan_rpm[0].load(), bed_fan_rpm[1].load() };
}

std::optional<uint16_t> AcController::get_psu_fan_rpm() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return psu_fan_rpm.load();
}

std::optional<uint8_t> AcController::get_bed_fan_pwm() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return bed_fan_pwm_desired.load();
}

std::optional<uint8_t> AcController::get_psu_fan_pwm() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return psu_fan_pwm_desired.load();
}

std::optional<ac_controller::Faults> AcController::get_faults() const {
    if (!all_valid()) {
        return std::nullopt;
    }

    return ac_controller::Faults { faults.load() };
}

NodeState AcController::get_node_state() const {
    // Intentionally not using all_valid() — that would never return a state
    // other than ready (chicken-and-egg with NodeState::ready).
    return valid.load() ? static_cast<NodeState>(node_state.load()) : NodeState::unknown;
}

void AcController::set_bed_target_temp(float target_temp) {
    const uint16_t value = static_cast<uint16_t>(target_temp * 10.0f);
    if (bed_target_temp_desired.exchange(value) != value) {
        config_dirty.store(true);
    }
}

void AcController::set_bed_fan_pwm(uint8_t pwm) {
    if (bed_fan_pwm_desired.exchange(pwm) != pwm) {
        config_dirty.store(true);
    }
}

void AcController::set_psu_fan_pwm(uint8_t pwm) {
    if (psu_fan_pwm_desired.exchange(pwm) != pwm) {
        config_dirty.store(true);
    }
}

void AcController::turn_off_bed_leds() {
    Lock lock(mutex);
    if (led_animation_type != ac_controller::AnimationType::OFF) {
        led_animation_type = ac_controller::AnimationType::OFF;
        led_config_dirty.store(true);
    }
}

void AcController::set_rgbw_led(std::array<uint8_t, 4> rgbw) {
    Lock lock(mutex);
    if (led_animation_type != ac_controller::AnimationType::STATIC_COLOR) {
        led_animation_type = ac_controller::AnimationType::STATIC_COLOR;
        led_config_dirty.store(true);
    }
    if (led_rgbw != rgbw) {
        led_rgbw = rgbw;
        led_config_dirty.store(true);
    }
}

void AcController::set_progress_percent(uint8_t percent) {
    constexpr auto progress_color = leds::ColorRGBW { 0, 150, 255, 0 };

    Lock lock(mutex);
    if (led_animation_type != ac_controller::AnimationType::PROGRESS_PERCENT) {
        led_animation_type = ac_controller::AnimationType::PROGRESS_PERCENT;
        led_config_dirty.store(true);
    }
    if (led_progress_percent != percent) {
        led_progress_percent = percent;
        led_config_dirty.store(true);
    }
    const std::array<uint8_t, 4> progress_rgbw { progress_color.r, progress_color.g, progress_color.b, progress_color.w };
    if (led_rgbw != progress_rgbw) {
        led_rgbw = progress_rgbw;
        led_config_dirty.store(true);
    }
}

CommunicationStatus AcController::refresh_input(PuppyModbus &bus, uint32_t max_age) {
    // Already locked by caller.

    ModbusInputRegisterBlock<Status_t::address, Status_t> block {};
    block.last_read_timestamp_ms = status_last_read_ms;
    const auto result = bus.read(unit, block, max_age);
    status_last_read_ms = block.last_read_timestamp_ms;

    switch (result) {
    case CommunicationStatus::OK:
        mcu_temp_dc.store(static_cast<int16_t>(block.value.mcu_temp));
        bed_temp_dc.store(static_cast<int16_t>(block.value.bed_temp));
        bed_voltage_dv.store(block.value.bed_voltage);
        bed_fan_rpm[0].store(block.value.bed_fan_rpm[0]);
        bed_fan_rpm[1].store(block.value.bed_fan_rpm[1]);
        psu_fan_rpm.store(block.value.psu_fan_rpm);
        faults.store(static_cast<uint32_t>(block.value.faults_lo) | (static_cast<uint32_t>(block.value.faults_hi) << 16));
        node_state.store(block.value.node_state);
        // Only after all status fields are published, so readers never see valid=true
        // with stale cached values.
        valid.store(true);
        break;
    case CommunicationStatus::ERROR:
        valid.store(false);
        break;
    default:
        // SKIPPED doesn't change the validity.
        break;
    }

    return result;
}

CommunicationStatus AcController::refresh(PuppyModbus &bus) {
    Lock lock(mutex);

    // Clear dirty before snapshotting; a racing setter then either lands in
    // our snapshot or re-marks dirty for the next cycle.
    const bool config_was_dirty = config_dirty.exchange(false);
    const bool led_was_dirty = led_config_dirty.exchange(false);

    ModbusHoldingRegisterBlock<Config_t::address, Config_t> config_block {};
    config_block.value.bed_target_temp = bed_target_temp_desired.load();
    config_block.value.bed_fan_pwm = static_cast<uint16_t>(bed_fan_pwm_desired.load());
    config_block.value.psu_fan_pwm = static_cast<uint16_t>(psu_fan_pwm_desired.load());
    config_block.dirty = config_was_dirty;

    ModbusHoldingRegisterBlock<LedConfig_t::address, LedConfig_t> led_block {};
    led_block.value.animation_type = std::to_underlying(led_animation_type);
    led_block.value.led_r = led_rgbw[0];
    led_block.value.led_g = led_rgbw[1];
    led_block.value.led_b = led_rgbw[2];
    led_block.value.led_w = led_rgbw[3];
    led_block.value.progress_percent = led_progress_percent;
    led_block.dirty = led_was_dirty;

    const auto input = refresh_input(bus, 250);
    const auto holding_config = bus.write(unit, config_block);
    const auto holding_led_config = bus.write(unit, led_block);

    if (holding_config == CommunicationStatus::ERROR && config_was_dirty) {
        config_dirty.store(true);
    }
    if (holding_led_config == CommunicationStatus::ERROR && led_was_dirty) {
        led_config_dirty.store(true);
    }

    return aggregate_communication_status({
        input,
        holding_config,
        holding_led_config,
    });
}

CommunicationStatus AcController::initial_scan(PuppyModbus &bus) {
    Lock lock(mutex);

    const auto input = refresh_input(bus, 0);
    config_dirty.store(true);
    return input;
}

bool AcController::all_valid() const {
    return valid.load() && static_cast<NodeState>(node_state.load()) == NodeState::ready;
}

AcController ac_controller;

} // namespace buddy::puppies
