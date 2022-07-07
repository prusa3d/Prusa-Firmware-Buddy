/**
 * @file filament_sensor.hpp
 * @author Radek Vana
 * @brief basic api of filament sensor
 * @date 2019-12-16
 */

#pragma once

#include <stdint.h>
#include <optional>
#include <atomic>

enum class fsensor_t : uint8_t {
    NotInitialized, //enable enters this state too
    NotCalibrated,
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};

//basic filament sensor api
class FSensor {
public:
    enum class event {
        NoFilament,
        HasFilament,
        EdgeFilamentInserted,
        EdgeFilamentRemoved
    };

protected:
    std::atomic<fsensor_t> state;
    uint8_t meas_cycle;

    void init();
    void set_state(fsensor_t st);

    event generateEvent(fsensor_t last_state_before_cycle) const;

    void enable();
    void disable();
    void cycle();
    void cycle0();
    void cycle1();

    void record_state(); // record metrics
public:
    std::optional<FSensor::event> Cycle();
    //thread safe functions
    fsensor_t Get();

    //thread safe functions, but cannot be called from interrupt
    void Enable();
    void Disable();

    fsensor_t WaitInitialized();

    FSensor();
};
