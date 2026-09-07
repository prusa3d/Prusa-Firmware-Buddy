#include "toolchanger_utils.h"
#include "tool_offset.hpp"
#include "dock_position.hpp"
#include "utils/variant_utils.hpp"
#include <feature/gcode_exception/gcode_exception.hpp>
#include <option/has_crash_detection.h>
#include <option/has_toolchanger.h>
#include <tool_index.hpp>
#include <bsod/bsod.h>

#if HAS_TOOLCHANGER()
    #include "Marlin/src/module/stepper.h"
    #include "Marlin/src/feature/bedlevel/bedlevel.h"
    #include "Marlin.h"
    #include <logging/log.hpp>
    #include "timing.h"
    #include <puppies/Dwarf.hpp>
    #include <puppies/PuppyModbus.hpp>

    #if HAS_CRASH_DETECTION()
        #include "../../feature/prusa/crash_recovery.hpp"
    #endif

    #include <config_store/store_instance.hpp>

LOG_COMPONENT_DEF(PrusaToolChanger, logging::Severity::debug);

using namespace buddy::puppies;

static_assert(EXTRUDERS == dwarfs.size());

bool PrusaToolChangerUtils::is_pos_in_toolchange_area(const xy_pos_t &pos) {
    return pos.y > SAFE_Y_WITH_TOOL;
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

PrusaToolChangerUtils::PrusaToolChangerUtils() {
    tool_info.fill({ 0, 0 });
}

bool PrusaToolChangerUtils::init(PuppyModbus &bus, bool first_run) {
    if (first_run) {
        toolchanger_enabled = autodetect_toolchanger_enabled();

        if (toolchanger_enabled == false) {
            if (dwarfs[PhysicalToolIndex::from_raw(0)].set_selected(bus, true) == CommunicationStatus::ERROR) {
                return false;
            }
        }
    }

    load_tool_info();

    // Reactivate active dwarf on reinit
    if (active_dwarf) {
        request_toolchange_dwarf = active_dwarf.load();
        active_dwarf = nullptr;
        request_toolchange = true;
    }

    // Update picked tool and optionally select active dwarf
    if (update(bus) == false) {
        return false;
    }

    // Force toolchange after reset to properly init marlin's tool variables
    if (first_run && toolchanger_enabled) {
        force_marlin_picked_tool(nullptr);
        force_toolchange_gcode = true; // This needs to be set after update()
    }

    return true;
}

bool PrusaToolChangerUtils::autodetect_toolchanger_enabled() {
    // This will detect whenever printer will be threated as multitool or singletool printer. Single tool means tool is firmly attached to effector, no toolchanger mechanism.

    // Detection is done under assumption that if there is single dwarf, it has to be connected to DWARF1 connector, otherwise PuppyBootstrap will not boot.
    // if multiple dwarfs are connected, printer is multitool

    uint8_t num_dwarfs = 0;
    for (Dwarf &dwarf : dwarfs) {
        if (dwarf.is_enabled()) {
            ++num_dwarfs;
        }
    }

    if (num_dwarfs == 1) {
        log_info(PrusaToolChanger, "Initializing as single tool printer");
        return false;
    } else {
        // any other number of dwarfs means multitool (0 dwarfs is not allowed and will not get through PuppyBootstrap)
        log_info(PrusaToolChanger, "Initializing with toolchanger");
        return true;
    }
}

void PrusaToolChangerUtils::autodetect_active_tool(PuppyModbus &bus) {
    if (!is_toolchanger_enabled()) { // Ignore on singletool
        auto &first_dwarf = dwarfs[PhysicalToolIndex::from_raw(0)];
        picked_dwarf = &first_dwarf;
        if (!first_dwarf.is_selected()) {
            first_dwarf.set_selected(bus, true);
        }
        return;
    }

    Dwarf *active = nullptr;
    for (Dwarf &dwarf : dwarfs) {
        if (dwarf.is_enabled()) {
            if (dwarf.is_picked() && (dwarf.is_parked() == false)) { // Dwarf needs to be picked and not parked to be really "picked"
                if (active != nullptr) {
                    toolchanger_error("Multiple dwarfs are picked");
                }
                active = &dwarf;
            }
        }
    }
    picked_dwarf = active;
    picked_update = true;
}

void PrusaToolChangerUtils::request_active_switch(Dwarf *new_dwarf) {
    debug_assert(request_toolchange == false && "Repeated dwarf switch request");
    request_toolchange_dwarf = new_dwarf;
    request_toolchange = true;
    if (wait([this]() { return !this->request_toolchange.load(); }, WAIT_TIME_TOOL_SELECT) == false) {
        // wait() bails out as soon as the planner starts draining, which is what a failed wait
        // means here far more often than a puppy task that failed to answer. No error may be
        // raised while draining - the request stays pending and the puppy task still applies it.
        if (gcode_exceptions().is_unwinding()) {
            return;
        }
    #if HAS_CRASH_DETECTION()
        if (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) {
            return; // Fail silently, so powerpanic can work
        }
    #endif
        toolchanger_error("Tool switch failed");
    }
}

bool PrusaToolChangerUtils::is_tool_enabled(PhysicalToolIndex tool, Badge<PhysicalToolIndex>) {
    return buddy::puppies::dwarfs[tool].is_enabled();
}

bool PrusaToolChangerUtils::update(PuppyModbus &bus) {
    if (request_toolchange) {
        // Make requested tool active
        Dwarf *old_tool = active_dwarf.load();
        Dwarf *new_tool = request_toolchange_dwarf.load();
        if (old_tool != new_tool) {
            if (old_tool) {
                if (old_tool->set_selected(bus, false) == CommunicationStatus::ERROR) {
                    return false;
                }
                log_info(PrusaToolChanger, "Deactivated Dwarf #%u", old_tool->dwarf_index());

                active_dwarf = nullptr; // No dwarf is selected right now
                loadcell.Clear(); // No loadcell is available now, make sure that it is not stuck in active mode
            }
            if (new_tool) {
                if (new_tool->set_selected(bus, true) == CommunicationStatus::ERROR) {
                    return false;
                }
                log_info(PrusaToolChanger, "Activated Dwarf #%u", new_tool->dwarf_index());

                active_dwarf = new_tool; // New tool is necessary for stepperE0.push()
                stepperE0.push(); // Write current stepper settings
            }
        }
        // Update physically picked tool before clearing the request
        autodetect_active_tool(bus);
        request_toolchange = false;
        return true;
    }

    // Update physically picked tool
    autodetect_active_tool(bus);
    force_marlin_picked_tool(picked_dwarf);
    return true;
}

uint8_t PrusaToolChangerUtils::get_num_enabled_tools() const {
    return std::ranges::count_if(dwarfs, [](const auto &dwarf) { return dwarf.is_enabled(); });
}

Dwarf *PrusaToolChangerUtils::get_marlin_picked_tool() {
    return match(
        PhysicalToolIndex::currently_selected(),
        [](PhysicalToolIndex physical_tool) { return &dwarfs[physical_tool]; },
        [](NoTool) -> Dwarf * { return nullptr; });
}

void PrusaToolChangerUtils::force_marlin_picked_tool(Dwarf *dwarf) {
    if (dwarf == nullptr) {
        active_extruder = MARLIN_NO_TOOL_PICKED;
    } else {
        active_extruder = dwarf->dwarf_index();
    }
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

uint16_t PrusaToolChangerUtils::detect_tool_nr() {
    Dwarf *dwarf = picked_dwarf.load();
    if (dwarf) {
        return dwarf->dwarf_index();
    } else {
        return MARLIN_NO_TOOL_PICKED;
    }
}

uint16_t PrusaToolChangerUtils::get_enabled_mask() {
    static_assert(PhysicalToolIndex::count < 16, "Using uint16_t as a mask of dwarves");
    uint16_t mask = 0;

    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        mask |= (0x01 << tool.to_raw());
    }

    return mask;
}

uint16_t PrusaToolChangerUtils::get_parked_mask() {
    static_assert(PhysicalToolIndex::count < 16, "Using uint16_t as a mask of dwarves");
    uint16_t mask = 0;

    for (auto tool : PhysicalToolIndex::all()) {
        if (dwarfs[tool].is_enabled() && !dwarfs[tool].is_picked() && dwarfs[tool].is_parked()) {
            mask |= (0x01 << tool.to_raw());
        }
    }

    return mask;
}

buddy::puppies::Dwarf &PrusaToolChangerUtils::getActiveToolOrFirst() {
    auto active = active_dwarf.load();
    return active ? *active : dwarfs[PhysicalToolIndex::from_raw(0)];
}

buddy::puppies::Dwarf &PrusaToolChangerUtils::getTool(PhysicalToolIndex tool_index) {
    return buddy::puppies::dwarfs[tool_index];
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

const PrusaToolInfo &PrusaToolChangerUtils::get_tool_info(PhysicalToolIndex tool, bool check_calibrated) const {
    const PrusaToolInfo &info = tool_info[tool.to_raw()];

    if (check_calibrated && (std::isnan(info.dock_x) || std::isnan(info.dock_y) || info.dock_x == 0 || info.dock_y == 0)) {
        toolchanger_error("Dock Position not calibrated");
    }

    return info;
}

bool PrusaToolChangerUtils::is_tool_info_valid(PhysicalToolIndex tool, const PrusaToolInfo &info) const {
    const PrusaToolInfo synthetic = create_default_tool_info(tool);
    const auto dx = info.dock_x - synthetic.dock_x;
    const auto dy = info.dock_y - synthetic.dock_y;
    return sqrt(pow(dx, 2) + pow(dy, 2)) < DOCK_INVALID_OFFSET_MM;
}

void PrusaToolChangerUtils::set_tool_info(PhysicalToolIndex tool, const PrusaToolInfo &info) {
    tool_info[tool.to_raw()] = info;
}

void PrusaToolChangerUtils::toolchanger_error(const char *message) const {
    fatal_error(message, "PrusaToolChanger");
}

PrusaToolChangerUtils::StepperConfigGuard::StepperConfigGuard() {
    x_current_ma = stepperX.rms_current();
    x_stall_sensitivity = stepperX.stall_sensitivity();
    y_current_ma = stepperY.rms_current();
    y_stall_sensitivity = stepperY.stall_sensitivity();
    stepperX.rms_current(PARKING_CURRENT_MA);
    stepperX.stall_sensitivity(PARKING_STALL_SENSITIVITY);
    stepperY.rms_current(PARKING_CURRENT_MA);
    stepperY.stall_sensitivity(PARKING_STALL_SENSITIVITY);
}

PrusaToolChangerUtils::StepperConfigGuard::~StepperConfigGuard() {
    stepperX.rms_current(x_current_ma);
    stepperX.stall_sensitivity(x_stall_sensitivity);
    stepperY.rms_current(y_current_ma);
    stepperY.stall_sensitivity(y_stall_sensitivity);
}

PrusaToolInfo PrusaToolChangerUtils::create_default_tool_info(PhysicalToolIndex tool) const {
    return PrusaToolInfo({
        .dock_x = DOCK_DEFAULT_FIRST_X_MM + DOCK_OFFSET_X_MM * tool.to_raw(),
        .dock_y = DOCK_DEFAULT_Y_MM,
    });
}

void PrusaToolChangerUtils::ConfRestorer::sample() {
    if (sampled) {
        bsod("Double sampled planner configuration");
    }
    #if HAS_CLASSIC_JERK
    // #error dead code found by automatic analyses (see BFW-5461)
    sampled_jerk = planner.settings.max_jerk;
    #endif
    sampled_travel_acceleration = planner.settings.travel_acceleration;
    sampled_feedrate_mm_s = feedrate_mm_s;
    sampled_feedrate_percentage = feedrate_percentage;
    sampled = true;
}

void PrusaToolChangerUtils::ConfRestorer::restore_clear() {
    restore();
    sampled = false;
}

void PrusaToolChangerUtils::ConfRestorer::restore_jerk() {
    if (!sampled.load()) {
        bsod("Restoring not sampled jerk");
    }

    auto s = planner.user_settings;
    #if HAS_CLASSIC_JERK
    // #error dead code found by automatic analyses (see BFW-5461)
    s.max_jerk = sampled_jerk;
    #endif
    planner.apply_settings(s);
}

void PrusaToolChangerUtils::ConfRestorer::restore_acceleration() {
    if (!sampled.load()) {
        bsod("Restoring not sampled acceleration");
    }

    auto s = planner.user_settings;
    s.travel_acceleration = sampled_travel_acceleration;
    planner.apply_settings(s);
}

void PrusaToolChangerUtils::ConfRestorer::restore_feedrate() {
    if (!sampled.load()) {
        bsod("Restoring not sampled feedrate");
    }
    feedrate_mm_s = sampled_feedrate_mm_s;
    feedrate_percentage = sampled_feedrate_percentage;
}

#endif
