#include "toolchanger_utils.h"
#include "tool_offset.hpp"
#include "dock_position.hpp"

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "Marlin/src/module/stepper.h"
    #include "Marlin/src/feature/bedlevel/bedlevel.h"
    #include "Marlin.h"
    #include <logging/log.hpp>
    #include "timing.h"
    #include <option/is_knoblet.h>
    #include <puppies/Dwarf.hpp>

    #if ENABLED(CRASH_RECOVERY)
        #include "../../feature/prusa/crash_recovery.hpp"
    #endif /*ENABLED(CRASH_RECOVERY)*/

    #include <config_store/store_instance.hpp>

LOG_COMPONENT_DEF(PrusaToolChanger, logging::Severity::debug);

using namespace buddy::puppies;

static_assert(EXTRUDERS == dwarfs.size());

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

bool PrusaToolChangerUtils::init(bool first_run) {
    if (first_run) {
        toolchanger_enabled = autodetect_toolchanger_enabled();

        if (toolchanger_enabled == false) {
            if (dwarfs[0].set_selected(true) == CommunicationStatus::ERROR) {
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
    if (update() == false) {
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

void PrusaToolChangerUtils::autodetect_active_tool() {
    if (!is_toolchanger_enabled()) { // Ignore on singletool
        picked_dwarf = &dwarfs[0];
        if (!dwarfs[0].is_selected()) {
            dwarfs[0].set_selected(true);
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
                if constexpr (option::is_knoblet) {
                    // In knoblet build without cheese board, all dwarfs appear picked.
                    // So we will act as if only first dwarf is picked, to avoid "Multiple dwarfs are picked" redscreen
                    break;
                }
            }
        }
    }
    picked_dwarf = active;
    picked_update = true;
}

void PrusaToolChangerUtils::request_active_switch(Dwarf *new_dwarf) {
    assert(request_toolchange == false && "Repeated dwarf switch request");
    request_toolchange_dwarf = new_dwarf;
    request_toolchange = true;
    if (wait([this]() { return !this->request_toolchange.load(); }, WAIT_TIME_TOOL_SELECT) == false) {
    #if ENABLED(CRASH_RECOVERY)
        if (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) {
            return; // Fail silently, so powerpanic can work
        }
    #endif /*ENABLED(CRASH_RECOVERY)*/
        toolchanger_error("Tool switch failed");
    }
}

bool PrusaToolChangerUtils::update() {
    if (request_toolchange) {
        // Make requested tool active
        Dwarf *old_tool = active_dwarf.load();
        Dwarf *new_tool = request_toolchange_dwarf.load();
        if (old_tool != new_tool) {
            if (old_tool) {
                if (old_tool->set_selected(false) == CommunicationStatus::ERROR) {
                    return false;
                }
                log_info(PrusaToolChanger, "Deactivated Dwarf #%u", old_tool->dwarf_index());

                active_dwarf = nullptr; // No dwarf is selected right now
                loadcell.Clear(); // No loadcell is available now, make sure that it is not stuck in active mode
            }
            if (new_tool) {
                if (new_tool->set_selected(true) == CommunicationStatus::ERROR) {
                    return false;
                }
                log_info(PrusaToolChanger, "Activated Dwarf #%u", new_tool->dwarf_index());

                active_dwarf = new_tool; // New tool is necessary for stepperE0.push()
                stepperE0.push(); // Write current stepper settings
            }
        }
        // Update physically picked tool before clearing the request
        autodetect_active_tool();
        request_toolchange = false;
        return true;
    }

    // Update physically picked tool
    autodetect_active_tool();
    return true;
}

uint8_t PrusaToolChangerUtils::get_active_tool_nr() const {
    return active_extruder;
}

bool PrusaToolChangerUtils::is_any_tool_active() const {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    return active_extruder != MARLIN_NO_TOOL_PICKED;
}

bool PrusaToolChangerUtils::is_tool_active(uint8_t idx) const {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    return active_extruder == idx;
}

uint8_t PrusaToolChangerUtils::get_num_enabled_tools() const {
    return std::ranges::count_if(dwarfs, [](const auto &dwarf) { return dwarf.is_enabled(); });
}

Dwarf *PrusaToolChangerUtils::get_marlin_picked_tool() {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    if (active_extruder == MARLIN_NO_TOOL_PICKED) {
        return nullptr;
    } else {
        return &dwarfs[active_extruder];
    }
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

uint8_t PrusaToolChangerUtils::detect_tool_nr() {
    Dwarf *dwarf = picked_dwarf.load();
    if (dwarf) {
        return dwarf->dwarf_index();
    } else {
        return MARLIN_NO_TOOL_PICKED;
    }
}

uint8_t PrusaToolChangerUtils::get_enabled_mask() {
    static_assert(DWARF_MAX_COUNT < 8, "Using uint8_t as a mask of dwarves");
    uint8_t mask = 0;

    for (int i = 0; i < DWARF_MAX_COUNT; i++) {
        if (dwarfs[i].is_enabled()) {
            mask |= (0x01 << i);
        }
    }

    return mask;
}

uint8_t PrusaToolChangerUtils::get_parked_mask() {
    static_assert(DWARF_MAX_COUNT < 8, "Using uint8_t as a mask of dwarves");
    uint8_t mask = 0;

    for (int i = 0; i < DWARF_MAX_COUNT; i++) {
        if (dwarfs[i].is_enabled() && (dwarfs[i].is_picked() == false) && dwarfs[i].is_parked()) {
            mask |= (0x01 << i);
        }
    }

    return mask;
}

buddy::puppies::Dwarf &PrusaToolChangerUtils::getActiveToolOrFirst() {
    auto active = active_dwarf.load();
    return active ? *active : dwarfs[0];
}

buddy::puppies::Dwarf &PrusaToolChangerUtils::getTool(uint8_t tool_index) {
    assert(tool_index < dwarfs.size());
    return buddy::puppies::dwarfs[tool_index];
}

void PrusaToolChangerUtils::load_tool_info() {
    for (unsigned int i = 0; i < tool_info.size(); ++i) {
        if (i < config_store_ns::max_tool_count) {
            DockPosition position = config_store().get_dock_position(i);
            tool_info[i].dock_x = position.x;
            tool_info[i].dock_y = position.y;
        } else {
            tool_info[i].dock_x = 0;
            tool_info[i].dock_y = 0;
        }
    }
}

void PrusaToolChangerUtils::save_tool_info() {
    for (size_t i = 0; i < std::min<size_t>(tool_info.size(), config_store_ns::max_tool_count); ++i) {
        config_store().set_dock_position(i, { .x = tool_info[i].dock_x, .y = tool_info[i].dock_y });
    }
}

void PrusaToolChangerUtils::save_tool_offsets() {
    for (int8_t e = 0; e < std::min<int8_t>(HOTENDS, config_store_ns::max_tool_count); e++) {
        config_store().set_tool_offset(e, { .x = hotend_offset[e].x, .y = hotend_offset[e].y, .z = hotend_offset[e].z });
    }
}

void PrusaToolChangerUtils::load_tool_offsets() {
    HOTEND_LOOP() {
        if (e < static_cast<int8_t>(config_store_ns::max_tool_count)) {
            ToolOffset offset = config_store().get_tool_offset(e);
            hotend_offset[e].x = offset.x;
            hotend_offset[e].y = offset.y;
            hotend_offset[e].z = offset.z;
        } else {
            hotend_offset[e].reset();
        }
    }
}

bool PrusaToolChangerUtils::load_tool_info_from_usb() {
    // This is for development purposes only

    FILE *file = fopen("/usb/toolchanger.txt", "r");

    if (file == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < tool_info.size(); ++i) {
        std::array<char, 20> line_buffer;
        size_t pos = 0;

        // Read line
        while (pos < line_buffer.size()) {
            char c;
            if (fread(&c, 1, 1, file) != 1 || c == '\n') {
                line_buffer[pos++] = 0;
                break;
            }
            line_buffer[pos++] = c;
        }

        // Get x position
        tool_info[i].dock_x = atof(line_buffer.data());

        // Get y position
        char *second = strstr(line_buffer.data(), " ");
        if (second) {
            tool_info[i].dock_y = atof(second);
        }
    }

    return fclose(file) == 0;
}

bool PrusaToolChangerUtils::save_tool_info_to_usb() {
    // This is for development purposes only

    FILE *file = fopen("/usb/toolchanger.txt", "w");

    if (file == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < tool_info.size(); ++i) {
        std::array<char, 30> buffer;
        int n = snprintf(buffer.data(), buffer.size(), "%.2f %.2f\n", tool_info[i].dock_x, tool_info[i].dock_y);
        fwrite(buffer.data(), sizeof(char), std::min<int>(n, buffer.size() - 1), file);
    }

    return fclose(file) == 0;
}

bool PrusaToolChangerUtils::save_tool_offsets_to_file(const char *filename) {
    // This is for development purposes only

    FILE *file = fopen(filename, "w");

    if (file == nullptr) {
        return false;
    }

    HOTEND_LOOP() {
        std::array<char, 40> buffer;
        int n = snprintf(buffer.data(), buffer.size(), "%f %f %f\n", hotend_offset[e].x, hotend_offset[e].y, hotend_offset[e].z);
        fwrite(buffer.data(), sizeof(char), std::min<int>(n, buffer.size() - 1), file);
    }

    return fclose(file) == 0;
}

bool PrusaToolChangerUtils::load_tool_offsets_from_usb() {
    // This is for development purposes only

    FILE *file = fopen("/usb/tooloffsets.txt", "r");

    if (file == nullptr) {
        return false;
    }

    HOTEND_LOOP() {
        std::array<char, 40> buffer;
        size_t pos = 0;

        // Read line
        while (pos < buffer.size()) {
            char c;
            if (fread(&c, 1, 1, file) != 1 || c == '\n') {
                buffer[pos++] = 0;
                break;
            }
            buffer[pos++] = c;
        }

        hotend_offset[e].x = atof(buffer.data());

        char *second = strnstr(buffer.data(), " ", pos) + 1;
        hotend_offset[e].y = atof(second);

        char *third = strnstr(second, " ", pos) + 1;
        hotend_offset[e].z = atof(third);
    }

    hotend_offset[MARLIN_NO_TOOL_PICKED].reset(); // Discard any offset on no tool

    return fclose(file) == 0;
}

const PrusaToolInfo &PrusaToolChangerUtils::get_tool_info(const Dwarf &dwarf, bool check_calibrated) const {
    assert(dwarf.dwarf_index() < tool_info.size());
    const PrusaToolInfo &info = tool_info[dwarf.dwarf_index()];

    if (check_calibrated && (std::isnan(info.dock_x) || std::isnan(info.dock_y) || info.dock_x == 0 || info.dock_y == 0)) {
        toolchanger_error("Dock Position not calibrated");
    }

    return info;
}

bool PrusaToolChangerUtils::is_tool_info_valid(const Dwarf &dwarf) const {
    return is_tool_info_valid(dwarf, get_tool_info(dwarf));
}

bool PrusaToolChangerUtils::is_tool_info_valid(const Dwarf &dwarf, const PrusaToolInfo &info) const {
    const PrusaToolInfo synthetic = compute_synthetic_tool_info(dwarf);
    const auto dx = info.dock_x - synthetic.dock_x;
    const auto dy = info.dock_y - synthetic.dock_y;
    return sqrt(pow(dx, 2) + pow(dy, 2)) < DOCK_INVALID_OFFSET_MM;
}

void PrusaToolChangerUtils::set_tool_info(const buddy::puppies::Dwarf &dwarf, const PrusaToolInfo &info) {
    assert(dwarf.dwarf_index() < tool_info.size());

    tool_info[dwarf.dwarf_index()] = info;
}

void PrusaToolChangerUtils::toolchanger_error(const char *message) const {
    fatal_error(message, "PrusaToolChanger");
}

void PrusaToolChangerUtils::expand_first_dock_position() {
    // Compute dock positions using first dock position
    const PrusaToolInfo first = get_tool_info(dwarfs[0]);

    for (uint i = 1; i < tool_info.size(); ++i) {
        const PrusaToolInfo computed = {
            .dock_x = first.dock_x + i * DOCK_OFFSET_X_MM,
            .dock_y = first.dock_y
        };
        set_tool_info(dwarfs[i], computed);
    }
}

PrusaToolInfo PrusaToolChangerUtils::compute_synthetic_tool_info(const Dwarf &dwarf) const {
    return PrusaToolInfo({ .dock_x = DOCK_DEFAULT_FIRST_X_MM + DOCK_OFFSET_X_MM * (dwarf.dwarf_index()),
        .dock_y = DOCK_DEFAULT_Y_MM });
}

void PrusaToolChangerUtils::ConfRestorer::sample() {
    if (sampled) {
        bsod("Double sampled planner configuration");
    }
    sampled_jerk = planner.settings.max_jerk;
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
    s.max_jerk = sampled_jerk;
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

// This function confuses the indexer, so it is last in the file
bool PrusaToolChangerUtils::wait(std::function<bool()> function, uint32_t timeout_ms) {
    uint32_t start_time = ticks_ms();
    bool result = false;
    while (!(result = function()) // Wait for this and remember its state for return
        && !planner.draining() // This triggers on powerpanic and quickstop
        && (ticks_ms() - start_time) < timeout_ms) { // Timeout
        idle(true, true);
    }
    return result;
}

#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
