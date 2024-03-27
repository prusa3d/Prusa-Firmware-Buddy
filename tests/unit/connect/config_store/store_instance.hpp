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

    HostName wifi_hostname;
    HostName lan_hostname;
};

ConfigStore &config_store();
