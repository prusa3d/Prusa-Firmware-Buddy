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

    Item<FilamentTypeParameters> adhoc_filament_parameters;
    Item<FilamentTypeParameters> user_filament_parameters;
};

ConfigStoreStub &config_store() {
    static ConfigStoreStub stub;
    return stub;
}
