#include "filament_to_load.hpp"

static filament::Type filament_to_load = FilamentType::none;

filament::Type filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(filament::Type filament) {
    filament_to_load = filament;
}

static std::optional<Color> color_to_load { std::nullopt };

std::optional<Color> filament::get_color_to_load() {
    return color_to_load;
}

void filament::set_color_to_load(std::optional<Color> color) {
    color_to_load = color;
}
