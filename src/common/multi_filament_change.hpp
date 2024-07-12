#pragma once

#include <optional>
#include <array>

#include <filament.hpp>

namespace multi_filament_change {

/// Number of tools shown in the multi filament change menu
static constexpr size_t tool_count = 5;

enum class Action {
    /// Keep as is, do not change the filament
    keep,

    /// Change filament to \p new_filament
    change,

    /// Unload the filament
    unload,
};

struct ConfigItem {
    Action action = Action::keep;
    FilamentType new_filament = FilamentType::none;
    std::optional<Color> color;
};

using Config = std::array<ConfigItem, tool_count>;

} // namespace multi_filament_change

/// Configuration used in DialogChangeAllFilaments
using MultiFilamentChangeConfig = multi_filament_change::Config;
