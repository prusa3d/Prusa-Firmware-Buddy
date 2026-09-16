#include "filament_to_load.hpp"

static FilamentType filament_to_load = FilamentType::none;

FilamentType filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(FilamentType filament) {
    filament_to_load = filament;
}

FilamentType filament::apply_load_color(FilamentType filament, VirtualToolIndex virtual_tool, OptionalColor color) {
    if (!color.has_value()) {
        return filament;
    }

    // Pending adhoc -> the color applies to this load only,
    // the user preset stays untouched (and a system preset cannot be changed at all)
    filament = filament.to_pending_adhoc(virtual_tool);
    filament.modify_parameters([&](auto &params) { params.color = *color; });
    return filament;
}
