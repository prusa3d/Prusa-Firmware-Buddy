/// @file
#pragma once

#include <span>
#include <atomic>

#include "base_hotend.hpp"

#include <module/temperature/marlin_temptable.hpp>
#include <module/temperature/heatbreak_regulator.hpp>
#include <sum_ring_buffer.hpp>

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
    #include <module/temperature/thermal_model_protection.hpp>
#endif

#if ENABLED(MODEL_BASED_HOTEND_REGULATOR)
    #include <module/temperature/hotend_regulator/model_based_hotend_regulator.hpp>
using HotendRegulator = ModelBasedHotendRegulator;

#else
    #include <module/temperature/hotend_regulator/standard_hotend_regulator.hpp>
using HotendRegulator = StandardHotendRegulator;
#endif

#if WATCH_HEATBREAK
    #include <module/temperature/heater_watch.hpp>
#endif

/// Represents a hotend that is controlled on the current processor (not on a dwarf)
class LocalHotend final : public BaseHotend {

public:
    using TempTable = std::span<const short[2]>;

    struct Config {
        Hotend::Config base_config;

        /// Temperature table for mapping raw temperature readouts
        TempTable nozzle_temp_table;

#if HAS_TEMP_HEATBREAK
        TempTable heatbreak_temp_table;
#endif

        /// "Marlin pin" controlling the nozzle heater
        /// Gets then passed through analogWrite/digitalWrite from Arduino.h,
        /// Which gets eventually mapped to the actual HAL calls in hwio_XX.cpp
        /// One day, it would be nice to untangle this mess.
        uint32_t nozzle_heater_marlin_pin;

        /// Whether the nozzle heater should use software bitbanged PWM
        /// If true, the pin is actually controlled by digitalWrite() in Temperature::isr
        /// Otherwise, analogWrite() is used
        bool nozzle_heater_soft_pwm : 1;

        /// Maximum nozzle temperature (celsius) below which the extra averaging
        /// filter is applied. NTC: 50 (filter only at cold temps where noise is worst).
        /// PT1000 / HT hotend: use a high value (e.g. 500) to always filter.
        int16_t nozzle_filter_max_temp = 50;
    };

public:
    /// !!! Careful, the config pointer is stored, so make sure the config is persistent!
    explicit LocalHotend(PhysicalToolIndex tool, const Config *config);

#if HAS_TEMP_HEATBREAK_CONTROL
    void set_heatbreak_target_temp(TargetTemperature set) override;
#endif

protected:
    virtual void manage() override;

    void handle_nozzle_target_change() override;

    virtual void isr_on_readings_ready() override;

    virtual void isr_soft_pwm(PWM255 phase) override;

private:
#if HAS_TEMP_HEATBREAK
    void manage_heatbreak();
#endif

protected:
    const Config &local_config_;

    MarlinTemptableRawMinMax nozzle_raw_temp_range_;

#if HAS_TEMP_HEATBREAK
    MarlinTemptableRawMinMax heatbreak_raw_temp_range_;
#endif

    HotendRegulator nozzle_regulator_;

    /// Additional filter for low temperature nozzle values
    /// Increases oversampling to improve accuracy
    SumRingBuffer<uint16_t, uint32_t, 8> nozzle_low_temp_filter_;

#if ENABLED(PIDTEMPHEATBREAK)
    HeatbreakRegulator heatbreak_fan_regulator_;
#endif

#if WATCH_HEATBREAK
    HeaterWatch heatbreak_watch_;
#endif

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
    ThermalModelProtection thermal_model_protection_;
#endif

#if ENABLED(PID_EXTRUSION_SCALING)
    uint32_t last_e_position_ = 0;
#endif

#if HAS_TEMP_HEATBREAK
    millis_t next_heatbreak_check_ms_ = 0;
#endif

    /// Written from the Temperature ISR, read from the defaultTask
    /// !!! Contains a sum of OVERSAMPLENR samples
    std::atomic<uint16_t> nozzle_raw_temp_;

#if HAS_TEMP_HEATBREAK
    std::atomic<uint16_t> heatbreak_raw_temp_;
#endif

#if ENABLED(HAS_HOTEND_AUTO_FAN)
    bool auto_fan_out_ : 1 = false;
#endif
};
