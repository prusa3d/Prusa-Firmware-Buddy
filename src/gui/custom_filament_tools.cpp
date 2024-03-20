#include "custom_filament_tools.hpp"
#include "ini_handler.h"

namespace custom_filament_tools {

int8_t currentslot;

const constexpr char *const CUSTOM_FILAMENT_SECTION = "custom_filament";

void SetCurrentSlot(const int8_t slot) {
    currentslot = slot;
}

int8_t GetCurrentSlot() {
    return currentslot;
}

int16_t GetSlotTemp(const CustomFilamentTemperatures setting) {
    if (setting == CustomFilamentTemperatures::nozzle) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_nozzle_1.get();
        case 1:
            return config_store().custom_filament_nozzle_2.get();
        case 2:
            return config_store().custom_filament_nozzle_3.get();
        case 3:
            return config_store().custom_filament_nozzle_4.get();
        default:
            assert("Unknown custom filament slot");
            break;
        }
    } else if (setting == CustomFilamentTemperatures::nozzle_preheat) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_nozzle_preheat_1.get();
        case 1:
            return config_store().custom_filament_nozzle_preheat_2.get();
        case 2:
            return config_store().custom_filament_nozzle_preheat_3.get();
        case 3:
            return config_store().custom_filament_nozzle_preheat_4.get();
        default:
            assert("Unknown custom filament slot");
            break;
        }
    } else if (setting == CustomFilamentTemperatures::heatbed) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_heatbed_1.get();
        case 1:
            return config_store().custom_filament_heatbed_2.get();
        case 2:
            return config_store().custom_filament_heatbed_3.get();
        case 3:
            return config_store().custom_filament_heatbed_4.get();
        default:
            assert("Unknown custom filament slot");
            break;
        }
    }
    return 0;
};

const char *GetSlotName(const int slot) {
    switch (slot) {
    case 0:
        return config_store().custom_filament_name_1.get_c_str();
    case 1:
        return config_store().custom_filament_name_2.get_c_str();
    case 2:
        return config_store().custom_filament_name_3.get_c_str();
    case 3:
        return config_store().custom_filament_name_4.get_c_str();
    default:
        break;
    }
    return nullptr;
}

const char *GetSlotName() {
    return GetSlotName(GetCurrentSlot());
}

void SetSlotName(const char *name) {
    switch (GetCurrentSlot()) {
    case 0:
        return config_store().custom_filament_name_1.set(name);
    case 1:
        return config_store().custom_filament_name_2.set(name);
    case 2:
        return config_store().custom_filament_name_3.set(name);
    case 3:
        return config_store().custom_filament_name_4.set(name);
    default:
        break;
    }
}

void SetSlotTemp(const CustomFilamentTemperatures setting, const int16_t temp) {
    if (setting == CustomFilamentTemperatures::nozzle) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_nozzle_1.set(temp);
        case 1:
            return config_store().custom_filament_nozzle_2.set(temp);
        case 2:
            return config_store().custom_filament_nozzle_3.set(temp);
        case 3:
            return config_store().custom_filament_nozzle_4.set(temp);
        default:
            break;
        }
    } else if (setting == CustomFilamentTemperatures::nozzle_preheat) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_nozzle_preheat_1.set(temp);
        case 1:
            return config_store().custom_filament_nozzle_preheat_2.set(temp);
        case 2:
            return config_store().custom_filament_nozzle_preheat_3.set(temp);
        case 3:
            return config_store().custom_filament_nozzle_preheat_4.set(temp);
        default:
            break;
        }
    } else if (setting == CustomFilamentTemperatures::heatbed) {
        switch (GetCurrentSlot()) {
        case 0:
            return config_store().custom_filament_heatbed_1.set(temp);
        case 1:
            return config_store().custom_filament_heatbed_2.set(temp);
        case 2:
            return config_store().custom_filament_heatbed_3.set(temp);
        case 3:
            return config_store().custom_filament_heatbed_4.set(temp);
        default:
            break;
        }
    }
};

bool ini_string_match(const char *section, const char *section_var,
    const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

// TODO: How do we extract some user-friendly error indicator what exactly is wrong?
int connect_ini_handler(void *user, const char *section, const char *name,
    const char *value) {

    auto *config = reinterpret_cast<custom_filament_tools::CustomFilamentConfig *>(user);
    size_t len = strlen(value);

    if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "name_1")) {
        if (len <= config_store_ns::max_filament_name_size) {
            strlcpy(config->name[0], value, sizeof config->name[0]);
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_1);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "name_2")) {
        if (len <= config_store_ns::max_filament_name_size) {
            strlcpy(config->name[1], value, sizeof config->name[1]);
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_2);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "name_3")) {
        if (len <= config_store_ns::max_filament_name_size) {
            strlcpy(config->name[2], value, sizeof config->name[2]);
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_3);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "name_4")) {
        if (len <= config_store_ns::max_filament_name_size) {
            strlcpy(config->name[3], value, sizeof config->name[3]);
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_4);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_preheat_1")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle_preheat[0] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_1);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_preheat_2")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle_preheat[1] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_2);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_preheat_3")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle_preheat[2] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_3);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_preheat_4")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle_preheat[3] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_4);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_1")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle[0] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_1);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_2")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle[1] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_2);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_3")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle[2] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_3);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "nozzle_4")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->nozzle[3] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_4);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "heatbed_1")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->heatbed[0] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_1);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "heatbed_2")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->heatbed[1] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_2);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "heatbed_3")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->heatbed[2] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_3);
        }
    } else if (ini_string_match(section, CUSTOM_FILAMENT_SECTION, name, "heatbed_4")) {
        char *endptr;
        long tmp = strtol(value, &endptr, 10);
        if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
            config->heatbed[3] = (uint16_t)tmp;
            config->mask |= FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_4);
        }
    }
    return 1;
}

bool load_cfg_from_ini() {
    CustomFilamentConfig config;
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    if (ok) {
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_1)) {
            config_store().custom_filament_name_1.set(config.name[0]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_2)) {
            config_store().custom_filament_name_2.set(config.name[1]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_3)) {
            config_store().custom_filament_name_3.set(config.name[2]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_4)) {
            config_store().custom_filament_name_4.set(config.name[3]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_1)) {
            config_store().custom_filament_nozzle_1.set(config.nozzle[0]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_2)) {
            config_store().custom_filament_nozzle_2.set(config.nozzle[1]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_3)) {
            config_store().custom_filament_nozzle_3.set(config.nozzle[2]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_4)) {
            config_store().custom_filament_nozzle_4.set(config.nozzle[3]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_1)) {
            config_store().custom_filament_nozzle_preheat_1.set(config.nozzle_preheat[0]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_2)) {
            config_store().custom_filament_nozzle_preheat_2.set(config.nozzle_preheat[1]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_3)) {
            config_store().custom_filament_nozzle_preheat_3.set(config.nozzle_preheat[2]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_4)) {
            config_store().custom_filament_nozzle_preheat_4.set(config.nozzle_preheat[3]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_1)) {
            config_store().custom_filament_heatbed_1.set(config.heatbed[0]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_2)) {
            config_store().custom_filament_heatbed_2.set(config.heatbed[1]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_3)) {
            config_store().custom_filament_heatbed_3.set(config.heatbed[2]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_4)) {
            config_store().custom_filament_heatbed_4.set(config.heatbed[3]);
        }
    }
    return ok;
}

} // namespace custom_filament_tools
