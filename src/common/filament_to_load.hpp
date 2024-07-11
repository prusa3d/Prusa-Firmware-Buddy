#pragma once

#include <filament.hpp>
#include <color.hpp>

// TODO: Remove this ugly interface
namespace filament {

Type get_type_to_load();
void set_type_to_load(Type filament);

std::optional<Color> get_color_to_load();
void set_color_to_load(std::optional<Color> color);

} // namespace filament
