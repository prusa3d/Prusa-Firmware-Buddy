#pragma once

#include <bitset>

#include <filament.hpp>

struct ConfigStoreStub {
    template <typename T>
    struct Item {
        T get() { return T(); }
        T get(auto) { return T(); }

        void set(auto) {}
        void set(auto, auto) {}

        void apply(auto) {}
    };

    Item<std::bitset<max_preset_filament_type_count>> visible_preset_filament_types;
    Item<std::bitset<max_preset_filament_type_count>> visible_user_filament_types;

    Item<FilamentTypeParameters_EEPROM1> adhoc_filament_parameters;
    Item<FilamentTypeParameters_EEPROM5> adhoc_filament_parameters_5;

    Item<FilamentTypeParameters_EEPROM1> user_filament_parameters;
};

ConfigStoreStub &config_store() {
    static ConfigStoreStub stub;
    return stub;
}
