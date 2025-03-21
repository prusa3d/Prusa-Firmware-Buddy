#include "auto_retract.hpp"

#include <marlin_vars.hpp>
#include <config_store/store_definition.hpp>
#include <feature/ramming/standard_ramming_sequence.hpp>
#include <module/planner.h>
#include <RAII.hpp>
#include <filament_sensors_handler.hpp>
#include <logging/log.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

// Auto retract functionality collides with the nozzle cleaner
#include <option/has_nozzle_cleaner.h>
static_assert(!HAS_NOZZLE_CLEANER());

LOG_COMPONENT_REF(MarlinServer);

using namespace buddy;

AutoRetract &buddy::auto_retract() {
    static AutoRetract instance;
    return instance;
}

bool AutoRetract::is_retracted(uint8_t hotend) const {
    return retracted_hotends_bitset_.test(hotend);
}

bool AutoRetract::is_retracted() const {
    return is_retracted(marlin_vars().active_hotend_id());
}

float AutoRetract::retracted_distance() const {
    const auto hotend = marlin_vars().active_hotend_id();
    return is_retracted(hotend) ? standard_ramming_sequence(StandardRammingSequence::auto_retract, hotend).retracted_distance() : 0;
}

void buddy::AutoRetract::mark_as_retracted(uint8_t hotend, bool retracted) {
    if (retracted_hotends_bitset_.test(hotend) == retracted) {
        return;
    }

    retracted_hotends_bitset_.set(hotend, retracted);
    config_store().filament_auto_retracted_bitset.set(retracted_hotends_bitset_.to_ulong());
}

void AutoRetract::maybe_retract_from_nozzle() {
    if (!can_perform_action()) {
        return;
    }

    const auto hotend = marlin_vars().active_hotend_id();

    // Already is retracted -> exit
    if (is_retracted(hotend)) {
        return;
    }

    // Do not auto retract flexible filaments, they might get tangled in the extruder (BFW-6953)
    if (config_store().get_filament_type(hotend).parameters().is_flexible) {
        return;
    }

    const auto orig_e_position = planner.get_position_msteps().e;
    const auto orig_current_e_position = current_position.e;

    standard_ramming_sequence(StandardRammingSequence::auto_retract, hotend).execute();

    // "Fake" original extruder position - we are interrupting various movements by this function,
    // firmware gets very confused if the current position changes while it is planning a move
    planner.set_e_position_mm(orig_e_position);
    current_position.e = orig_current_e_position;

    mark_as_retracted(hotend, true);
}

void AutoRetract::maybe_deretract_to_nozzle() {
    // Prevent deretract nesting
    if (is_checking_deretract_) {
        return;
    }
    AutoRestore ar(is_checking_deretract_, true);

    const auto hotend = marlin_vars().active_hotend_id();

    // Already not retracted -> exit
    if (!is_retracted(hotend)) {
        return;
    }

    if (!can_perform_action()) {
        log_error(MarlinServer, "auto_retract: Cannot perform deretract");
        return;
    }

    const auto orig_e_position = planner.get_position_msteps().e;
    const auto orig_current_e_position = current_position.e;

    standard_ramming_sequence(StandardRammingSequence::auto_retract, hotend).undo();

    // "Fake" original extruder position - we are interrupting various movements by this function,
    // firmware gets very confused if the current position changes while it is planning a move
    planner.set_e_position_mm(orig_e_position);
    current_position.e = orig_current_e_position;

    mark_as_retracted(hotend, false);
}

AutoRetract::AutoRetract() {
    retracted_hotends_bitset_ = config_store().filament_auto_retracted_bitset.get();
}

bool AutoRetract::can_perform_action() const {
#if HAS_MMU2()
    // No auto retracting if MMU is enabled
    if (MMU2::mmu2.Enabled()) {
        return false;
    }
#endif

    if (thermalManager.tooColdToExtrude(active_extruder)) {
        return false;
    }

    if (planner.draining()) {
        return false;
    }

    return true;
}
