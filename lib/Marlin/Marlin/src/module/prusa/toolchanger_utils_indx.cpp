#include "toolchanger_utils.h"
#include "tool_offset.hpp"
#include "dock_position.hpp"
#include <tool_index.hpp>

#include <option/has_indx.h>
#include <option/has_toolchanger.h>

#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/motion.h"
#include "Marlin/src/feature/bedlevel/bedlevel.h"
#include "Marlin.h"
#include <logging/log.hpp>
#include "timing.h"
#include <tool/hotend/hotend/indx_hotend.hpp>
#include <utils/badge.hpp>

#include <config_store/store_instance.hpp>

LOG_COMPONENT_DEF(PrusaToolChanger, logging::Severity::debug);

// using namespace buddy::puppies;

bool PrusaToolChangerUtils::is_pos_in_toolchange_area(const xy_pos_t &pos) {
    return pos.y < SAFE_Y_WITH_TOOL;
}

float PrusaToolChangerUtils::limit_stealth_feedrate(float feedrate) {
    // If the HWLIMIT_STEALTH_MAX_FEEDRATE changes, this function needs to be revisited
    static_assert(std::to_array(HWLIMIT_STEALTH_MAX_FEEDRATE) == std::to_array({ 140, 140, 12, 100 }));

    // In stealth mode, various travel speeds get reduced to HWLIMIT_STEALTH_MAX_FEEDRATE, which is 140 mm/s.
    // Unfortunately, the printer has some ugly resonancies when moving at this speed.
    // Changing the stealth feedrate was not allowed.
    // So instead, we're further lowering the travel feedrates in stealth mode.
    // BFW-5496
    return config_store().stealth_mode.get() ? std::min<float>(feedrate, 80) : feedrate;
}

void PrusaToolChangerUtils::set_active_extruder(std::variant<PhysicalToolIndex, NoTool> maybe_tool) {
    // Pickup/park flip heating earlier for finer timing.
    // Boot, PP resume, and dropped-nozzle recovery skip pickup/park, we have to handle the heating transition here too.
    match(
        maybe_tool,
        [&](PhysicalToolIndex tool) {
            active_extruder = tool.to_raw();
            // any active-tool change reapplies the matching offset
            hotend_currently_applied_offset = hotend_offset[tool];

            IndxHotend &to_be_active_hotend = IndxHotend::indx_tool(tool).hotend();
            if (!to_be_active_hotend.is_thermally_managed()) {
                to_be_active_hotend.start_heating();
            }
        },
        [&](NoTool) {
            // order is important here: currently_selected() reads active_extruder
            if (auto curr = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected())) {
                auto &h = IndxHotend::indx_tool(*curr).hotend();
                if (h.is_thermally_managed()) {
                    h.stop_heating();
                }
            }
            active_extruder = MARLIN_NO_TOOL_PICKED;
            hotend_currently_applied_offset = xyz_pos_t {};
        });

    IndxHotend::assert_thermally_managed_invariant(maybe_tool);
}

PrusaToolChangerUtils::PrusaToolChangerUtils() {
    tool_info.fill({ 0, 0 });
}

void PrusaToolChangerUtils::restore_last_picked_tool() {
    // INDX has no sensors; trust EEPROM for the last picked tool
    if (config_store().indx_last_picked_tool_valid.get()) {
        set_active_extruder(config_store().get_indx_last_picked_tool());
    } else {
        set_active_extruder(NoTool {});
    }
}

bool PrusaToolChangerUtils::is_tool_enabled(PhysicalToolIndex tool, Badge<PhysicalToolIndex>) {
    return config_store().indx_dock_calibrated_mask.get().test(tool.to_raw());
}

uint8_t PrusaToolChangerUtils::get_num_enabled_tools() const {
    return config_store().indx_dock_calibrated_mask.get().count();
}

float PrusaToolChangerUtils::get_mbl_z_lift_height() const {
    // Get maximal Z of MBL
    float mbl_max_z_height = std::numeric_limits<float>::lowest();
    float mbl_min_z_height = std::numeric_limits<float>::max();
    for (uint8_t x = 0; x < GRID_MAX_POINTS_X; x++) {
        for (uint8_t y = 0; y < GRID_MAX_POINTS_Y; y++) {
            if (const float z = Z_VALUES(x, y); !isnan(z)) {
                mbl_min_z_height = std::min(mbl_min_z_height, z);
                mbl_max_z_height = std::max(mbl_max_z_height, z);
            }
        }
    }
    return mbl_max_z_height - mbl_min_z_height;
}

void PrusaToolChangerUtils::load_tool_info() {
    for (auto tool : PhysicalToolIndex::all()) {
        DockPosition position = config_store().get_dock_position(tool);
        tool_info[tool].dock_x = position.x;
        tool_info[tool].dock_y = position.y;
    }
}

void PrusaToolChangerUtils::save_tool_info() {
    for (auto tool : PhysicalToolIndex::all()) {
        config_store().set_dock_position(tool, { .x = tool_info[tool].dock_x, .y = tool_info[tool].dock_y });
    }
}

void PrusaToolChangerUtils::save_tool_offset(PhysicalToolIndex tool) {
    config_store().set_tool_offset(tool, { .x = hotend_offset[tool].x, .y = hotend_offset[tool].y, .z = hotend_offset[tool].z });
}

void PrusaToolChangerUtils::save_tool_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        save_tool_offset(tool);
    }
}

void PrusaToolChangerUtils::load_tool_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        ToolOffset offset = config_store().get_tool_offset(tool);
        hotend_offset[tool].x = offset.x;
        hotend_offset[tool].y = offset.y;
        hotend_offset[tool].z = offset.z;
    }
}

const PrusaToolInfo &PrusaToolChangerUtils::get_tool_info(PhysicalToolIndex tool_index, bool check_calibrated) const {
    const PrusaToolInfo &info = tool_info[tool_index];
    if (check_calibrated && !is_tool_info_valid(tool_index, info)) {
        toolchanger_error("Dock Position not calibrated");
    }
    return info;
}

bool PrusaToolChangerUtils::is_tool_info_valid(PhysicalToolIndex tool, const PrusaToolInfo &info) const {
    const PrusaToolInfo synthetic = create_default_tool_info(tool);
    const auto dx = std::abs(info.dock_x - synthetic.dock_x);
    const auto dy = std::abs(info.dock_y - synthetic.dock_y);
    return dx <= (DOCK_INVALID_OFFSET_X_MM + EPSILON_MM) && dy <= (DOCK_INVALID_OFFSET_Y_MM + EPSILON_MM);
}

void PrusaToolChangerUtils::set_tool_info(PhysicalToolIndex tool, const PrusaToolInfo &info) {
    tool_info[tool] = info;
}

void PrusaToolChangerUtils::toolchanger_error(const char *message) const {
    fatal_error(message, "PrusaToolChanger");
}

PrusaToolInfo PrusaToolChangerUtils::create_default_tool_info(PhysicalToolIndex tool) const {
    return PrusaToolInfo({ .dock_x = DOCK_DEFAULT_X_MM[tool],
        .dock_y = DOCK_DEFAULT_Y_MM });
}
