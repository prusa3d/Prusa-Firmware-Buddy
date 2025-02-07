#pragma once

#include <cstdint>

// Mocked config store with the one variable we need. Not persistent.

struct ConfigStore {
    struct Value {
        uint8_t v = 100;
        void set(uint16_t n) {
            v = n;
        }
        uint8_t get() const {
            return v;
        }
    };

    Value chamber_fan_max_control_pwm;
};

ConfigStore &config_store();
