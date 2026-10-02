#include "indx_nozzle_cleaning.hpp"

#include <algorithm>
#include <optional>

#include <bsod/bsod.h>
#include <client_response.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>
#include <filament.hpp>
#include <leds/side_strip_handler.hpp>
#include <logging/log.hpp>
#include <mapi/calibration_preamble.hpp>
#include <mapi/parking.hpp>
#include <marlin_server.hpp>
#include <Marlin/src/Marlin.h>
#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/temperature.h>
#include <Marlin/src/module/tool_change.h>
#include <nozzle_cleaner.hpp>
#include <printers.h>
#include <raii/scope_guard.hpp>
#include <tool_index.hpp>
#include <utils/variant_utils.hpp>

LOG_COMPONENT_DEF(IndxNozzleCleaning, logging::Severity::info);

using marlin_server::wait_for_response;
using Phase = PhaseNozzleCleaning;

namespace indx_nozzle_cleaning {

namespace {

    /// Cleaning temperature for a tool without a loaded filament [°C]
    constexpr int16_t default_hot_temp = 230;

    /// Temperature at which the user finishes the manual cleaning [°C]
    constexpr int16_t cold_temp = 180;

    /// Delay between reaching the cleaning temperature and the automatic cleaning [ms]
    constexpr millis_t automatic_clean_delay_ms = 5000;

    /// Head position for the manual cleaning; the bed is lowered to make room for the user's hands
#if PRINTER_IS_PRUSA_COREONE()
    constexpr float manual_clean_x = 125;
    constexpr float manual_clean_y = 100;
#elif PRINTER_IS_PRUSA_COREONEL()
    constexpr float manual_clean_x = X_BED_SIZE / 2.f;
    constexpr float manual_clean_y = Y_BED_SIZE / 2.f;
#else
    #error "Manual nozzle cleaning position not defined for this printer"
#endif
    constexpr float manual_clean_min_z = 200;
    static_assert(manual_clean_min_z <= Z_SIZE);

    int16_t hot_temp_for(PhysicalToolIndex tool) {
        const auto virtual_tool = stdext::get_optional<VirtualToolIndex>(tool.currently_selected_virtual_tool());
        const FilamentType filament = virtual_tool.has_value() ? FilamentType::for_tool_heuristic(*virtual_tool) : FilamentType::none;
        if (filament == FilamentType::none) {
            return default_hot_temp;
        }
        const int16_t temp = filament.parameters().nozzle_temperature;
        return temp > 0 ? temp : default_hot_temp;
    }

    class Wizard {
    public:
        Wizard(uint32_t tool_mask, Mode mode)
            : tool_mask_(tool_mask)
            , mode_(mode) {}

        void run() {
            const bool completed = run_inner();

            for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                if (is_selected(tool)) {
                    thermalManager.setTargetHotend(0, tool);
                }
            }

            if (completed) {
                fsm_change(Phase::finished);
                wait_for_response(Phase::finished);
            }
        }

    private:
        marlin_server::FSM_Holder holder_ { Phase::preparing };

        // The light would dim after a while without user input, as outside of a print
        Subscriber<> keep_light_on_ { marlin_server::idle_publisher, [] { leds::SideStripHandler::instance().activity_ping(); } };
        const uint32_t tool_mask_;
        const Mode mode_;

        void fsm_change(Phase phase, fsm::PhaseData data = {}) {
            marlin_server::fsm_change(phase, data);
        }

        bool is_selected(PhysicalToolIndex tool) const {
            return tool_mask_ & (uint32_t { 1 } << tool.to_raw());
        }

        /// @return false when aborted or failed
        bool run_inner() {
            const mapi::CalibrationPreamble preamble {
                // Each selected tool gets picked by the cleaning loop
                .tool_policy = mapi::CalibrationPreamble::ToolPolicy::keep_as_is,
                .on_step = [this](mapi::CalibrationPreamble::Step step) {
                    switch (step) {
                    case mapi::CalibrationPreamble::Step::moving_away:
                    case mapi::CalibrationPreamble::Step::homing:
                        fsm_change(Phase::preparing);
                        break;
                    case mapi::CalibrationPreamble::Step::picking_tool:
                    case mapi::CalibrationPreamble::Step::parking_tool:
                        bsod_unreachable();
                    }
                },
            };
            if (!preamble.run()) {
                return false;
            }
            ScopeGuard park_guard = [this] { park_tool_and_head(); };

            for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
                if (!is_selected(tool)) {
                    continue;
                }
                if (!clean_tool(tool)) {
                    return false;
                }
            }

            return true;
        }

        bool clean_tool(PhysicalToolIndex tool) {
            const uint8_t display_index = tool.display_index();
            const int16_t hot_temp = hot_temp_for(tool);
            log_info(IndxNozzleCleaning, "Tool %u: cleaning at %d C", display_index, hot_temp);

            fsm_change(Phase::picking_tool, PhaseDataLayout::make(display_index, 0));
            // The preamble lowered the bed already; a lift per toolchange would add up to the bottom
            if (!tool_change(stdext::to_variant(tool), tool_return_t::no_return, tool_change_lift_t::no_lift, false)) {
                log_error(IndxNozzleCleaning, "Tool change to tool %u failed", display_index);
                return false;
            }

            if (mode_ == Mode::manual) {
                if (!mapi::park({ .x = manual_clean_x, .y = manual_clean_y, .z = mapi::ParkingPosition::AtLeast { .absolute = manual_clean_min_z } })) {
                    return false;
                }
            }

            if (!wait_temp(Phase::heating, tool, hot_temp)) {
                return false;
            }

            if (mode_ == Mode::automatic) {
                GcodeSuite::dwell(automatic_clean_delay_ms);
            }

            if (mode_ == Mode::manual) {
                if (!ask_user(Phase::manual_clean_hot, display_index)) {
                    return false;
                }
                if (!wait_temp(Phase::cooling, tool, std::min(cold_temp, hot_temp))) {
                    return false;
                }
                if (!ask_user(Phase::manual_clean_cold, display_index)) {
                    return false;
                }
            }

            fsm_change(Phase::cleaning, PhaseDataLayout::make(display_index, 0));
            const auto sequence = mode_ == Mode::automatic ? nozzle_cleaner::Sequence::deep_clean : nozzle_cleaner::Sequence::quick_clean;
            if (!nozzle_cleaner::load_and_execute(sequence)) {
                return false;
            }
            thermalManager.setTargetHotend(0, tool);

            // Don't do the next toolchange from inside the cleaner
            return nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::exit_cleaner);
        }

        void park_tool_and_head() {
            if (planner.draining()) {
                return;
            }
            fsm_change(Phase::parking_tool);
            if (!nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::exit_cleaner)
                || !tool_change(NoTool {}, tool_return_t::no_return, tool_change_lift_t::no_lift, false)) {
                log_error(IndxNozzleCleaning, "Parking the tool failed");
                return;
            }
            mapi::park();
        }

        /// @return false when the user aborted
        bool ask_user(Phase phase, uint8_t display_index) {
            fsm_change(phase, PhaseDataLayout::make(display_index, 0));
            return wait_for_response(phase) == Response::Continue;
        }

        /// Sets the target and waits for it in both directions; returns as soon as the target is crossed when heating.
        /// @return false when the user aborted
        bool wait_temp(Phase phase, PhysicalToolIndex tool, int16_t target) {
            const uint8_t display_index = tool.display_index();
            const auto publish = [&] {
                fsm_change(phase, PhaseDataLayout::make(display_index, static_cast<int16_t>(thermalManager.degHotend(tool))));
            };
            publish();

            // The wait does no moves, so there are no skipped steps to recover from
            GCodeExceptionHandler abort_handler { GCEHandlerExtent::extruder_only, [] {} };
            const auto on_idle = [&] {
                if (marlin_server::get_response_from_phase(phase) == Response::Abort) {
                    gcode_exceptions().throw_at(&abort_handler);
                    return;
                }
                publish();
            };
            // Subscriber stores only a small callable, so pass the state by a single reference
            Subscriber subscriber(marlin_server::idle_publisher, [&on_idle] { on_idle(); });

            const bool heating = thermalManager.degHotend(tool) < target;
            thermalManager.setTargetHotend(target, tool);
            thermalManager.wait_for_hotend(tool, {
                                                     .no_wait_for_cooling = false,
                                                     .early_return_temperature = heating ? std::optional<float>(target) : std::nullopt,
                                                 });
            return !gcode_exceptions().is_unwinding();
        }
    };

} // namespace

void run(uint32_t tool_mask, Mode mode) {
    Wizard wizard(tool_mask, mode);
    wizard.run();
}

} // namespace indx_nozzle_cleaning
