#pragma once

#include "adc.hpp"
#include "hwio_pindef.h"

class Cheese {
public:
    static constexpr uint16_t HALL_ON_STATE = 1470; //< Value when hall sensor is detecting magnet
    static constexpr uint16_t HALL_OFF_STATE = 761; //< Value without any magnet
    static constexpr uint16_t HALL_HYSTERESIS = 100;
    static constexpr uint16_t HALL_THRESHOLD_MID = (HALL_ON_STATE + HALL_OFF_STATE) / 2;

    static constexpr uint16_t HALL_THRESHOLD_LOW = HALL_THRESHOLD_MID - HALL_HYSTERESIS / 2;
    static constexpr uint16_t HALL_THRESHOLD_HIGH = HALL_THRESHOLD_MID + HALL_HYSTERESIS / 2;

    static inline void set_led(bool state) {
        buddy::hw::ledPWM.writeb(state);
    }

    static inline uint16_t get_raw_parked() {
        return AdcGet::picked0();
    }

    static inline uint16_t get_raw_picked() {
        return AdcGet::picked1();
    }

    static inline bool is_picked() {
        return Cheese::picked;
    }

    static inline bool is_parked() {
        return Cheese::parked;
    }

    static void update();

private:
    static bool picked;
    static bool parked;

    static bool update_state(bool current_state, uint16_t raw_value, const char *name);
};
