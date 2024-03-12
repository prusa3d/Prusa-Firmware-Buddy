#pragma once

#include <cstdint>

// Mocked config store with the one variable we need. Not persistent.

struct ConfigStore {
    struct Value {
        uint16_t v = 0;
        void set(uint16_t n) {
            v = n;
        }
        uint16_t get() const {
            return v;
        }
    };

    Value mmu_fail_bucket;
};

ConfigStore &config_store();
