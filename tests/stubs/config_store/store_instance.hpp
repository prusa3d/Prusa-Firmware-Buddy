#pragma once

#include <cstdint>

// Mocked config store with the one variable we need. Not persistent.

struct ConfigStore {
    struct HostName {
        void set(const char *n) {
        }
        const char *get() const {
            return "nice_hostname";
        }
        const char *get_c_str() const {
            return "nice_hostname";
        }
    };

    HostName hostname;

    struct BoolTrue {
        bool get() const {
            return true;
        }
    };

    BoolTrue verify_gcode;
};

inline ConfigStore &config_store() {
    static ConfigStore store;
    return store;
}
