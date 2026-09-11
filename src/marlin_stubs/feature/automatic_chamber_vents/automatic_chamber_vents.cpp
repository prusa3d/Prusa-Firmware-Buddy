/// @file
#include "automatic_chamber_vents.hpp"

#include <feature/print_status_message/print_status_message_guard.hpp>
#include <mapi/parking.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/module/planner.h>
#include <printers.h>
#include <option/has_indx.h>
#include <raii/scope_guard.hpp>
#include <marlin_server.hpp>

#include <feature/chamber/chamber.hpp>

#if HAS_INDX()
    #include <module/prusa/toolchanger.h>
#endif

namespace automatic_chamber_vents {
namespace {

    static constexpr auto approach_feedrate = feedRate_t(XY_PROBE_FEEDRATE_MM_S); ///< Moves that do not touch the lever

    /// The following constants define key positions for controlling a vent lever.
    /// These coordinates are absolute in the printer's coordinate system.
#if HAS_INDX()
    /// Dock whose tool's bowden operates the vent lever. Must be empty during the sequence.
    static constexpr auto VENT_DOCK = PhysicalToolIndex::from_raw(3); // T4
    static constexpr auto vent_feedrate = feedRate_t(60.0f); ///< 60 mm/s (G0 F3600)
    static constexpr float vent_travel_accel_mm_s2 = 1000.0f; ///< M204 T1000
#elif PRINTER_IS_PRUSA_COREONE()
    static constexpr auto Y_SAFE = -3.f; ///< Safe Y line with no risk of coming in contact with lever
    static constexpr auto Y_LEVER = -18.f; ///< In line with the lever
    static constexpr auto X_OPEN_START_POS = 37.f; ///< An X-axis position to the right of the lever, where opening move starts.
    static constexpr auto X_OPEN_END_POS = 24.f; ///< The X-axis position to move to on Y_LEVER to open the vents.
    static constexpr auto X_CLOSE_START_POS = 11.f; ///< An X-axis position to the left of the lever, where closing move starts.
    static constexpr auto X_CLOSE_END_POS = 26.f; ///< The X-axis position to move to on Y_LEVER to close the vents.
    static constexpr auto X_LEVER_MOVE_AWAY = 4.f; ///< The X-axis distance to move away from the lever after switch
    static constexpr auto lever_move_feedrate = feedRate_t(40.0f);
#elif PRINTER_IS_PRUSA_COREONEL()
    static constexpr auto Y_SAFE = 10.f; ///< Safe Y line with no risk of coming in contact with lever
    static constexpr auto Y_LEVER = -7.f; ///< In line with the lever
    static constexpr auto X_OPEN_START_POS = 25.f; ///< An X-axis position to the left of the lever, where opening move starts.
    static constexpr auto X_OPEN_END_POS = 42.f; ///< The X-axis position to move to on Y_LEVER to open the vents.
    static constexpr auto X_CLOSE_START_POS = 50.f; ///< An X-axis position to the right of the lever, where closing move starts.
    static constexpr auto X_CLOSE_END_POS = 35.f; ///< The X-axis position to move to on Y_LEVER to close the vents.
    static constexpr auto X_LEVER_MOVE_AWAY = -4.f; ///< The X-axis distance to move away from the lever after switch (COREONEL has inverted open/close direction)
    static constexpr auto lever_move_feedrate = feedRate_t(17.0f);
#else
    #error
#endif

    /// @brief Plans a move to a new X-axis coordinate.
    /// @param x The target X-axis position.
    /// @param feedrate The speed of the move in mm/s.
    void plan_to_x(float x, feedRate_t feedrate) {
        prepare_move_to(current_position.with_x(x), feedrate, {});
    }

    /// @brief Plans a move to a new Y-axis coordinate.
    /// @param y The target Y-axis position.
    /// @param feedrate The speed of the move in mm/s.
    void plan_to_y(float y, feedRate_t feedrate) {
        prepare_move_to(current_position.with_y(y), feedrate, {});
    }

    /// @brief Plans a move to a new XY coordinate.
    [[maybe_unused]] void plan_to_xy(float x, float y, feedRate_t feedrate) {
        prepare_move_to(current_position.with_xy(x, y), feedrate, {});
    }

#if HAS_INDX()
    void open_vents_move_sequence() {
    #if PRINTER_IS_PRUSA_COREONE()
        plan_to_y(0.0f, approach_feedrate);
        plan_to_x(123.0f, approach_feedrate);
        plan_to_y(-7.0f, vent_feedrate);
        plan_to_xy(120.0f, -7.5f, vent_feedrate);
        plan_to_x(115.0f, vent_feedrate);
        plan_to_x(117.0f, vent_feedrate);
        plan_to_y(0.0f, vent_feedrate);
    #elif PRINTER_IS_PRUSA_COREONEL()
        plan_to_y(0.0f, approach_feedrate);
        plan_to_x(153.0f, approach_feedrate);
        plan_to_y(-10.0f, vent_feedrate);
        plan_to_xy(147.0f, -16.0f, vent_feedrate);
        plan_to_x(140.0f, vent_feedrate);
        plan_to_y(0.0f, vent_feedrate);
    #endif
    }

    void close_vents_move_sequence() {
    #if PRINTER_IS_PRUSA_COREONE()
        plan_to_y(0.0f, approach_feedrate);
        plan_to_x(108.0f, approach_feedrate);
        plan_to_y(-11.5f, vent_feedrate);
        plan_to_x(117.0f, vent_feedrate);
        plan_to_x(115.0f, vent_feedrate);
        plan_to_y(0.0f, vent_feedrate);
    #elif PRINTER_IS_PRUSA_COREONEL()
        plan_to_y(0.0f, approach_feedrate);
        plan_to_x(127.0f, approach_feedrate);
        plan_to_y(-10.0f, vent_feedrate);
        plan_to_xy(133.0f, -16.0f, vent_feedrate);
        plan_to_x(145.0f, vent_feedrate);
        plan_to_y(0.0f, vent_feedrate);
    #endif
    }

    /// @brief Run the full INDX vent-lever actuation: tool prep, reduced accel, move sequence.
    /// @return true on success.
    bool execute_indx_vent_lever(bool open) {
        prusa_toolchanger.pick_tool_out_of_dock(VENT_DOCK);

        // Make lever moves independent of print-time M203/M204, then lower travel accel.
        Temporary_Reset_Motion_Parameters motion_params_guard;
        auto vent_settings = planner.user_settings;
        vent_settings.travel_acceleration = vent_travel_accel_mm_s2;
        planner.apply_settings(vent_settings);

        if (open) {
            open_vents_move_sequence();
        } else {
            close_vents_move_sequence();
        }
        return true;
    }
#endif

}; // namespace

bool execute_control(VentState target_state) {
    const bool open = (target_state == VentState::open);

    PrintStatusMessageGuard psm_guard;
    if (open) {
        psm_guard.update<PrintStatusMessage::Type::opening_chamber_vents>({});
    } else {
        psm_guard.update<PrintStatusMessage::Type::closing_chamber_vents>({});
    }
    marlin_server::FSM_Holder fsm_holder { PhaseWait::print_status_message };

    // On INDX, vent control holds specific tool, for that we need pickup (that needs precise homing), this precise homing avoids double homing on the next pickup
    if (!GcodeSuite::G28_no_parser(true, true, false, { .only_if_needed = true, .precise = HAS_INDX() })) {
        return false;
    }

#if HAS_INDX()
    const auto previous_tool = PhysicalToolIndex::currently_selected();
    auto restore_tool_now = [&]() -> bool {
        return prusa_toolchanger.tool_change(previous_tool, tool_return_t::no_return, {}, tool_change_lift_t::full_lift, false);
    };
    // Best-effort tool restore on the failure paths (already returns false).
    ScopeGuard restore_tool([&] { restore_tool_now(); });

    if (!execute_indx_vent_lever(open)) {
        return false;
    }
#else
    // Move to a safe Y-axis position to avoid the lever.
    plan_to_y(Y_SAFE, approach_feedrate);
    // Move to a horizontal position (left or right) of the lever.
    plan_to_x(open ? X_OPEN_START_POS : X_CLOSE_START_POS, approach_feedrate);
    // Move into the lever's Y-axis line.
    plan_to_y(Y_LEVER, approach_feedrate);
    // Move horizontally to engage the lever and switch it.
    plan_to_x(open ? X_OPEN_END_POS : X_CLOSE_END_POS, lever_move_feedrate);
    // Move horizontally to release the lever tension
    plan_to_x(open ? X_OPEN_END_POS + X_LEVER_MOVE_AWAY : X_CLOSE_END_POS - X_LEVER_MOVE_AWAY, lever_move_feedrate);
    // Back out to the safe Y-axis position to avoid a collision on future moves.
    plan_to_y(Y_SAFE, approach_feedrate);

    // Return to the home position after the vent operation.
    mapi::park(mapi::get_parking_position(mapi::ParkPosition::park).without_z_move());
#endif

    // Wait for all planned moves to complete
    planner.synchronize();

    buddy::chamber().set_vent_state(target_state);

#if HAS_INDX()
    restore_tool.disarm();
    if (!restore_tool_now()) {
        return false;
    }
#endif

    return true;
}

} // namespace automatic_chamber_vents
