#include "tool_offset_wizard.hpp"

#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/mapi/calibration_preamble.hpp>
#include <common/selftest_result.hpp>
#include <config_store/store_instance.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>
#include <feature/tool_offset_calibration/tool_offset_calibration.hpp>
#include <logging/log.hpp>
#include <marlin_server.hpp>
#include <module/motion.h>
#include <selftest/selftest_invocation.hpp>
#include <test_result.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_DEF(ToolOffsetWizard, logging::Severity::info);

using marlin_server::wait_for_response;

namespace tool_offset_wizard {

namespace {

    class Wizard {
    public:
        void run() {
            const auto result = run_inner();

            switch (result) {
            case Result::success:
                config_store().selftest_result_tool_offsets_calibration.set(TestResult::passed);
                break;
            case Result::failed:
                config_store().selftest_result_tool_offsets_calibration.set(TestResult::failed);
                break;
            case Result::aborted_during_calib:
                // run() reset the offsets, so the previous result is stale -> force a recalibration.
                config_store().selftest_result_tool_offsets_calibration.set(TestResult::unknown);
                [[fallthrough]];
            case Result::aborted_before_calib:
#if HAS_SELFTEST()
                selftest_invocation::mark_aborted();
#endif // HAS_SELFTEST()
                break;
            }

            if (result == Result::success) {
                fsm_change(PhaseToolOffsetsCalibration::calibration_success);
                wait_for_response(PhaseToolOffsetsCalibration::calibration_success);
            } else if (result == Result::failed) {
                fsm_change(PhaseToolOffsetsCalibration::calibration_failed);
                wait_for_response(PhaseToolOffsetsCalibration::calibration_failed);
            }
        }

    private:
        marlin_server::FSM_Holder holder { PhaseToolOffsetsCalibration::intro };

        enum class Result {
            success,
            failed,
            aborted_before_calib,
            aborted_during_calib
        };

        void fsm_change(PhaseToolOffsetsCalibration phase, fsm::PhaseData data = {}) {
            marlin_server::fsm_change(phase, data);
        }

        Result run_inner() {
            // Intro
            fsm_change(PhaseToolOffsetsCalibration::intro);
            if (wait_for_response(PhaseToolOffsetsCalibration::intro) == Response::Abort) {
                return Result::aborted_before_calib;
            }

            // The nozzle cleaner is not calibrated yet at this stage of the selftest, so we can't
            // auto-purge/clean. Ask the user to ensure nozzles are clean before we heat & probe.
            fsm_change(PhaseToolOffsetsCalibration::ensure_nozzles_clean);
            if (wait_for_response(PhaseToolOffsetsCalibration::ensure_nozzles_clean) == Response::Abort) {
                return Result::aborted_before_calib;
            }

            const mapi::CalibrationPreamble preamble {
                .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_picked,
                .on_step = [this](mapi::CalibrationPreamble::Step step) {
                    switch (step) {
                    case mapi::CalibrationPreamble::Step::moving_away:
                        fsm_change(PhaseToolOffsetsCalibration::moving_away);
                        break;
                    case mapi::CalibrationPreamble::Step::picking_tool:
                        fsm_change(PhaseToolOffsetsCalibration::picking_tool);
                        break;
                    case mapi::CalibrationPreamble::Step::homing:
                        fsm_change(PhaseToolOffsetsCalibration::homing);
                        break;
                    case mapi::CalibrationPreamble::Step::parking_tool:
                        bsod_unreachable();
                    }
                },
            };

            // Make the bed provably safe (it may be at an unknown height with Z unhomed) and
            // get a tool picked / XY homed before any moves over the bed.
            if (!preamble.run()) {
                return Result::aborted_before_calib;
            }

            // Most of the time is spent in blocking heat-ups and the XY scan, so an idle subscriber
            // throws a gcode exception the moment Abort is pressed; run() then unwinds at its next
            // draining check. The handler marks axes unhomed on exit, as the quick-stop may have
            // skipped steps. Scoping the handler here resumes queuing automatically once it unwinds.
            fsm_change(PhaseToolOffsetsCalibration::calibrating);
            GCodeExceptionHandler abort_handler { GCEHandlerExtent::any_move, [] { set_all_unhomed(); } };
            Subscriber abort_watcher { marlin_server::idle_publisher, [&abort_handler] {
                                          if (marlin_server::get_response_from_phase(PhaseToolOffsetsCalibration::calibrating) == Response::Abort) {
                                              gcode_exceptions().throw_at(&abort_handler);
                                          }
                                      } };
            const auto progress_cb = [this](const tool_offset_calibration::ProgressReport &p) -> bool {
                fsm_change(PhaseToolOffsetsCalibration::calibrating,
                    fsm::serialize_data(ProgressData::from(p.step, p.total_steps, p.tool.to_raw())));
                return true;
            };
            if (!tool_offset_calibration::run(0, 1, tool_offset_calibration::Context::Calibration, progress_cb)) {
                return gcode_exceptions().is_unwinding() ? Result::aborted_during_calib : Result::failed;
            }

            return Result::success;
        }
    };

} // namespace

void run() {
    Wizard wizard;
    wizard.run();
}

} // namespace tool_offset_wizard
