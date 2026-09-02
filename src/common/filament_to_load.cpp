#include "filament_to_load.hpp"

static FilamentType filament_to_load = FilamentType::none;

FilamentType filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(FilamentType filament) {
    filament_to_load = filament;
}
