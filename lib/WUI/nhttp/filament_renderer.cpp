#include "filament_renderer.h"

#include <filament.hpp>
#include <tool_index.hpp>
#include <segmented_json_macros.h>

#if HAS_FILAMENT_TRACKER()
    #include <feature/filament_tracker/filament_tracker.hpp>
#endif
#if HAS_ANFC()
    #include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
    #include <uuid/uuid.hpp>
#endif

#include <algorithm>
#include <cinttypes>

namespace nhttp::handler {

namespace {

    static_assert(sizeof(FilamentState::Tool::name) == sizeof(FilamentTypeParameters::Name));
#if HAS_ANFC()
    static_assert(sizeof(FilamentState::Tool::UUIDBytes) == sizeof(UUID::Bytes));
#endif

    FilamentState::Tool snapshot(VirtualToolIndex tool) {
        FilamentState::Tool result;

#if HAS_FILAMENT_TRACKER()
        // Unloads first, so that extruded is never below extruded_at_unload
        const auto unloads = buddy::filament_tracker().get_unloads(tool);
        result.unload_count = unloads.count;
        result.extruded_at_unload = unloads.extruded_at_last;
        result.extruded = buddy::filament_tracker().get_extruded_distance(tool);
#endif

        if (const FilamentType type = FilamentType::for_tool(tool)) {
            const FilamentTypeParameters params = type.parameters();
            result.has_material = true;
            std::copy_n(params.name.data(), result.name.size(), result.name.begin());
            result.name.back() = '\0';
            result.nozzle_temperature = params.nozzle_temperature;
            result.preheat_temperature = params.nozzle_preheat_temperature;
            result.bed_temperature = params.heatbed_temperature;
            if (params.color.has_value()) {
                // Color keeps 0x00RRGGBB in its raw value
                result.color_rgb = params.color->raw & 0xffffff;
            }
#if HAS_ANFC()
            const auto bytes = [](const FilamentTypeParameters::OptionalUUID &uuid) -> std::optional<FilamentState::Tool::UUIDBytes> {
                if (!uuid.has_value()) {
                    return std::nullopt;
                }
                FilamentState::Tool::UUIDBytes out;
                std::ranges::copy(uuid->bytes, out.begin());
                return out;
            };
            result.material_uuid = bytes(params.material_uuid);
            result.package_uuid = bytes(params.package_uuid);
            result.instance_uuid = bytes(params.instance_uuid);
#endif
        }

#if HAS_ANFC()
        if (const auto report = buddy::openprinttag::filament_usage_tracker().filament_report(tool)) {
            result.tag = FilamentState::Tool::Tag {
                .full_weight = report->full_weight_g,
                .remaining_weight = report->remaining_weight_g,
                .full_weight_is_nominal = report->full_weight_is_nominal,
            };
        }
#endif

        return result;
    }

} // namespace

json::JsonResult FilamentRenderer::renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const {
#if HAS_ANFC()
    std::array<char, UUID::string_length> uuid_buffer;
    const auto uuid_str = [&](const FilamentState::Tool::UUIDBytes &bytes) {
        return UUID::from_bytes(bytes).to_string(uuid_buffer);
    };
#endif
    const auto &tool = state.tool;

    // Keep the indentation of the JSON in here!
    // clang-format off
    JSON_START;
    JSON_OBJ_START;
#if HAS_FILAMENT_TRACKER()
        JSON_FIELD_INT("boot_id", buddy::filament_tracker().extruded_distance_id()) JSON_COMMA;
#endif
        JSON_FIELD_ARR("tools");
        for (state.iter = 0; state.iter < VirtualToolIndex::count; state.iter++) {
            // Snapshot once per tool: a resumed chunk jumps past this point
            if (!VirtualToolIndex::from_raw(state.iter).is_enabled()) {
                continue;
            }
            state.tool = snapshot(VirtualToolIndex::from_raw(state.iter));

            if (state.need_comma) {
                JSON_COMMA;
            }
            state.need_comma = true;

            JSON_OBJ_START;
                JSON_FIELD_INT("index", state.iter);
#if HAS_FILAMENT_TRACKER()
                JSON_COMMA;
                JSON_FIELD_INT("extruded", tool.extruded) JSON_COMMA;
                JSON_FIELD_INT("unload_count", tool.unload_count) JSON_COMMA;
                JSON_FIELD_INT("extruded_at_unload", tool.extruded_at_unload);
#endif
                if (tool.has_material) {
                    JSON_COMMA;
                    JSON_FIELD_OBJ("material");
                        JSON_FIELD_STR("name", tool.name.data()) JSON_COMMA;
                        JSON_FIELD_INT("nozzle_temperature", tool.nozzle_temperature) JSON_COMMA;
                        JSON_FIELD_INT("preheat_temperature", tool.preheat_temperature) JSON_COMMA;
                        JSON_FIELD_INT("bed_temperature", tool.bed_temperature);
                        if (tool.color_rgb.has_value()) {
                            JSON_COMMA;
                            JSON_FIELD_STR_FORMAT("color", "#%06" PRIx32, *tool.color_rgb);
                        }
#if HAS_ANFC()
                        if (tool.material_uuid.has_value()) {
                            JSON_COMMA;
                            JSON_FIELD_STR("material_uuid", uuid_str(*tool.material_uuid));
                        }
                        if (tool.package_uuid.has_value()) {
                            JSON_COMMA;
                            JSON_FIELD_STR("package_uuid", uuid_str(*tool.package_uuid));
                        }
                        if (tool.instance_uuid.has_value()) {
                            JSON_COMMA;
                            JSON_FIELD_STR("instance_uuid", uuid_str(*tool.instance_uuid));
                        }
#endif
                    JSON_OBJ_END;
                }
#if HAS_ANFC()
                if (tool.tag.has_value()) {
                    JSON_COMMA;
                    JSON_FIELD_OBJ("tag");
                        JSON_FIELD_FFIXED("full_weight", tool.tag->full_weight, 1) JSON_COMMA;
                        JSON_FIELD_BOOL("full_weight_is_nominal", tool.tag->full_weight_is_nominal) JSON_COMMA;
                        JSON_FIELD_FFIXED("remaining_weight", tool.tag->remaining_weight, 1);
                    JSON_OBJ_END;
                }
#endif
            JSON_OBJ_END;
        }
        JSON_ARR_END;
    JSON_OBJ_END;
    JSON_END;
    // clang-format on
}

} // namespace nhttp::handler
