#pragma once

#include <buddy/door_sensor.hpp>
#include <device/board.h>
#include <option/has_bed_fan.h>
#include <option/has_door_sensor.h>
#include <option/has_loadcell.h>
#include <option/has_psu_fan.h>
#include <option/has_remote_bed.h>
#include <option/has_dwarf.h>
#include <option/has_indx.h>
#include <utils/atomic.hpp>
#include <utils/timing/rate_limiter.hpp>

// this struct collects data from metrics and gives access to the
// sensor info screen
struct SensorData {
private:
    friend SensorData &sensor_data();

    SensorData();
    SensorData(SensorData &other) = delete;
    SensorData(SensorData &&other) = delete;

    void update_MCU_temp();

    RateLimiter<uint32_t> limiter;
    uint8_t sample_nr = 0;
    int32_t mcu_sum = 0;
#if BOARD_IS_XLBUDDY()
    int32_t sandwich_sum = 0;
    int32_t splitter_sum = 0;
#endif

public:
    RelaxedAtomic<float> MCUTemp;
    RelaxedAtomic<float> sandwichTemp;
    RelaxedAtomic<float> splitterTemp;
    RelaxedAtomic<float> boardTemp;
    RelaxedAtomic<float> heatbreak_fan_pwm;
#if BOARD_IS_XLBUDDY()
    RelaxedAtomic<float> inputVoltage;
    RelaxedAtomic<float> sandwich5VVoltage;
    RelaxedAtomic<float> sandwich5VCurrent;
    RelaxedAtomic<float> buddy5VCurrent;
#elif BOARD_IS_XBUDDY()
    RelaxedAtomic<float> bed_voltage;
    RelaxedAtomic<float> heater_voltage;
    RelaxedAtomic<float> heater_current;
    RelaxedAtomic<float> input_current;
    RelaxedAtomic<float> mmuCurrent;
#endif
#if HAS_DWARF() || HAS_INDX()
    static float head_pcb_temperature();
    static float head_mcu_temperature();
#endif
#if HAS_INDX()
    static float head_ambient_temperature();
    static float nozzle_temp_uncompensated();
    static int16_t ringdown_decay();

    float nozzle_power_W() const {
        return nozzle_power_W_.load(std::memory_order_relaxed);
    }
#endif

#if HAS_DOOR_SENSOR()
    RelaxedAtomic<buddy::DoorSensor::DetailedState> door_sensor_detailed_state;
#endif
#if HAS_LOADCELL()
    RelaxedAtomic<float> loadCell;
#endif
#if HAS_REMOTE_BED()
    RelaxedAtomic<float> bedMCUTemperature;
#endif
#if HAS_BED_FAN()
    RelaxedAtomic<uint16_t> bed_fan1_rpm;
    RelaxedAtomic<uint16_t> bed_fan2_rpm;
    RelaxedAtomic<uint8_t> bed_fan1_pwm;
    RelaxedAtomic<uint8_t> bed_fan2_pwm;
#endif
#if HAS_PSU_FAN()
    RelaxedAtomic<uint16_t> psu_fan_rpm;
    RelaxedAtomic<uint8_t> psu_fan_pwm;
#endif

    void update();

private:
    uint32_t last_update_ms_ = 0;

#if HAS_INDX()
    std::atomic<float> nozzle_power_W_ = 0;
    uint32_t last_nozzle_energy_uJ_ = 0;
#endif
};

SensorData &sensor_data();
