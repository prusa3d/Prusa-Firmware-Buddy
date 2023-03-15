#include "Marlin.h"

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "cmsis_os.h"
    #include "toolchanger.h"
    #include "puppies/PuppyBus.hpp"
    #include "Marlin/src/module/stepper.h"
    #include "Marlin/src/module/motion.h"
    #include "Marlin/src/feature/bedlevel/bedlevel.h"
    #include "log.h"
    #include "wdt.h"
    #include <functional>
    #include <algorithm>
    #include "timing.h"
    #include "Marlin/src/gcode/gcode.h"
    #include "fanctl.hpp"
    #include "../motion.h"
    #include <option/is_knoblet.h>
    #include "eeprom.h"
    #include <marlin_server.hpp>
    #include <cmath_ext.h>

    #if ENABLED(CRASH_RECOVERY)
        #include "../../feature/prusa/crash_recovery.h"
    #endif /*ENABLED(CRASH_RECOVERY)*/

LOG_COMPONENT_DEF(PrusaToolChanger, LOG_SEVERITY_DEBUG);

static_assert(EXTRUDERS == buddy::puppies::dwarfs.size());

PrusaToolChanger prusa_toolchanger;

using buddy::puppies::Dwarf;
using buddy::puppies::dwarfs;
using buddy::puppies::PuppyBus;

bool PrusaToolChanger::autodetect_toolchanger_enabled() {
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

void PrusaToolChanger::autodetect_active_tool() {
    if (!is_toolchanger_enabled()) { // Ignore on singletool
        picked_dwarf = &dwarfs[0];
        if (!picked_dwarf.load()->is_selected()) {
            picked_dwarf.load()->set_selected(true);
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

    if (active) {
        // only update tool offsets when a new dwarf is picked, so that Z is not compensated
        // immediately, but only for the new tool
        hotend_currently_applied_offset = hotend_offset[active->get_dwarf_nr() - 1];
    }
}

uint8_t PrusaToolChanger::get_active_tool_nr() const {
    return active_extruder;
}

bool PrusaToolChanger::is_any_tool_active() const {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    return active_extruder != MARLIN_NO_TOOL_PICKED;
}

bool PrusaToolChanger::is_tool_active(uint8_t idx) const {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    return active_extruder == idx;
}

uint8_t PrusaToolChanger::get_num_enabled_tools() const {
    return std::ranges::count_if(dwarfs, [](const auto &dwarf) { return dwarf.is_enabled(); });
}

Dwarf *PrusaToolChanger::get_marlin_picked_tool() {
    assert(active_extruder >= 0 && active_extruder < EXTRUDERS);
    if (active_extruder == MARLIN_NO_TOOL_PICKED) {
        return nullptr;
    } else {
        return &dwarfs[active_extruder];
    }
}

void PrusaToolChanger::force_marlin_picked_tool(Dwarf *dwarf) {
    if (dwarf == nullptr) {
        active_extruder = MARLIN_NO_TOOL_PICKED;
    } else {
        active_extruder = dwarf->get_dwarf_nr() - 1;
    }
}

bool PrusaToolChanger::init(bool first_run) {
    if (first_run) {
        toolchanger_enabled = autodetect_toolchanger_enabled();

        if (toolchanger_enabled == false) {
            if (dwarfs[0].set_selected(true) != Dwarf::CommunicationStatus::OK) {
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

void PrusaToolChanger::ensure_safe_move() {
    if (!can_move_safely()) {
        // in case XY is not homed, home it first
        GcodeSuite::G28_no_parser(false, false, 0, false, true, true, false);
    #if ENABLED(NO_HALT_ON_HOMING_ERROR)
        if (!can_move_safely()) {
            toolchanger_error("XY homing failed");
        }
    #endif
    }
}

bool PrusaToolChanger::tool_change(const uint8_t new_tool_id) {

    // WARNING: called from default(marlin) task

    if ((sampled_feedrate == NAN) || (sampled_feedrate < feedrate_mm_s)) {
        sampled_feedrate = feedrate_mm_s; // Default to feedrate set by marlin specifically for toolchange
    }

    assert(new_tool_id < EXTRUDERS);

    Dwarf *new_dwarf = (new_tool_id == MARLIN_NO_TOOL_PICKED) ? nullptr : &dwarfs[new_tool_id];
    Dwarf *old_dwarf = picked_dwarf.load(); // Change from physically picked dwarf

    if (!is_toolchanger_enabled()) {
        toolchanger_error("Toolchanger not enabled");
    }
    if (new_dwarf && !new_dwarf->is_enabled()) {
        toolchanger_error("Toolchange to tool that is not enabled");
    }

    // Set block_tool_check and toolchange_in_progress and reset on return from this function
    ResetOnReturn resetter([&](bool state) {
        block_tool_check = state;
    #if ENABLED(CRASH_RECOVERY)
        crash_s.set_toolchange_in_progress(state);
    #endif /*ENABLED(CRASH_RECOVERY)*/
    });

    if (new_dwarf != old_dwarf) {
        // Home X and Y if needed
        ensure_safe_move();

        // Park old tool
        if (old_dwarf != nullptr) {
            if (park(*old_dwarf) == false) {
                return false;
            }
            check_skipped_step();
        }
    }

    // request select, wait for it to complete, and measure the final offset difference
    xyz_pos_t old_offset = hotend_currently_applied_offset;
    request_active_switch(new_dwarf);
    tool_change_diff = hotend_currently_applied_offset - old_offset;

    // Disable print fan on old dwarf, enable on new dwarf
    // todo: remove this when multiple fans are implemented properly
    // todo: remove active_extruder, is is only needed to avoid turning fan back on
    active_extruder = new_tool_id;
    if (old_dwarf != nullptr) {
        fanCtlPrint[old_dwarf->get_dwarf_nr() - 1].setPWM(0);
    }

    if (new_dwarf != old_dwarf) {
        if (new_dwarf == nullptr) {
            // compensate for the Z difference even when detaching
            z_shift(tool_change_diff.z);
        } else {
            // Before we try to pick up new tool, check that its parked properly
            if (!new_dwarf->is_parked()) {
                log_error(PrusaToolChanger, "Trying to pick missing Dwarf %u, triggering toolchanger recovery", new_dwarf->get_dwarf_nr());
                toolcrash();
                return false;
            }

            // Pick new tool
            if (pickup(*new_dwarf, tool_change_diff) == false) {
                return false;
            }
            check_skipped_step();
        }
    }

    return true;
}

void PrusaToolChanger::check_skipped_step() {
    #if ENABLED(CRASH_RECOVERY)
    if (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH) {
        TemporaryBedLevelingState tbls(false); // Temporarily disable leveling (important is to restore it after homing)
        crash_s.set_state(Crash_s::TOOLCRASH); // Has to go through TOOLCRASH to PRINTING
        crash_s.set_state(Crash_s::PRINTING);
        GcodeSuite::G28_no_parser(false, false, 0, false, true, true, false); // Home to clear any skipped steps
    }
    #endif /*ENABLED(CRASH_RECOVERY)*/
}

uint8_t PrusaToolChanger::detect_tool_nr() {
    Dwarf *dwarf = picked_dwarf.load();
    if (dwarf) {
        return dwarf->get_dwarf_nr() - 1;
    } else {
        return PrusaToolChanger::MARLIN_NO_TOOL_PICKED;
    }
}

void PrusaToolChanger::toolcrash() {
    #if ENABLED(CRASH_RECOVERY)
    if (crash_s.is_active() && (crash_s.get_state() == Crash_s::PRINTING)) {
        crash_s.set_state(Crash_s::TRIGGERED_TOOLCRASH);
        return;
    }

    if ((crash_s.get_state() == Crash_s::IDLE)                                           // Print ended
        || ((crash_s.get_state() == Crash_s::RECOVERY) && crash_s.is_toolchange_event()) // Already recovering, cannot disrupt crash_s, it will replay Tx gcode
        || (crash_s.get_state() == Crash_s::TRIGGERED_ISR)                               // ISR crash happened, it will replay Tx gcode
        || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLFALL)                          // Toolcrash is already in progress
        || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH)
        || (crash_s.get_state() == Crash_s::TOOLCRASH)) {
        return;
    }
    #endif /*ENABLED(CRASH_RECOVERY)*/

    // Can happen if toolchange is a part of replay, would need a bigger change in crash_recovery.cpp
    toolchanger_error("Tool crashed");
}

void PrusaToolChanger::toolfall() {
    #if ENABLED(CRASH_RECOVERY)
    if (crash_s.is_active() && (crash_s.get_state() == Crash_s::PRINTING)) {
        crash_s.set_state(Crash_s::TRIGGERED_TOOLFALL);
        return;
    }

    if (crash_s.get_state() == Crash_s::IDLE) { // Print ended
        return;
    }
    #endif /*ENABLED(CRASH_RECOVERY)*/

    // Can be called when starting the print, not a big problem
    // Can happen if tool falls of during recovery, homing or reheating, would need a bigger change in crash_recovery.cpp
    toolchanger_error("Tool fell off");
}

void PrusaToolChanger::request_active_switch(Dwarf *new_dwarf) {
    request_toolchange_dwarf = new_dwarf;
    request_toolchange = true;
    if (wait([this]() { return !this->request_toolchange.load(); }, WAIT_TIME_TOOL_SELECT, WAIT_TIME_TOOL_PERIOD) == false) {
        toolchanger_error("Tool switch failed");
    }
}

bool PrusaToolChanger::wait(std::function<bool()> function, uint32_t timeout_ms, uint32_t period_ms) {
    uint32_t start_time = ticks_ms();
    while (!function() && (ticks_ms() - start_time) < timeout_ms) {
        osDelay(period_ms);
    }
    return function();
}

bool PrusaToolChanger::update() {
    if (request_toolchange) {
        // Make requested tool active
        Dwarf *old_tool = active_dwarf.load();
        Dwarf *new_tool = request_toolchange_dwarf.load();
        if (old_tool != new_tool) {
            if (old_tool) {
                if (old_tool->set_selected(false) != Dwarf::CommunicationStatus::OK) {
                    return false;
                }
                log_info(PrusaToolChanger, "Deactivated Dwarf %u", old_tool->get_dwarf_nr());

                active_dwarf = nullptr; // No dwarf is selected right now
            }
            if (new_tool) {
                if (new_tool->set_selected(true) != Dwarf::CommunicationStatus::OK) {
                    return false;
                }
                log_info(PrusaToolChanger, "Activated Dwarf %u", new_tool->get_dwarf_nr());

                active_dwarf = new_tool; // New tool is necessary for stepperE0.push()
                stepperE0.push();        // Write current stepper settings
            }
        }
        request_toolchange = false;
    }

    // Update physically picked tool
    autodetect_active_tool();

    // todo: return error properly
    return true;
}

void PrusaToolChanger::loop(bool printing) {
    // WARNING: called from default(marlin) task

    if (block_tool_check.load()         // Can be blocked if changing tools
        || !is_toolchanger_enabled()) { // Ignore on singletool
        return;
    }

    // Automatically change tool
    if (force_toolchange_gcode.load()                                                                                 // Force toolchange after reset to force all marlin tool variables
        || ((get_marlin_picked_tool() != picked_dwarf.load())                                                         // When user parked or picked manually
            && (printing == false) && (planner.movesplanned() == false) && (queue.has_commands_queued() == false))) { // Only if not printing and nothing is in queue
        force_toolchange_gcode = false;

        // Update tool through marlin
        marlin_server_enqueue_gcode_printf("T%d S1", detect_tool_nr());
    }

    // Check that all tools are where they should be
    if (printing // Only while printing
    #if ENABLED(CRASH_RECOVERY)
        // Do not check during crash recovery
        && (crash_s.get_state() == Crash_s::PRINTING)
    #endif /*ENABLED(CRASH_RECOVERY)*/
    ) {
        if (get_marlin_picked_tool() != picked_dwarf.load()) {
            toolfall(); // Tool switched
            return;
        }
        for (Dwarf &dwarf : dwarfs) {
            if (dwarf.is_enabled() && (dwarf.is_picked() == false) && (dwarf.is_parked() == false)) {
                toolfall(); // Tool fell off
                return;
            }
        }
    }
}

uint8_t PrusaToolChanger::get_enabled_mask() {
    static_assert(DWARF_MAX_COUNT < 8, "Using uint8_t as a mask of dwarves");
    uint8_t mask = 0;

    for (int i = 0; i < DWARF_MAX_COUNT; i++) {
        if (dwarfs[i].is_enabled()) {
            mask |= (0x01 << i);
        }
    }

    return mask;
}

uint8_t PrusaToolChanger::get_parked_mask() {
    static_assert(DWARF_MAX_COUNT < 8, "Using uint8_t as a mask of dwarves");
    uint8_t mask = 0;

    for (int i = 0; i < DWARF_MAX_COUNT; i++) {
        if (dwarfs[i].is_enabled() && (dwarfs[i].is_picked() == false) && dwarfs[i].is_parked()) {
            mask |= (0x01 << i);
        }
    }

    return mask;
}

buddy::puppies::Dwarf &PrusaToolChanger::getActiveToolOrFirst() {
    auto active = active_dwarf.load();
    return active ? *active : dwarfs[0];
}

buddy::puppies::Dwarf &PrusaToolChanger::getTool(uint8_t tool_index) {
    assert(tool_index < dwarfs.size());
    return buddy::puppies::dwarfs[tool_index];
}

PrusaToolChanger::PrusaToolChanger() {
    tool_info.fill({ 0, 0 });
}

void PrusaToolChanger::load_tool_info() {
    for (unsigned int i = 0; i < tool_info.size(); ++i) {
        if (i < EEPROM_MAX_TOOL_COUNT) {
            KennelPosition position = eeprom_get_kennel_position(i);
            tool_info[i].kennel_x = position.x;
            tool_info[i].kennel_y = position.y;
        } else {
            tool_info[i].kennel_x = 0;
            tool_info[i].kennel_y = 0;
        }
    }
}

void PrusaToolChanger::save_tool_info() {
    for (size_t i = 0; i < std::min<size_t>(tool_info.size(), EEPROM_MAX_TOOL_COUNT); ++i) {
        eeprom_set_kennel_position(i, { .x = tool_info[i].kennel_x, .y = tool_info[i].kennel_y });
    }
}

void PrusaToolChanger::save_tool_offsets() {
    for (int8_t e = 0; e < std::min(HOTENDS, EEPROM_MAX_TOOL_COUNT); e++) {
        eeprom_set_tool_offset(e, { .x = hotend_offset[e].x, .y = hotend_offset[e].y, .z = hotend_offset[e].z });
    }
}

void PrusaToolChanger::load_tool_offsets() {
    HOTEND_LOOP() {
        if (e < EEPROM_MAX_TOOL_COUNT) {
            ToolOffset offset = eeprom_get_tool_offset(e);
            hotend_offset[e].x = offset.x;
            hotend_offset[e].y = offset.y;
            hotend_offset[e].z = offset.z;
        } else {
            hotend_offset[e].reset();
        }
    }
}

bool PrusaToolChanger::load_tool_info_from_usb() {
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
        tool_info[i].kennel_x = atof(line_buffer.data());

        // Get y position
        char *second = strstr(line_buffer.data(), " ");
        if (second) {
            tool_info[i].kennel_y = atof(second);
        }
    }

    return fclose(file) == 0;
}

bool PrusaToolChanger::save_tool_info_to_usb() {
    // This is for development purposes only

    FILE *file = fopen("/usb/toolchanger.txt", "w");

    if (file == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < tool_info.size(); ++i) {
        std::array<char, 30> buffer;
        int n = snprintf(buffer.data(), buffer.size(), "%.2f %.2f\n", tool_info[i].kennel_x, tool_info[i].kennel_y);
        fwrite(buffer.data(), sizeof(char), n, file);
    }

    return fclose(file) == 0;
}

bool PrusaToolChanger::save_tool_offsets_to_usb() {
    // This is for development purposes only

    FILE *file = fopen("/usb/tooloffsets.txt", "w");

    if (file == nullptr) {
        return false;
    }

    HOTEND_LOOP() {
        std::array<char, 40> buffer;
        int n = snprintf(buffer.data(), buffer.size(), "%f %f %f\n", hotend_offset[e].x, hotend_offset[e].y, hotend_offset[e].z);
        fwrite(buffer.data(), sizeof(char), n, file);
    }

    return fclose(file) == 0;
}

bool PrusaToolChanger::load_tool_offsets_from_usb() {
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

const PrusaToolInfo &PrusaToolChanger::get_tool_info(const Dwarf &dwarf, bool check_calibrated) const {
    assert(dwarf.get_dwarf_nr() <= tool_info.size());
    assert(dwarf.get_dwarf_nr() > 0);
    const PrusaToolInfo &info = tool_info[dwarf.get_dwarf_nr() - 1];

    if (check_calibrated && (std::isnan(info.kennel_x) || std::isnan(info.kennel_y) || info.kennel_x == 0 || info.kennel_y == 0)) {
        toolchanger_error("Kennel Position not calibrated");
    }

    return info;
}

bool PrusaToolChanger::is_tool_info_valid(const Dwarf &dwarf) const {
    return is_tool_info_valid(dwarf, get_tool_info(dwarf));
}

bool PrusaToolChanger::is_tool_info_valid(const Dwarf &dwarf, const PrusaToolInfo &info) const {
    const PrusaToolInfo synthetic = compute_synthetic_tool_info(dwarf);
    const auto dx = abs(info.kennel_x - synthetic.kennel_x);
    const auto dy = abs(info.kennel_y - synthetic.kennel_y);
    return sqrt(pow(dx, 2) * pow(dy, 2)) < KENNEL_INVALID_OFFSET_MM;
}

void PrusaToolChanger::set_tool_info(const buddy::puppies::Dwarf &dwarf, const PrusaToolInfo &info) {
    assert(dwarf.get_dwarf_nr() <= tool_info.size());
    assert(dwarf.get_dwarf_nr() > 0);

    tool_info[dwarf.get_dwarf_nr() - 1] = info;
}

void PrusaToolChanger::move(const float x, const float y, const feedRate_t feedrate) {
    current_position.x = x;
    current_position.y = y;
    line_to_current_position(feedrate);
}

bool PrusaToolChanger::park(Dwarf &dwarf) {
    auto dwarf_parked = [&dwarf]() {
        dwarf.refresh_park_pick_status();
        return dwarf.is_parked();
    };

    auto dwarf_not_picked = [&dwarf]() {
        dwarf.refresh_park_pick_status();
        return !dwarf.is_picked();
    };

    const PrusaToolInfo &info = get_tool_info(dwarf, /*check_calibrated=*/true);

    // safe target kennel position
    const float target_x = info.kennel_x - 10.0;
    const float target_y = SAFE_Y_WITH_TOOL;

    // arc parameters
    const float arc_seg_len = 1.f;     // mm
    const float arc_max_radius = 50.f; // mm
    const float arc_min_radius = 2.f;  // mm
    const float arc_tg_jerk = 20.f;
    constexpr bool arc_allow_backward_move = true;

    float arc_d_y = (current_position.y - target_y);
    float arc_r = min(hypotf(target_x - current_position.x, arc_d_y), arc_max_radius);
    if (!arc_allow_backward_move) {
        // do not allow the Y carriage to move backwards
        arc_r = min(arc_r, -arc_d_y);
    } else {
        // reduce the radius to never exceed 1/2 of the backward movement
        arc_r = min(arc_r, -arc_d_y * 1.5f);
    }

    // reduce maximum parking speed to improve reliability during constant toolchanging
    float travel_feedrate = fminf(PARKING_FINAL_MAX_SPEED, sampled_feedrate);

    // save current jerk if an arc move is planned
    xy_float_t cur_jerk = planner.max_jerk;

    if (arc_r >= arc_min_radius) {
        // tangent arc planning
        float arc_dir = current_position.x < target_x ? 1 : -1;
        float arc_x = target_x - arc_r * arc_dir;
        float arc_d_x = (current_position.x - arc_x);
        float d = hypotf(arc_d_x, arc_d_y);
        float h = sqrtf(SQR(d) - SQR(arc_r));
        float a = atan2f(arc_d_x, arc_d_y) - atanf(h / arc_r) * arc_dir;
        float t_x = arc_x + sinf(a) * arc_r;
        float t_y = target_y + cosf(a) * arc_r;

        // arc length
        if (a < -M_PI_2)
            a += M_PI * 2;
        float a_l = (arc_dir > 0 ? M_PI_2 - a : M_PI_2 * 3 - a);
        int seg_n = (int)(fabsf(a_l) * arc_r / arc_seg_len);
        float a_n = a_l / seg_n;

        // execute combined move
        move(t_x, t_y, sampled_feedrate);
        float speed_d = sampled_feedrate - travel_feedrate;
        float speed_n = speed_d / seg_n;

        // ensure jerk doesn't limit the arc move (TODO: use planner hints with Marlin 2.1)
        planner.max_jerk[X_AXIS] = arc_tg_jerk;
        planner.max_jerk[Y_AXIS] = arc_tg_jerk;

        for (int i = 1; i < seg_n; ++i) {
            float s_a = a + a_n * i;
            float s_x = arc_x + sin(s_a) * arc_r;
            float s_y = target_y + cos(s_a) * arc_r;
            move(s_x, s_y, sampled_feedrate - speed_n * i);
        }

        planner.max_jerk = cur_jerk;
    }

    // go in front of the tool dock
    move(target_x, target_y, travel_feedrate);

    move(target_x, info.kennel_y, travel_feedrate);
    planner.synchronize(); // this creates a pause which allow the resonance in the tool to be damped before insertion of the tool in the dock

    // set motor current and stall sensitivity to parking and remember old value
    auto x_current_ma = stepperX.rms_current();
    auto x_stall_sensitivity = stepperX.stall_sensitivity();
    auto y_current_ma = stepperY.rms_current();
    auto y_stall_sensitivity = stepperY.stall_sensitivity();
    stepperX.rms_current(PARKING_CURRENT_MA);
    stepperX.stall_sensitivity(PARKING_STALL_SENSITIVITY);
    stepperY.rms_current(PARKING_CURRENT_MA);
    stepperY.stall_sensitivity(PARKING_STALL_SENSITIVITY);

    move(info.kennel_x - 9.0, info.kennel_y, SLOW_MOVE_MM_S);
    auto saved_acceleration = planner.settings.travel_acceleration;
    planner.settings.travel_acceleration = SLOW_ACCELERATION_MM_S2; // low acceleration
    move(info.kennel_x + 0.5, info.kennel_y, SLOW_MOVE_MM_S);
    planner.synchronize();
    planner.settings.travel_acceleration = saved_acceleration; // back to high acceleration

    // set motor current and stall sensitivity to old value
    stepperX.rms_current(x_current_ma);
    stepperX.stall_sensitivity(x_stall_sensitivity);
    stepperY.rms_current(y_current_ma);
    stepperY.stall_sensitivity(y_stall_sensitivity);

    move(info.kennel_x, info.kennel_y, SLOW_MOVE_MM_S);
    planner.synchronize();

    // Wait until dwarf is registering as parked
    if (!wait(dwarf_parked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
        log_warning(PrusaToolChanger, "Dwarf %u not parked, trying to wiggle it in", dwarf.get_dwarf_nr());

        move(info.kennel_x - 0.5, info.kennel_y, SLOW_MOVE_MM_S); // wiggle left
        move(info.kennel_x, info.kennel_y, SLOW_MOVE_MM_S);       // wiggle back
        planner.synchronize();

        if (!wait(dwarf_parked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
            log_error(PrusaToolChanger, "Dwarf %u not parked, triggering toolchanger recovery", dwarf.get_dwarf_nr());
            toolcrash();
            return false;
        }
    }

    move(info.kennel_x, SAFE_Y_WITHOUT_TOOL, sampled_feedrate); // extract tool

    // Wait until dwarf is registering as not picked
    if (!wait(dwarf_not_picked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
        log_error(PrusaToolChanger, "Dwarf %u still picked after parking, triggering toolchanger recovery", dwarf.get_dwarf_nr());
        toolcrash();
        return false;
    }
    log_info(PrusaToolChanger, "Dwarf %u parked successfully", dwarf.get_dwarf_nr());
    return true;
}

void PrusaToolChanger::z_shift(const float diff) {
    current_position.z += diff;
    line_to_current_position(planner.settings.max_feedrate_mm_s[Z_AXIS]);
    planner.synchronize();
}

bool PrusaToolChanger::pickup(Dwarf &dwarf, const xyz_pos_t &diff) {
    auto dwarf_picked = [&dwarf]() {
        dwarf.refresh_park_pick_status();
        return dwarf.is_picked();
    };

    auto dwarf_not_parked = [&dwarf]() {
        dwarf.refresh_park_pick_status();
        return !dwarf.is_parked();
    };

    const PrusaToolInfo &info = get_tool_info(dwarf, /*check_calibrated=*/true);

    // Save previous feedrate and acceleration and reset on return
    auto saved_acceleration = planner.settings.travel_acceleration;
    ResetOnReturn acceleration_resetter([&](bool state) {
        if (state == false) {
            planner.settings.travel_acceleration = saved_acceleration;
        }
    });

    move(info.kennel_x, SAFE_Y_WITHOUT_TOOL, sampled_feedrate);     // go in front of the tool
    move(info.kennel_x, info.kennel_y - 5.0, sampled_feedrate);     // pre-insert fast the tool
    planner.settings.travel_acceleration = SLOW_ACCELERATION_MM_S2; // low acceleration
    move(info.kennel_x, info.kennel_y, SLOW_MOVE_MM_S);             // insert slowly the last mm to allow part fitting + soft touch between TCM and tool thanks to the gentle deceleration
    planner.synchronize();

    // Wait until dwarf is registering as picked
    if (!wait(dwarf_picked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
        log_warning(PrusaToolChanger, "Dwarf %u not picked, trying to wiggle it in", dwarf.get_dwarf_nr());

        move(info.kennel_x, info.kennel_y + 0.5, SLOW_MOVE_MM_S); // wiggle pull
        move(info.kennel_x, info.kennel_y, SLOW_MOVE_MM_S);       // wiggle back
        planner.synchronize();

        if (!wait(dwarf_picked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
            log_error(PrusaToolChanger, "Dwarf %u not picked, triggering toolchanger recovery", dwarf.get_dwarf_nr());
            toolcrash();
            return false;
        }
    }

    // set motor current and stall sensitivity to parking and remember old value
    auto x_current_ma = stepperX.rms_current();
    auto x_stall_sensitivity = stepperX.stall_sensitivity();
    auto y_current_ma = stepperY.rms_current();
    auto y_stall_sensitivity = stepperY.stall_sensitivity();
    stepperX.rms_current(PARKING_CURRENT_MA);
    stepperX.stall_sensitivity(PARKING_STALL_SENSITIVITY);
    stepperY.rms_current(PARKING_CURRENT_MA);
    stepperY.stall_sensitivity(PARKING_STALL_SENSITIVITY);

    move(info.kennel_x - 11.8, info.kennel_y, SLOW_MOVE_MM_S); // accelerate gently to low speed to gently place the tool against the TCM
    planner.settings.travel_acceleration = saved_acceleration; // back to high acceleration
    move(info.kennel_x - 12.8, info.kennel_y, SLOW_MOVE_MM_S); // this line is just to allow a gentle acceleration and a quick deceleration
    move(info.kennel_x - 9.9, info.kennel_y, SLOW_MOVE_MM_S);
    planner.synchronize();

    // set motor current and stall sensitivity to old value
    stepperX.rms_current(x_current_ma);
    stepperX.stall_sensitivity(x_stall_sensitivity);
    stepperY.rms_current(y_current_ma);
    stepperY.stall_sensitivity(y_stall_sensitivity);

    // compensate for the Z difference before unparking
    z_shift(diff.z);

    // Wait until dwarf is registering as not parked
    if (!wait(dwarf_not_parked, WAIT_TIME_TOOL_PARKED_PICKED, WAIT_TIME_TOOL_PERIOD)) {
        log_error(PrusaToolChanger, "Dwarf %u still parked after picking, triggering toolchanger recovery", dwarf.get_dwarf_nr());
        toolcrash();
        return false;
    }

    move(info.kennel_x - 9.9, SAFE_Y_WITH_TOOL, sampled_feedrate); // tool extracted

    log_info(PrusaToolChanger, "Dwarf %u picked successfully", dwarf.get_dwarf_nr());
    return true;
}

bool PrusaToolChanger::can_move_safely() {
    return !axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS));
}

void PrusaToolChanger::toolchanger_error(const char *message) const {
    fatal_error(message, "PrusaToolChanger");
}

void PrusaToolChanger::expand_first_kennel_position() {
    // Compute kennel positions using first kennel position
    const PrusaToolInfo first = get_tool_info(dwarfs[0]);

    for (uint i = 1; i < tool_info.size(); ++i) {
        const PrusaToolInfo computed = {
            .kennel_x = first.kennel_x + i * KENNEL_OFFSET_X_MM,
            .kennel_y = first.kennel_y
        };
        set_tool_info(dwarfs[i], computed);
    }
}

PrusaToolInfo PrusaToolChanger::compute_synthetic_tool_info(const Dwarf &dwarf) const {
    return PrusaToolInfo({ .kennel_x = KENNEL_DEFAULT_FIRST_X_MM + KENNEL_OFFSET_X_MM * (dwarf.get_dwarf_nr() - 1),
        .kennel_y = KENNEL_DEFAULT_Y_MM });
}

#endif
