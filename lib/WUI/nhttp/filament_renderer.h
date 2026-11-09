#pragma once

#include "segmented_json.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <option/has_anfc.h>
#include <option/has_filament_tracker.h>

namespace nhttp::handler {

class FilamentState {
public:
    /// Values of the tool being rendered.
    ///
    /// Taken once per tool and kept here, so that a response split into
    /// several chunks doesn't mix values read at different times.
    struct Tool {
#if HAS_FILAMENT_TRACKER()
        uint32_t extruded = 0;
        uint32_t unload_count = 0;
        uint32_t extruded_at_unload = 0;
#endif

        bool has_material = false;
        std::array<char, 8> name {};
        std::optional<uint32_t> color_rgb;
        int16_t nozzle_temperature = 0;
        int16_t preheat_temperature = 0;
        int16_t bed_temperature = 0;

#if HAS_ANFC()
        using UUIDBytes = std::array<std::byte, 16>;
        std::optional<UUIDBytes> material_uuid;
        std::optional<UUIDBytes> package_uuid;
        std::optional<UUIDBytes> instance_uuid;

        struct Tag {
            float full_weight;
            float remaining_weight;
            bool full_weight_is_nominal;
        };
        std::optional<Tag> tag;
#endif
    };

    uint8_t iter = 0;
    bool need_comma = false;
    Tool tool;
};

/// Renders the response of `GET /api/v1/filament`: the filament loaded in
/// each enabled tool and how much each tool has extruded since boot.
class FilamentRenderer final : public json::JsonRenderer<FilamentState> {
public:
    FilamentRenderer()
        : JsonRenderer(FilamentState()) {}
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const override;
};

} // namespace nhttp::handler
