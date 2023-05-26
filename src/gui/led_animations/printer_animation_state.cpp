#include "printer_animation_state.hpp"
#include "log.h"

LOG_COMPONENT_DEF(Led, LOG_SEVERITY_INFO);

using namespace marlin_server;

PrinterState leds::mpsToAnimationState(marlin_server::State state) {
    switch (state) {
    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewQuestions:
    case State::Exit:
        return PrinterState::Idle;
    case State::WaitGui:
    case State::Printing:
    case State::PrintInit:
        return PrinterState::Printing;
    case State::Pausing_Begin:
    case State::Pausing_Failed_Code:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
    case State::Paused:
        return PrinterState::Pausing;
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
        return PrinterState::Resuming;
    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_ParkHead:
    case State::Aborted:
        return PrinterState::Aborting;
    case State::Finishing_WaitIdle:
    case State::Finishing_ParkHead:
    case State::Finished:
        return PrinterState::Finishing;
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_XY_Measure:
    case State::CrashRecovery_Tool_Pickup:
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
        return PrinterState::Warning;
    case State::PowerPanic_acFault:
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
        return PrinterState::PowerPanic;
    default:
        log_error(Led, "Invalid marlin print state");
        return PrinterState::Idle;
    }
    return PrinterState::Idle;
}
AnimatorLCD::AnimationGuard leds::start_animation(Animations animation, int priority) {
    return std::visit([priority](const auto &anim) -> AnimatorLCD::AnimationGuard {
        return Animator_LCD_leds().start_animations(anim, priority);
    },
        animation);
}
Animations leds::get_animation(PrinterState state) {
    std::array<leds::Color, eeprom_num_colors> colors;
    Animation_model animationModel = eeprom_get_animation(static_cast<uint32_t>(state), colors);
    switch (static_cast<AnimationTypes>(animationModel.type)) {
    case AnimationTypes::SolidColor:
        return SolidColor(leds::Color(animationModel.R, animationModel.G, animationModel.B));
    case AnimationTypes::Fading:
        return Fading(leds::Color(animationModel.R, animationModel.G, animationModel.B), animationModel.period);
    }
    return SolidColor(leds::Color(leds::HSV { 0, 0, 0 }));
}
AnimatorLCD::AnimationGuard leds::start_animation(PrinterState state, int priority) {
    Animations animation = get_animation(state);
    return start_animation(animation, priority);
}
PrinterStateAnimation &PrinterStateAnimation::Access() {
    static PrinterStateAnimation controller;
    return controller;
}
