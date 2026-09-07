#include <puppies/INDX.hpp>

#include <utility>

#include <bsod/bsod.h>
#include <fifo_coder/fifo_decoder.hpp>
#include <freertos/mutex.hpp>
#include <indx_head/errors.hpp>
#include <indx_head/nozzle_presence.hpp>

#include <logging/log.hpp>
#include "loadcell.hpp"
#include "timing.h"
#include <logging/log_dest_bufflog.hpp>
#include <puppies/PuppyBootstrap.hpp>
#include <i18n.h>
#include <config_store/store_instance.hpp>
#include "Marlin/src/module/prusa/accelerometer.h"
#include <utils/string_builder.hpp>
#include <option/has_indx_head.h>
#include <otp/types.hpp>

using namespace fifo_coder;

namespace buddy::puppies {

using Lock = std::unique_lock<freertos::Mutex>;

LOG_COMPONENT_DEF(INDX, logging::Severity::info);

Indx::Indx(uint8_t modbus_address)
    : ModbusDevice(modbus_address)
    , mutex(new freertos::Mutex)
    , time_sync(1) // Just magic number for metric (unnecessary with single head)
    , loadcell_samplerate {} {

    set_leds_config(config_store().tool_leds_enabled.get() ? desired_led_mode : indx_head::leds::Mode::off, COLOR_ORANGE);
    loadcell_enabled = true;
}

CommunicationStatus Indx::refresh(PuppyModbus &bus) {
    Lock guard(*mutex);
    typedef CommunicationStatus (Indx::*MethodType)(PuppyModbus &);
    static constexpr MethodType funcs[] = {
        &Indx::read_general_status,
        &Indx::write_general,
    };
    if (++refresh_nr >= std::size(funcs)) {
        refresh_nr = 0;
    }
    return (this->*funcs[refresh_nr])(bus);
}

void Indx::handle_fault_status(indx_head::errors::FaultStatusMask fault) {
    if (fault == indx_head::errors::FaultStatusMask::no_fault) {
        return;
    }

    log_error(INDX, "Fault status: %d", std::to_underlying(fault));
    auto has_fault = [=](indx_head::errors::FaultStatusMask tested) {
        return std::to_underlying(fault) & std::to_underlying(tested);
    };
    if (has_fault(indx_head::errors::FaultStatusMask::board_min_temp)) {
        fatal_error(ErrCode::ERR_TEMPERATURE_INDX_HEAD_BOARD_MINTEMP_ERR);
    }
    if (has_fault(indx_head::errors::FaultStatusMask::board_max_temp)) {
        fatal_error(ErrCode::ERR_TEMPERATURE_INDX_HEAD_BOARD_MAXTEMP_ERR);
    }
    if (has_fault(indx_head::errors::FaultStatusMask::tpis_ambient_min_temp)) {
        fatal_error(ErrCode::ERR_TEMPERATURE_INDX_HEAD_TPIS_AMBIENT_MINTEMP_ERR);
    }
    if (has_fault(indx_head::errors::FaultStatusMask::tpis_ambient_max_temp)) {
        fatal_error(ErrCode::ERR_TEMPERATURE_INDX_HEAD_TPIS_AMBIENT_MAXTEMP_ERR);
    }

    // Acknowledge the fault — write_general() will flush this and reset it to 0 after success.
    clear_fault_status_pending = std::to_underlying(fault);
    general_write_dirty.store(true);
}

void Indx::handle_nozzle_presence(uint16_t nozzle_present, uint16_t nozzle_invalidation_ack) {
    // Trust nozzle data only when both are true:
    //  1. Head echoed back our invalidation token (debouncer was reset)
    //  2. Head reports a definitive result (not unknown — debouncer has settled with fresh samples)
    using indx_head::NozzlePresence;
    const auto presence = static_cast<NozzlePresence>(nozzle_present);
    const bool has_definitive_result = presence != NozzlePresence::unknown;
    const bool head_acknowledged = nozzle_invalidation_ack == nozzle_invalidation_token.load();
    const bool valid = has_definitive_result && head_acknowledged;

    nozzle_state.store(valid ? presence : NozzlePresence::unknown);
}

void Indx::handle_time_sync(uint32_t time_sync_hi, uint32_t time_sync_lo, const RequestTiming &timing) {
    const uint32_t puppy_time_us = time_sync_hi << 16 | time_sync_lo;
    time_sync.sync(puppy_time_us, timing);
}

namespace fans {
    constexpr size_t PRINTFAN_INDEX = 0;
    constexpr size_t HEATBREAKFAN_INDEX = 1;
} // namespace fans

CommunicationStatus Indx::read_general_status(PuppyModbus &bus) {
    ModbusInputRegisterBlock<indx_head::modbus::Status::address, indx_head::modbus::Status> block {};
    block.last_read_timestamp_ms = register_general_status_modbus_last_read_ms;
    RequestTiming timing;
    const CommunicationStatus status = bus.read(unit, block, 250, &timing);
    register_general_status_modbus_last_read_ms = block.last_read_timestamp_ms;
    if (status == CommunicationStatus::OK) {
        handle_fault_status(block.value.fault_status);
        handle_nozzle_presence(block.value.nozzle_present, block.value.nozzle_invalidation_ack);

        hotend_temp_compensated_c100.store(block.value.hotend_measured_temperature_compensated_c100);
        hotend_temp_uncompensated_c100.store(block.value.hotend_measured_temperature_uncompensated_c100);
        hotend_temp_raw_c100_dt_s.store(block.value.hotend_temp_raw_c100_dt_s);
        hotend_pwm_averaged.store(static_cast<uint8_t>(block.value.hotend_pwm_averaged));
        hotend_energy_consumed_uJ.store(
            block.value.hotend_energy_consumed_uJ_lo
            | (uint32_t(block.value.hotend_energy_consumed_uJ_hi) << 16));

        mcu_temperature.store(block.value.mcu_temperature);
        board_temperature.store(block.value.board_temperature);
        tpis_ambient_temperature_c100.store(block.value.tpis_ambient_temperature_c100);
        v24_mV.store(block.value.system_24V_mV);

        fan_pwm[fans::PRINTFAN_INDEX].store(block.value.print_fan_pwm);
        fan_pwm[fans::HEATBREAKFAN_INDEX].store(block.value.heatbreak_fan_pwm);
        fan_rpm[fans::PRINTFAN_INDEX].store(block.value.print_fan_rpm);
        fan_rpm[fans::HEATBREAKFAN_INDEX].store(block.value.heatbreak_fan_rpm);
        fan_state[fans::PRINTFAN_INDEX].store(block.value.print_fan_state);
        fan_state[fans::HEATBREAKFAN_INDEX].store(block.value.heatbreak_fan_state);

        uint8_t rpm_ok_mask = 0;
        if (block.value.print_fan_is_rpm_ok) {
            rpm_ok_mask |= static_cast<uint8_t>(1u << fans::PRINTFAN_INDEX);
        }
        if (block.value.heatbreak_fan_is_rpm_ok) {
            rpm_ok_mask |= static_cast<uint8_t>(1u << fans::HEATBREAKFAN_INDEX);
        }
        fan_rpm_ok.store(rpm_ok_mask);

        ringdown_decay.store(block.value.ringdown_decay);
        heater_current_mA.store(block.value.heater_current_mA);

        // !!! MUST be stored after reading the temperatures to avoid race conditions
        temps_valid.store(block.value.temps_valid);

        handle_time_sync(block.value.time_sync_hi, block.value.time_sync_lo, timing);

        // 0 has a special meaning, report one ms less if we would try to set zero
        // !!! MUST be updated as the last thing to avoid race conditions
        const auto now_ms = last_ticks_ms();
        register_general_status_last_read_ms.store((now_ms != 0) ? now_ms : uint32_t(-1));
    }
    return status;
}

CommunicationStatus Indx::ping(PuppyModbus &bus) {
    Lock guard(*mutex);
    // Need to check if puppy is responding on modbus, might as well read general status.
    ModbusInputRegisterBlock<indx_head::modbus::Status::address, indx_head::modbus::Status> block {};
    return bus.read(unit, block, 0);
}

CommunicationStatus Indx::initial_scan(PuppyModbus &bus) {
    Lock guard(*mutex);
    time_sync.init();

    loadcell_enabled = true;
    general_write_dirty.store(true);

    register_general_status_last_read_ms = 0;
    temps_valid = false;

    // !!! MUST be after resetting all the stuff to avoid race conditions
    // The intention is that when someone detects a reset, we want to guarantee that the data is already marked as invalid
    // and will become valid again only after it is properly fetched from the newly reset puppy.
    reset_counter++;

    return write_general(bus);
}

void Indx::set_leds_config(indx_head::leds::Mode mode, Color primary, Color secondary, uint16_t delay_ms) {
    Lock guard(*mutex);
    // WARNING: This switch is itentional to keep as much data if user decides to disable the leds (off mode) and then enable them again.
    switch (mode) {
    case indx_head::leds::Mode::blinking:
    case indx_head::leds::Mode::pulsing:
        leds.secondary.r = secondary.r;
        leds.secondary.g = secondary.g;
        leds.secondary.b = secondary.b;
        [[fallthrough]];
    case indx_head::leds::Mode::solid:
        leds.primary.r = primary.r;
        leds.primary.g = primary.g;
        leds.primary.b = primary.b;
        leds.delay_ms = delay_ms;
        [[fallthrough]];
    case indx_head::leds::Mode::match_nozzle_temp:
        desired_led_mode = mode;
        [[fallthrough]];
    case indx_head::leds::Mode::off:
        leds.mode = mode;
    }
    general_write_dirty.store(true);
}

void Indx::set_leds_solid_color(Color color, uint16_t delay_ms) {
    set_leds_config(indx_head::leds::Mode::solid, color, Color::from_rgb(0, 0, 0), delay_ms);
}

void Indx::set_leds_blinking(Color primary, Color secondary, uint16_t delay_ms) {
    set_leds_config(indx_head::leds::Mode::blinking, primary, secondary, delay_ms);
}

void Indx::set_leds_pulsing(Color primary, Color secondary, uint16_t delay_ms) {
    set_leds_config(indx_head::leds::Mode::pulsing, primary, secondary, delay_ms);
}

void Indx::set_leds_to_follow_nozle_temp() {
    set_leds_config(indx_head::leds::Mode::match_nozzle_temp);
}

void Indx::set_leds_enabled(bool set) {
    Lock guard(*mutex);
    leds.mode = set ? desired_led_mode : indx_head::leds::Mode::off;
    general_write_dirty.store(true);
}

CommunicationStatus Indx::pull_fifo(PuppyModbus &bus, bool &more) {
    Lock guard(*mutex);

    // Read coded FIFO
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    size_t read = 0;
    CommunicationStatus status = bus.ReadFIFO(unit, ENCODED_FIFO_ADDRESS, fifo, read, FIFO_RETRIES);
    if (status == CommunicationStatus::ERROR) {
        more = true; // Request failed, most probably there is more data waiting
        PrusaAccelerometer::set_possible_overflow();
        return status;
    }

    if (!read) {
        more = false;
        return CommunicationStatus::OK;
    }

    Decoder decoder(fifo, read);
    decoder.decode(*this);

    // Update sampling rate of the loadcell.ProcessSample()
    if (loadcell_samplerate.count > 30) {
        float interval = static_cast<float>(loadcell_samplerate.last_timestamp - loadcell_samplerate.last_processed_timestamp) / static_cast<float>(1000 * loadcell_samplerate.count);
        // Ignore invalid values, values outside of 25% of expected value may be caused by glitch in modbus communication
        if (interval >= loadcell_samplerate.expected * 0.75f && interval <= loadcell_samplerate.expected * 1.25f) {
            loadcell.analysis.SetSamplingIntervalMs(interval); // Update sampling interval
        }
        loadcell_samplerate.count = 0;
        loadcell_samplerate.last_processed_timestamp = loadcell_samplerate.last_timestamp;
    }

    more = decoder.more();
    return status;
}

CommunicationStatus Indx::write_general(PuppyModbus &bus) {
    // Clear dirty before snapshotting; a racing setter then either lands in
    // our snapshot or re-marks dirty for the next cycle.
    const bool was_dirty = general_write_dirty.exchange(false);

    ModbusHoldingRegisterBlock<indx_head::modbus::Config::address, indx_head::modbus::Config> block {};
    block.value.selftest_mode = selftest_mode_.load() ? 1u : 0u;
    block.value.nozzle_target_temperature = nozzle_target_temperature_desired.load();
    block.value.hotend_temperature_compensation_c100 = hotend_temperature_compensation_c100_desired.load();
    block.value.invalidate_nozzle_presence = nozzle_invalidation_token.load();
    block.value.print_fan_pwm.value = static_cast<uint8_t>(fan_pwm_desired[fans::PRINTFAN_INDEX].load());
    block.value.leds = leds;
    block.value.loadcell_enabled = loadcell_enabled ? 1u : 0u;
    block.value.accelerometer_enabled = accelerometer_enabled ? 1u : 0u;
    block.value.clear_fault_status = clear_fault_status_pending;
    block.dirty = was_dirty;

    const CommunicationStatus status = bus.write(unit, block);
    if (status == CommunicationStatus::ERROR && was_dirty) {
        // Write didn't go through, keep work for next cycle.
        general_write_dirty.store(true);
    }

    if (status == CommunicationStatus::OK) {
        clear_fault_status_pending = 0;
        log_debug(INDX, "Written GeneralWrite");
    }
    return status;
}

float Indx::get_hotend_temp_compensated() const {
    // Sent in centiDeg (deg * 100) for precision on 2 decimal places
    return static_cast<float>(hotend_temp_compensated_c100.load()) / 100.f;
}

float Indx::get_hotend_temp_uncompensated() const {
    // Sent in centiDeg (deg * 100) for precision on 2 decimal places
    return static_cast<float>(hotend_temp_uncompensated_c100.load()) / 100.f;
}

float Indx::get_hotend_temp_raw_c_dt_s() const {
    // Sent in centiDeg (deg * 100) for precision on 2 decimal places
    return static_cast<float>(hotend_temp_raw_c100_dt_s.load()) / 100.f;
}

uint8_t Indx::get_hotend_pwm_averaged() const {
    return hotend_pwm_averaged.load();
}

uint32_t Indx::get_hotend_energy_consumed_uJ() const {
    return hotend_energy_consumed_uJ.load();
}

CommunicationStatus Indx::set_hotend_target_temp(float target) {
    const uint16_t value = static_cast<uint16_t>(target);
    if (nozzle_target_temperature_desired.exchange(value) != value) {
        general_write_dirty.store(true);
    }
    return CommunicationStatus::OK;
}

CommunicationStatus Indx::set_hotend_temp_compensation(float offset) {
    const int16_t value = static_cast<int16_t>(offset * 100.0f);
    if (hotend_temperature_compensation_c100_desired.exchange(value) != value) {
        general_write_dirty.store(true);
    }
    return CommunicationStatus::OK;
}

bool Indx::set_accelerometer(PuppyModbus &bus, bool active) {
    Lock guard(*mutex);
    accelerometer_enabled = active;
    general_write_dirty.store(true);
    return write_general(bus) == CommunicationStatus::OK;
}

bool Indx::get_accelerometer_active() {
    Lock guard(*mutex);
    return accelerometer_enabled;
}

void Indx::set_loadcell(bool active) {
    Lock guard(*mutex);
    loadcell_enabled = active;
    general_write_dirty.store(true);
}

bool Indx::get_loadcell_active() {
    Lock guard(*mutex);
    return loadcell_enabled;
}

int16_t Indx::get_mcu_temperature() {
    return mcu_temperature.load();
}

int16_t Indx::get_board_temperature() {
    return board_temperature.load();
}

float Indx::get_tpis_ambient_temperature() {
    // Sent in centiDeg (deg * 100) for precision on 2 decimal places
    return static_cast<float>(tpis_ambient_temperature_c100.load()) / 100.f;
}

int16_t Indx::get_ringdown_decay() const {
    return ringdown_decay.load();
}

uint16_t Indx::get_heater_current_mA() const {
    return heater_current_mA.load();
}

float Indx::get_24V() {
    return static_cast<float>(v24_mV.load()) / 1000.0f;
}

std::optional<bool> Indx::get_nozzle_present() {
    // Called from Marlin - keep lockfree.
    const auto state = nozzle_state.load();
    if (state == indx_head::NozzlePresence::unknown) {
        return std::nullopt;
    }

    return state == indx_head::NozzlePresence::present;
}

void Indx::invalidate_nozzle_data() {
    // Generate a new token. The head will reset its debouncer and echo the
    // token back in nozzle_invalidation_ack. handle_nozzle_presence() won't
    // re-validate until the ack matches.
    uint16_t token = static_cast<uint16_t>(ticks_ms());
    if (token == 0) {
        token = 1; // Avoid 0 — it's the head's initial ack value
    }
    // Bump the token before clearing the cache: handle_nozzle_presence() running on
    // the puppy task can race with this function and write back the previous (now stale)
    // state if it observes the old token still matching the head's old ack.
    nozzle_invalidation_token.store(token);
    general_write_dirty.store(true);
    nozzle_state.store(indx_head::NozzlePresence::unknown);
}

std::optional<uint32_t> Indx::get_register_general_status_last_read_ms() const {
    const auto val = register_general_status_last_read_ms.load();
    return val ? std::optional { val } : std::nullopt;
}

void Indx::set_fan(uint8_t fan, uint16_t target) {
    debug_assert(fan < NUM_FANS);
    // FIXME:
    // Because this sometimes gets called from an interrupt, we need to just
    // store the value and handle it properly under a lock somewhere else.
    // BFW-6219.
    if (fan_pwm_desired[fan].exchange(target) != target) {
        general_write_dirty.store(true);
    }
}

void Indx::set_fan_auto(uint8_t fan) {
    debug_assert(fan < NUM_FANS);
    if (fan_pwm_desired[fan].exchange(FAN_MODE_AUTO_PWM) != FAN_MODE_AUTO_PWM) {
        general_write_dirty.store(true);
    }
}

void Indx::set_selftest_mode(bool enabled) {
    if (selftest_mode_.exchange(enabled) != enabled) {
        general_write_dirty.store(true);
    }
}

uint16_t Indx::get_heatbreak_fan_pwr() {
    return fan_pwm[fans::HEATBREAKFAN_INDEX].load();
}

void Indx::decode_log(const LogData &data) {
    // This function is mandated by the API, but head doesn't actually produce
    // any logs and we intend to keep it that way.
    (void)data;
}

void Indx::decode_loadcell(const LoadcellRecord &data) {
    // throw away samples if time is not synced
    if (!this->time_sync.is_time_sync_valid()) {
        return;
    }

    // Store sample timestamp and count sample
    loadcell_samplerate.last_timestamp = this->time_sync.buddy_time_us(data.timestamp);
    loadcell_samplerate.count++;

    loadcell.ProcessSample(data.loadcell_raw_value, loadcell_samplerate.last_timestamp, reset_counter.load());
}

void Indx::decode_accelerometer_fast(const AccelerometerFastData &data) {
    for (AccelerometerXyzSample sample : data) {
        PrusaAccelerometer::put_sample(sample);
    }
}

void Indx::decode_accelerometer_freq(const AccelerometerSamplingRate &data) {
    PrusaAccelerometer::set_rate(data.frequency);
}

uint16_t Indx::get_fan_pwm(uint8_t fan_nr) const {
    switch (fan_nr) {
    case fans::PRINTFAN_INDEX:
        return fan_pwm[fans::PRINTFAN_INDEX].load();
    case fans::HEATBREAKFAN_INDEX:
        return fan_pwm[fans::HEATBREAKFAN_INDEX].load();
    }
    bsod_unreachable();
}

uint16_t Indx::get_fan_rpm(uint8_t fan_nr) const {
    switch (fan_nr) {
    case fans::PRINTFAN_INDEX:
        return fan_rpm[fans::PRINTFAN_INDEX].load();
    case fans::HEATBREAKFAN_INDEX:
        return fan_rpm[fans::HEATBREAKFAN_INDEX].load();
    }
    bsod_unreachable();
}

bool Indx::get_fan_rpm_ok(uint8_t fan_nr) const {
    switch (fan_nr) {
    case fans::PRINTFAN_INDEX:
        return (fan_rpm_ok.load() >> fans::PRINTFAN_INDEX) & 1u;
    case fans::HEATBREAKFAN_INDEX:
        return (fan_rpm_ok.load() >> fans::HEATBREAKFAN_INDEX) & 1u;
    }
    bsod_unreachable();
}

uint16_t Indx::get_fan_state(uint8_t fan_nr) const {
    switch (fan_nr) {
    case fans::PRINTFAN_INDEX:
        return fan_state[fans::PRINTFAN_INDEX].load();
    case fans::HEATBREAKFAN_INDEX:
        return fan_state[fans::HEATBREAKFAN_INDEX].load();
    }
    bsod_unreachable();
}

void Indx::set_otp(const OTP_v5 &otp_data) {
    Lock guard(*mutex);
    otp = otp_data;
}

OTP_v5 Indx::get_otp() const {
    Lock guard(*mutex);
    return otp;
}

#if HAS_INDX_HEAD()
Indx indx { PuppyBootstrap::get_modbus_address_for_dock(Dock::INDX_HEAD) };
#endif

} // namespace buddy::puppies
