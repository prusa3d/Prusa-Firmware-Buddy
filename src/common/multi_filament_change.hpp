#pragma once

#include <optional>

#include <filament.hpp>
#include <tool_index.hpp>
#include <color.hpp>
#include <utils/compact_optional.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <gcode_basic_parser.hpp>
#include <general_response.hpp>
#include <feature/compatibility_checks/compatibility_checks_common.hpp>

namespace multi_filament_change {

enum class Action : uint8_t {
    /// Keep as is, do not change the filament
    keep,

    /// Change filament to \p new_filament
    change,

    /// Unload the filament
    unload,
};

struct ConfigItem {
    Action action = Action::keep;
    EncodedFilamentType new_filament = FilamentType::none;
    CompactOptional<Color, COLOR_NONE> color;
};

using Config = StrongIndexArray<ConfigItem, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static>;

/// GCode command used to represent the gcode
inline constexpr GCodeCommand gcode_command {
    .letter = 'M',
    .codenum = 9934,
};

/// Constructs a MultiFilamentChange screen configuration based on current print setup - that is GCodeInfo, ToolMapping and SpoolJoin
/// That is, it suggests changing filaments so that they would match the current configuration for the print
Config config_from_current_print_setup();

/// Constructs a MultiFilamentChange configuration from gcode parameters
std::optional<Config> config_from_gcode(GCodeBasicParser &parser);

/// Generates a MultiFilamentChange gcode from the provided configuration
void config_to_gcode(const Config &config, StringBuilder &sb);

/// Pops up warnings for potential filament incompatibilities
/// @returns false if the user pressed abort_response at any point
/// @param skip_level Skips incompatibilities with the provided compatibility level and better
/// !!! To be executed only from the GUI thread
[[nodiscard]] bool gui_config_confirm_incompatibilities(const ConfigItem &config, std::variant<VirtualToolIndex, AllTools> tools, Response abort_response, buddy::compatibility_checks::CompatibilityLevel skip_level = buddy::compatibility_checks::CompatibilityLevel::fully_compatible);

/// Pops up warnings for potential filament incompatibilities
/// @returns false if the user pressed abort_response at any point
/// @param skip_level Skips incompatibilities with the provided compatibility level and better
/// !!! To be executed only from the GUI thread
[[nodiscard]] bool gui_config_confirm_incompatibilities(const Config &config, Response abort_response, buddy::compatibility_checks::CompatibilityLevel skip_level = buddy::compatibility_checks::CompatibilityLevel::fully_compatible);

/// Executes a multi filament change
/// !!! To be called only from the marlin thread
void execute(const Config &config);

} // namespace multi_filament_change

/// Configuration used in DialogChangeAllFilaments
using MultiFilamentChangeConfig = multi_filament_change::Config;
