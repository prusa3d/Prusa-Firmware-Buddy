#pragma once

#include <stdbool.h>
#include "Pin.hpp"
#include "CFanCtlCommon.hpp"

/// class for controlling a 4-pin enclosure fan -- it is not very suitable for general 4-pin fan,
/// if there is ever a need, it would be better to write something with a pid regulator and control it with percentage of rpm instead pwm)
class CFanCtlEnclosure : public CfanCtlCommon {
    static_assert(FANCTLENCLOSURE_PWM_MAX == 255);

public:
    CFanCtlEnclosure(const buddy::hw::InputPin &pin_tach,
        uint16_t min_rpm, uint16_t max_rpm);

public:
    /// tick callback from 1kHz timer interrupt
    void tick();

    uint16_t getMinRPM() const { return min_rpm; }
    uint16_t getMaxRPM() const { return max_rpm; }
    inline uint8_t getMaxPWM() const { return 255; }
    FanState getState() const { return state; }
    uint8_t getPWM() const { return desired_pwm; }
    uint16_t getActualRPM() const { return tachometer.get_rpm().value_or(0); }
    bool getRPMIsOk() { return !desired_pwm || getActualRPM() > min_rpm; }

    inline bool getRPMMeasured() const { return tachometer.get_rpm().has_value(); }

    void setPWM(uint8_t pwm) { desired_pwm = pwm; }

private:
    void set_actual_pwm(uint8_t);

    struct Tachometer {
        Tachometer(const buddy::hw::InputPin &pin)
            : pin(pin) {}
        /// tick callback from 1kHz timer interrupt, returns true if edge detected
        bool tick();
        std::optional<uint16_t> get_rpm() const { return rpm; }

    private:
        static constexpr uint16_t measurement_ticks_interval = 1000;
        const buddy::hw::InputPin &pin;
        bool previous_level = false;
        uint16_t edges = 0;
        uint16_t ticks = 0;
        std::atomic<std::optional<uint16_t>> rpm;
    };

    const uint16_t min_rpm;
    const uint16_t max_rpm;
    std::atomic<uint8_t> desired_pwm = 0;
    uint8_t actual_pwm;
    Tachometer tachometer;

    // state handling

    static constexpr uint16_t start_timeout = 2000; //< first edge detection timeout [milliseconds]
    static constexpr uint16_t rpm_delay = 5000; //< rpm stabilization phase timeout [mimmiseconds]

    static constexpr uint16_t start_edges = 4;

    std::atomic<FanState> state = idle;
    uint16_t edges = 0;
    uint16_t ticks = 0; //< state machine time measurement [milliseconds]
};
