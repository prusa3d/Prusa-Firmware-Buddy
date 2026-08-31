/// @file
#include "toolchanger.h"
#include "tool_offset.hpp"
#include "dock_position.hpp"

#include <module/planner.h>
#include <Marlin.h>
#include <Marlin/src/feature/bedlevel/bedlevel.h>
#include <timing.h>
#include <bsod/bsod.h>
#include <logging/log.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_power_panic.h>

#if HAS_POWER_PANIC()
    #include <power_panic.hpp>
#endif

LOG_COMPONENT_DEF(PrusaToolChanger, logging::Severity::debug);

PrusaToolChanger prusa_toolchanger;

PrusaToolChangerUtils::PrusaToolChangerUtils() {
    tool_info.fill({ 0, 0 });
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
    const auto &offset = hotend_offset[tool];
    config_store().set_tool_offset(tool, { .x = offset.x, .y = offset.y, .z = offset.z });
}

void PrusaToolChangerUtils::save_tool_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        save_tool_offset(tool);
    }
}

void PrusaToolChangerUtils::load_tool_offsets() {
    for (auto tool : PhysicalToolIndex::all()) {
        const auto offset = config_store().get_tool_offset(tool);
        hotend_offset[tool] = xyz_pos_t { .x = offset.x, .y = offset.y, .z = offset.z };
    }
}

void PrusaToolChangerUtils::set_tool_info(PhysicalToolIndex tool, const PrusaToolInfo &info) {
    tool_info[tool] = info;
}

void PrusaToolChangerUtils::toolchanger_error(const char *message) const {
    fatal_error(message, "PrusaToolChanger");
}

bool PrusaToolChanger::can_move_safely(AxisHomeLevel required_level) {
    // Toolchange requires precise homing, otherwise we might not hit the docks right
    return !axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS), required_level);
}

void PrusaToolChanger::move(const float x, const float y, const feedRate_t feedrate) {
    current_position.x = x;
    current_position.y = y;
    line_to_current_position(feedrate);
}

void PrusaToolChanger::z_shift(const float diff) {
    if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
        MachinePosXYZE target = current_machine_position();
        target.z += diff;
        target.z = std::clamp<float>(target.z, Z_MIN_POS, Z_MAX_POS);
        line_to_machine_pos(target, Z_HOP_FEEDRATE_MM_S);

    } else {
        // Prevent nasty motor skipping
        do_homing_move(Z_AXIS, diff, HOMING_FEEDRATE_INVERTED_Z);
    }

    planner.synchronize();
}

float PrusaToolChanger::calc_z_raise(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool levelling_active) const {
    if (z_lift == tool_change_lift_t::no_lift) {
        return 0.f;
    }

    float min_z = current_position.z;

    // raise above the printed model
    min_z = std::max(min_z, planner.max_printed_z);

    // avoid the workpiece for parking
    if (z_lift == tool_change_lift_t::full_lift) {
        min_z += toolchange_settings.z_raise;
    }

    // account for clearance in the return move (dock_backoff has no Z return, like no_return)
    if (return_type != tool_return_t::no_return && return_type != tool_return_t::dock_backoff) {
        min_z = std::max(min_z, return_position.z);
    }

    float z_raise = (min_z - current_position.z);
    if (levelling_active) {
        z_raise += get_mbl_z_lift_height();
    }
    return z_raise;
}

// This function confuses the indexer, so it is last in the file
bool PrusaToolChangerUtils::wait(const stdext::inplace_function<bool()> &function, uint32_t timeout_ms, WaitMode mode) {
    auto should_bail = [mode]() {
        switch (mode) {
        case WaitMode::default_mode:
            return planner.draining();
        case WaitMode::bail_on_power_panic:
#if HAS_POWER_PANIC()
            return power_panic::panic_is_active();
#else
            return false;
#endif
        }
        bsod_unreachable();
    };

    uint32_t start_time = ticks_ms();
    bool result = false;
    while (!(result = function()) // Wait for this and remember its state for return
        && !should_bail()
        && (ticks_ms() - start_time) < timeout_ms) { // Timeout
        idle(true);
    }
    return result;
}
