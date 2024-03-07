#include "custom_filament_tools.hpp"
#include "ini_handler.h"

namespace custom_filament_tools {

CustomFilamentSlots currentslot;

const constexpr char *const CUSTOM_FILAMENT_1_SECTION = "custom_filament_1";
const constexpr char *const CUSTOM_FILAMENT_2_SECTION = "custom_filament_2";
const constexpr char *const CUSTOM_FILAMENT_3_SECTION = "custom_filament_3";
const constexpr char *const CUSTOM_FILAMENT_4_SECTION = "custom_filament_4";

template <typename Enumeration>
auto ToValue(Enumeration const value)
    -> typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

uint8_t SlotToIndex(CustomFilamentSlots slot) {
    return static_cast<uint8_t>(ToValue(slot));
}

uint8_t TempToIndex(CustomFilamentTemperatures temp) {
    return static_cast<uint8_t>(ToValue(temp));
}

void SetCurrentSlot(const CustomFilamentSlots slot) {
    currentslot = slot;
}

CustomFilamentSlots GetCurrentSlot() {
    return currentslot;
}

int16_t GetSlotTemp(const CustomFilamentTemperatures setting) {
    return config_store().custom_filament_temps.get()[SlotToIndex(GetCurrentSlot())].at(TempToIndex(setting));
};

const char *GetSlotName(const CustomFilamentSlots slot) {
    switch (slot) {
    case CustomFilamentSlots::CUSTOM_1:
        return config_store().custom_filament_name_1.get_c_str();
    case CustomFilamentSlots::CUSTOM_2:
        return config_store().custom_filament_name_2.get_c_str();
    case CustomFilamentSlots::CUSTOM_3:
        return config_store().custom_filament_name_3.get_c_str();
    case CustomFilamentSlots::CUSTOM_4:
        return config_store().custom_filament_name_4.get_c_str();
    default:
        break;
    }
    return nullptr;
}

CustomFilamentSlots IndexToSlot(const size_t index) {
    switch (index) {
    case 0:
        return CustomFilamentSlots::CUSTOM_1;
    case 1:
        return CustomFilamentSlots::CUSTOM_2;
    case 2:
        return CustomFilamentSlots::CUSTOM_3;
    case 3:
        return CustomFilamentSlots::CUSTOM_4;
    default:
        return CustomFilamentSlots::END;
    }
}

const char *GetSlotName() {
    return GetSlotName(GetCurrentSlot());
}

void SetSlotName(const char *name) {
    switch (GetCurrentSlot()) {
    case CustomFilamentSlots::CUSTOM_1:
        return config_store().custom_filament_name_1.set(name);
    case CustomFilamentSlots::CUSTOM_2:
        return config_store().custom_filament_name_2.set(name);
    case CustomFilamentSlots::CUSTOM_3:
        return config_store().custom_filament_name_3.set(name);
    case CustomFilamentSlots::CUSTOM_4:
        return config_store().custom_filament_name_4.set(name);
    default:
        break;
    }
}

void SetSlotTemp(const CustomFilamentTemperatures setting, const int16_t temp) {
    config_store().custom_filament_temps.get()[SlotToIndex(GetCurrentSlot())][TempToIndex(setting)] = temp;
};

void set_filament_mask(custom_filament_tools::CustomFilamentConfig *config, const CustomFilamentSlots filament_slot, const char *value, CustomFilamentTemperatures temp_type) {
    const uint8_t slot = SlotToIndex(filament_slot);
    if (slot >= max_filament_slots) {
        return;
    }
    int16_t mask;

    switch (temp_type) {
    case custom_filament_tools::CustomFilamentTemperatures::nozzle_preheat:
        mask = FILAMENT_VAR_NOZZLE_PREHEAT_1;
        break;
    case custom_filament_tools::CustomFilamentTemperatures::nozzle:
        mask = FILAMENT_VAR_NOZZLE_1;
        break;
    case custom_filament_tools::CustomFilamentTemperatures::heatbed:
        mask = FILAMENT_VAR_HEATBED_1;
        break;
    default:
        return;
    }
    char *endptr;
    long tmp = strtol(value, &endptr, 10);
    if (*endptr == '\0' && tmp >= 0 && tmp <= 65535) {
        config->nozzle_preheat[slot] = (uint16_t)tmp;
        config->mask |= FILAMENT_VAR_MSK(mask + slot);
    }
}

void set_filament_name_mask(custom_filament_tools::CustomFilamentConfig *config, const CustomFilamentSlots filament_slot, const char *value) {
    const uint8_t slot = SlotToIndex(filament_slot);
    if (slot >= max_filament_slots) {
        return;
    }

    int16_t mask = FILAMENT_VAR_NAME_1;
    size_t value_len = strlen(value);
    if (value_len <= config_store_ns::max_filament_name_size) {
        strlcpy(config->name[slot], value, sizeof config->name[slot]);
        config->mask |= FILAMENT_VAR_MSK(mask + slot);
    }
}

// TODO: How do we extract some user-friendly error indicator what exactly is wrong?
int connect_ini_handler(void *user, const char *section, const char *name,
    const char *value) {

    auto *config = reinterpret_cast<custom_filament_tools::CustomFilamentConfig *>(user);
    CustomFilamentSlots slot;
    if (strcmp(section, CUSTOM_FILAMENT_1_SECTION) == 0) {
        slot = CustomFilamentSlots::CUSTOM_1;
    } else if (strcmp(section, CUSTOM_FILAMENT_2_SECTION) == 0) {
        slot = CustomFilamentSlots::CUSTOM_2;
    } else if (strcmp(section, CUSTOM_FILAMENT_3_SECTION) == 0) {
        slot = CustomFilamentSlots::CUSTOM_3;
    } else if (strcmp(section, CUSTOM_FILAMENT_4_SECTION) == 0) {
        slot = CustomFilamentSlots::CUSTOM_4;
    } else {
        return 1;
    }

    if (strcmp(name, "name") == 0) {
        set_filament_name_mask(config, slot, value);
    } else if (strcmp(name, "nozzle_preheat") == 0) {
        set_filament_mask(config, slot, value, CustomFilamentTemperatures::nozzle_preheat);
    } else if (strcmp(name, "nozzle") == 0) {
        set_filament_mask(config, slot, value, CustomFilamentTemperatures::nozzle);
    } else if (strcmp(name, "heatbed") == 0) {
        set_filament_mask(config, slot, value, CustomFilamentTemperatures::heatbed);
    }
    return 1;
}

bool load_cfg_from_ini() {
    CustomFilamentConfig config;
    bool ok = ini_parse("/usb/prusa_printer_settings.ini", connect_ini_handler, &config) == 0;
    if (ok) {
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_1)) {
            config_store().custom_filament_name_1.set(config.name[SlotToIndex(CustomFilamentSlots::CUSTOM_1)]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_2)) {
            config_store().custom_filament_name_2.set(config.name[SlotToIndex(CustomFilamentSlots::CUSTOM_2)]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_3)) {
            config_store().custom_filament_name_3.set(config.name[SlotToIndex(CustomFilamentSlots::CUSTOM_3)]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NAME_4)) {
            config_store().custom_filament_name_4.set(config.name[SlotToIndex(CustomFilamentSlots::CUSTOM_4)]);
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_1)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_1)].at(TempToIndex(CustomFilamentTemperatures::nozzle));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_2)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_2)].at(TempToIndex(CustomFilamentTemperatures::nozzle));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_3)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_3)].at(TempToIndex(CustomFilamentTemperatures::nozzle));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_4)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_4)].at(TempToIndex(CustomFilamentTemperatures::nozzle));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_1)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_1)].at(TempToIndex(CustomFilamentTemperatures::nozzle_preheat));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_2)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_2)].at(TempToIndex(CustomFilamentTemperatures::nozzle_preheat));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_3)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_3)].at(TempToIndex(CustomFilamentTemperatures::nozzle_preheat));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_NOZZLE_PREHEAT_4)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_4)].at(TempToIndex(CustomFilamentTemperatures::nozzle_preheat));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_1)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_1)].at(TempToIndex(CustomFilamentTemperatures::heatbed));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_2)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_2)].at(TempToIndex(CustomFilamentTemperatures::heatbed));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_3)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_3)].at(TempToIndex(CustomFilamentTemperatures::heatbed));
        }
        if (config.mask & FILAMENT_VAR_MSK(FILAMENT_VAR_HEATBED_4)) {
            config_store().custom_filament_temps.get()[SlotToIndex(CustomFilamentSlots::CUSTOM_4)].at(TempToIndex(CustomFilamentTemperatures::heatbed));
        }
    }
    return ok;
}

} // namespace custom_filament_tools
