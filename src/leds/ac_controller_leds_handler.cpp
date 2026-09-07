#include "leds/ac_controller_leds_handler.hpp"

#include <marlin_server_state.h>
#include <marlin_vars.hpp>
#include <puppies/ac_controller.hpp>
#include <ac_controller/types.hpp>
#include <option/has_tool_crash_recovery.h>
#include <bsod/bsod.h>

namespace leds {

using namespace marlin_server;

static ac_controller::AnimationType marlin_to_anim_state() {

    const marlin_server::State printer_state = marlin_vars().print_state;

    switch (printer_state) {
    case State::Printing:
    case State::PrintInit:
    case State::SerialPrintInit:
    case State::Finishing_WaitIdle:
    case State::Pausing_Begin:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
    case State::Resuming_BufferData:
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
    case State::Resuming_ExecutingGCodeInterrupt:
        return ac_controller::AnimationType::PROGRESS_PERCENT;

    case State::Idle:
        return ac_controller::AnimationType::OFF;

    case State::Aborting_Begin:
    case State::Aborted:
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
#if HAS_INDX()
    case State::PowerPanic_FinishIndxToolchange:
#endif
    case State::CrashRecovery_XY_Measure:
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
#if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup:
#endif
    case State::PowerPanic_acFault:
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
    case State::Finished:
    case State::Exit:
    case State::MediaErrorRecovery_BufferData:
    case State::PrintPreviewInit:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
    case State::PrintPreviewImage:
    case State::Paused:
    case State::Aborting_ParkHead:
    case State::Aborting_UnloadFilament:
    case State::Aborting_WaitIdle:
    case State::Aborting_Preview:
    case State::Finishing_UnloadFilament:
    case State::Finishing_ParkHead:
        return ac_controller::AnimationType::STATIC_COLOR;
    }

    bsod("Unknown printer state in AcControllerLedsHandler");
    return ac_controller::AnimationType::OFF;
}

void AcControllerLedsHandler::update(ColorRGBW &color, uint8_t progress_percent) {
    switch (marlin_to_anim_state()) {
    case ac_controller::AnimationType::OFF:
        buddy::puppies::ac_controller.turn_off_bed_leds();
        break;
    case ac_controller::AnimationType::STATIC_COLOR:
        buddy::puppies::ac_controller.set_rgbw_led({ color.r, color.g, color.b, color.w });
        break;
    case ac_controller::AnimationType::PROGRESS_PERCENT:
        buddy::puppies::ac_controller.set_progress_percent(progress_percent);
        break;
    }
}

} // namespace leds
