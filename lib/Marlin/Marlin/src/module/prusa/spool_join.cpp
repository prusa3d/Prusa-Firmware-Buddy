#include "spool_join.hpp"
#include "printers.h"
#if PRINTER_IS_PRUSA_XL()
    #include "Configuration_XL.h"
#endif
#include <logging/log.hpp>
#include "module/motion.h"
#include "module/prusa/tool_mapper.hpp"
#include <cmath>
#include <limits>
#include <optional>
#include <option/has_crash_detection.h>
#include <option/has_mmu2.h>
#include <option/has_dwarf.h>
#include "module/temperature.h"
#include "module/planner.h" // for get_axis_position_mm
#include "module/tool_change.h"
#include <config_store/store_instance.hpp>
#include "mmu2_toolchanger_common.hpp"
#include <feature/print_status_message/print_status_message_mgr.hpp>
#include <feature/print_status_message/print_status_message_guard.hpp>
#include <tool_index.hpp>
#include <mapi/parking.hpp>
#include <mapi/motion.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>
#include <pause_stubbed.hpp>
#include <include/fs_event_autolock.hpp>
#include <feature/prusa/e-stall_detector.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include <option/has_nozzle_cleaner.h>
#if HAS_NOZZLE_CLEANER()
    #include "../../../feature/nozzle_cleaner/include/nozzle_cleaner.hpp"
#endif

#include <option/has_filament_tracker.h>
#include <bsod/bsod.h>
#if HAS_FILAMENT_TRACKER()
    #include <feature/filament_tracker/filament_tracker.hpp>
#endif

#if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
#endif

SpoolJoin spool_join;

LOG_COMPONENT_REF(Marlin);

SpoolJoin &SpoolJoin::operator=(const SpoolJoin &other) {
    std::scoped_lock lock(mutex, other.mutex);
    this->num_joins = other.num_joins;
    for (size_t i = 0; i < joins.size(); i++) {
        this->joins[i] = other.joins[i];
    }
    return *this;
}

void SpoolJoin::reset() {
    std::unique_lock lock(mutex);
    num_joins = 0;
    for (auto &join : joins) {
        join.spool_1 = join.spool_2 = reset_value;
    }
}

bool SpoolJoin::add_join(uint8_t spool_1, uint8_t spool_2) {
    std::unique_lock lock(mutex);
    if (num_joins >= joins.size() || spool_1 >= VirtualToolIndex::count || !VirtualToolIndex::from_raw(spool_1).is_enabled() || spool_2 >= VirtualToolIndex::count || !VirtualToolIndex::from_raw(spool_2).is_enabled() || spool_1 == spool_2) {
        return false;
    }
    // join will be added at the end of existing joins, so when for example
    // 0 will join with 1, and we want to join0 with 2,  actual join created will be 1 -> 2,
    // because we first want to join 0 -> 1, and then 1 -> 2
    spool_1 = get_last_spool_2_from_chain_unlocked(spool_1);

    // Prevent adding loops
    if (get_first_spool_1_from_chain_unlocked(spool_2) == get_first_spool_1_from_chain_unlocked(spool_1)) {
        return false;
    }

    // check again that we are not joining spool with itself - spool_1 might have changed above
    if (spool_1 == spool_2) {
        return false;
    }

    for (auto &join : joins) {
        if (join.spool_2 == spool_2) {
            // join to this spool was already configured before, do not allow to join to same spool twice
            return false;
        }
    }

    // save join
    join_config_t join;
    join.spool_1 = spool_1;
    join.spool_2 = spool_2;
    joins[num_joins++] = join;

    return true;
}

void SpoolJoin::remove_join_at(size_t idx) {
    debug_assert(num_joins > 0 && idx < num_joins);
    joins[idx].spool_1 = joins[idx].spool_2 = reset_value;
    // so that we can insert new join at num_joins, we need to store the last join instead of the one we're deleting (note: we can swap even if `idx == num_joins - 1`)
    std::swap(joins[idx], joins[num_joins - 1]);
    --num_joins;
}

bool SpoolJoin::reroute_joins_containing(uint8_t spool) {
    std::unique_lock lock(mutex);
    size_t preceding_idx { std::size(joins) };
    size_t followup_idx { std::size(joins) };

    for (size_t i = 0; i < num_joins; ++i) {
        if (joins[i].spool_1 == spool) {
            followup_idx = i;
        } else if (joins[i].spool_2 == spool) {
            preceding_idx = i;
        }
    }

    if (preceding_idx != std::size(joins) && followup_idx != std::size(joins)) {
        // if found && not last in chain && not first -> rechain
        joins[preceding_idx].spool_2 = joins[followup_idx].spool_2;

        remove_join_at(followup_idx);
        return true;
    } else if (preceding_idx != std::size(joins)) {
        // if found && first in chain -> remove
        remove_join_at(preceding_idx);
        return true;
    } else if (followup_idx != std::size(joins)) {
        // if found && last in chain -> remove
        remove_join_at(followup_idx);
        return true;
    } else {
        // we don't have it
        return false;
    }
}

bool SpoolJoin::remove_join_chain_containing(uint8_t spool) {
    std::unique_lock lock(mutex);
    return remove_join_chain_containing_unlocked(spool);
}

bool SpoolJoin::remove_join_chain_containing_unlocked(uint8_t spool) {
    size_t preceding_idx { std::size(joins) };
    size_t followup_idx { std::size(joins) };

    for (size_t i = 0; i < num_joins; ++i) {
        if (joins[i].spool_1 == spool) {
            followup_idx = i;
        } else if (joins[i].spool_2 == spool) {
            preceding_idx = i;
        }
    }

    if (preceding_idx != std::size(joins) && followup_idx != std::size(joins)) {
        // if found && not last in chain && not first -> rechain
        auto tmp_preceding = joins[preceding_idx].spool_1;
        auto tmp_followup = joins[followup_idx].spool_2;
        remove_join_at(std::max(preceding_idx, followup_idx)); // remove_join_at can reoder last item, so remove the bigger of the two indices in case one of them is last
        remove_join_at(std::min(preceding_idx, followup_idx)); // remove the other index

        remove_join_chain_containing_unlocked(tmp_preceding);
        remove_join_chain_containing_unlocked(tmp_followup);
        return true;
    } else if (preceding_idx != std::size(joins)) {
        // if found && first in chain -> remove

        auto tmp = joins[preceding_idx].spool_1;
        remove_join_at(preceding_idx);
        remove_join_chain_containing_unlocked(tmp);
        return true;
    } else if (followup_idx != std::size(joins)) {
        // if found && last in chain -> remove
        auto tmp = joins[followup_idx].spool_2;
        remove_join_at(followup_idx);
        remove_join_chain_containing_unlocked(tmp);
        return true;
    } else {
        // we don't have it
        return false;
    }
}

uint8_t SpoolJoin::get_last_spool_2_from_chain(uint8_t spool_1) const {
    std::unique_lock lock(mutex);
    return get_last_spool_2_from_chain_unlocked(spool_1);
}

uint8_t SpoolJoin::get_last_spool_2_from_chain_unlocked(uint8_t spool_1) const {
    for (size_t i = 0; i < num_joins; ++i) {
        if (joins[i].spool_1 == spool_1) {
            spool_1 = joins[i].spool_2;
            i = -1; // reset the loop and search again
        }
    }
    return spool_1;
}

uint8_t SpoolJoin::get_first_spool_1_from_chain(uint8_t spool_2) const {
    std::unique_lock lock(mutex);
    return get_first_spool_1_from_chain_unlocked(spool_2);
}

uint8_t SpoolJoin::get_first_spool_1_from_chain_unlocked(uint8_t spool_2) const {
    for (size_t i = 0; i < num_joins; ++i) {
        if (joins[i].spool_2 == spool_2) {
            spool_2 = joins[i].spool_1;
            i = -1; // reset the loop and search again
        }
    }
    return spool_2;
}

std::optional<uint8_t> SpoolJoin::get_spool_2(uint8_t tool) const {
    std::unique_lock lock(mutex);
    return get_spool_2_unlocked(tool);
}

std::optional<uint8_t> SpoolJoin::get_spool_2_unlocked(uint8_t tool) const {
    for (size_t i = 0; i < num_joins; i++) {
        if (joins[i].spool_1 == tool) {
            return joins[i].spool_2;
        }
    }

    return std::nullopt;
}

bool SpoolJoin::do_join(VirtualToolIndex current_virtual_tool) {
    std::unique_lock lock(mutex);
    auto new_raw_virtual_tool = spool_join.get_spool_2_unlocked(current_virtual_tool.to_raw());
    if (!new_raw_virtual_tool.has_value()) {
        return false;
    }
    const auto new_virtual_tool = VirtualToolIndex::from_raw(new_raw_virtual_tool.value());

    log_info(Marlin, "Spool join from %d to %d (z=%f)", current_virtual_tool.to_raw(), new_virtual_tool.to_raw(), planner.get_axis_position_mm(AxisEnum::Z_AXIS));
    PrintStatusMessageGuard statusGuard;
    statusGuard.update<PrintStatusMessage::joining_spool>({});

    planner.synchronize();

    /// The tool offset can change during the toolchange, so store the return position as logical (nozzle) position
    [[maybe_unused]] const XYZval<float, LogicalPosTag> return_pos = current_position.asLogical().xyz();

    // set up new tool mapping, so that next Tx will use spool we are joining to
    // but do mapping of logical->physical, so first convert current_tool to its logical tool
    const auto gcode_tool = stdext::get_optional<GcodeToolIndex>(tool_mapper.to_gcode(current_virtual_tool));
    if (!gcode_tool.has_value()) {
        return false;
    }
    if (!tool_mapper.set_mapping(*gcode_tool, new_virtual_tool)) {
        return false;
    }
    tool_mapper.set_enable(true);

    static_assert((HAS_INDX() + HAS_DWARF() + HAS_MMU2()) == 1, "This code assumes exactly one of these toolchanger types");

#if HAS_TOOLCHANGER()
    const auto current_physical_tool = current_virtual_tool.to_physical();
    const auto new_physical_tool = new_virtual_tool.to_physical();

    const auto nozzle_temp = thermalManager.degTargetHotend(current_physical_tool);

    const auto orig_e_pos = current_position.e;

    bool should_park = true;
    #if HAS_CRASH_DETECTION()
    // Do not park/unpark during crash recovery (= gcode intterupt)
    // Crash recovery handles that
    should_park &= (crash_s.get_state() != Crash_s::RECOVERY);
    #endif

    const float target_retracted_distance = buddy::filament_tracker().get_retracted_distance(current_physical_tool).value_or(0);

    // Start preheating the new tool
    thermalManager.setTargetHotend(nozzle_temp, new_physical_tool);

    if (should_park) {
        // Z raise
        mapi::park(mapi::ParkingPosition { .z = mapi::ParkingPosition::AtLeast { .above_print = TOOLCHANGE_ZRAISE } });
    }

    // Unload the old filament (hopefully not requiring user interaction)
    {
        FS_EventAutolock fs_lock;
        BlockEStallDetection estall_lock;

        pause::Settings settings;
        settings.SetExtruder(current_virtual_tool);

        // We've already retracted
        settings.SetRetractLength(0.f);

        settings.SetParkPoint(mapi::get_parking_position(mapi::ParkPosition::unload));

        // Pick the right tool
        Pause::Instance().perform(Pause::LoadType::unload_spool_join, settings);
    }

    // Cool down the old tool
    thermalManager.setTargetHotend(0, current_physical_tool);

    const auto wait_for_temp = [&] {
        if (nozzle_temp != 0) {
            thermalManager.wait_for_hotend(new_physical_tool, { .no_wait_for_cooling = false, .fan_cooling = true });
        }
    };

    #if !HAS_INDX()
    // Park current tool
    tool_change(NoTool {}, tool_return_t::no_return, tool_change_lift_t::no_lift, false);

    // Wait for the new tool to heat up WHILE IT'S PARKED - not possible on INDX
    wait_for_temp();
    #endif

    // change to new tool
    tool_change(new_virtual_tool, tool_return_t::no_return, tool_change_lift_t::no_lift, false);

    #if HAS_NOZZLE_CLEANER()
    nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::enter_cleaner);

    wait_for_temp();

    nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::purge_clean);
    nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::clean);
    nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::exit_cleaner);

    #else
    wait_for_temp();
    prusa_toolchanger.purge_tool(new_physical_tool);

    #endif

    if (should_park) {
        // return to original print position
        do_blocking_move_to(return_pos.asNative());
    }

    // Match the retracted distance of the original tool
    mapi::restore_retracted_distance(target_retracted_distance, buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::retract));

    // Make the spool join seamless in terms of E stepper position
    sync_e_position_to(orig_e_pos);

#elif HAS_MMU2()
    MMU2::mmu2.tool_change_full(new_virtual_tool.to_raw());
#else
    #error "unknown printer"
#endif

    print_status_message().show_temporary<PrintStatusMessage::spool_joined>({});
    return true;
}

void SpoolJoin::serialize(serialized_state_t &to) {
    // NOTE: We do not lock here now, as it is not possible other thread would be modifying
    // the objekt at this point (they do that before starting the print). If this ever changes
    // we should rethink this, this is called from default task, not ISR, so it might be ok to lock.
    // init to defaults
    to = serialized_state_t();
    for (size_t i = 0; i < num_joins; i++) {
        to.joins[i] = joins[i];
    }
}

void SpoolJoin::deserialize(serialized_state_t &from) {
    // Note: both functions below already lock, so no need here
    reset();
    for (auto join : from.joins) {
        // this will fail for undefined joins and otherwise invalid joins. Only valid joins will be added
        (void)add_join(join.spool_1, join.spool_2);
    }
}
