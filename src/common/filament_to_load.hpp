#pragma once

#include <filament.hpp>

// TODO: Remove this ugly interface
namespace filament {

FilamentType get_type_to_load();
void set_type_to_load(FilamentType filament);

/// Returns @p filament with @p color applied for a single load.
/// The result is a pending ad-hoc type, so the (possibly system) preset is left untouched.
/// Without a color, @p filament is returned unchanged.
FilamentType apply_load_color(FilamentType filament, VirtualToolIndex virtual_tool, OptionalColor color);

} // namespace filament
