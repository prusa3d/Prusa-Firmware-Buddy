#pragma once

#include <filament.hpp>

// TODO: Remove this ugly interface
namespace filament {

FilamentType get_type_to_load();
void set_type_to_load(FilamentType filament);

} // namespace filament
