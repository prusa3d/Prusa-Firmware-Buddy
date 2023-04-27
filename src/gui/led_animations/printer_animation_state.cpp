#include "printer_animation_state.hpp"
#include "log.h"

LOG_COMPONENT_DEF(Led, LOG_SEVERITY_INFO);

using namespace marlin_server;

PrinterState leds::mpsToAnimationState(marlin_server::marlin_print_state_t state) {
    switch (state) {
    case mpsIdle:
    case mpsPrintPreviewInit:
    case mpsPrintPreviewImage:
    case mpsPrintPreviewQuestions:
    case mpsExit:
        return PrinterState::Idle;
    case mpsWaitGui:
    case mpsPrinting:
    case mpsPrintInit:
        return PrinterState::Printing;
    case mpsPausing_Begin:
    case mpsPausing_Failed_Code:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
    case mpsPaused:
        return PrinterState::Pausing;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead_XY:
    case mpsResuming_UnparkHead_ZE:
        return PrinterState::Resuming;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
    case mpsAborted:
        return PrinterState::Aborting;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
    case mpsFinished:
        return PrinterState::Finishing;
    case mpsCrashRecovery_Begin:
    case mpsCrashRecovery_Retracting:
    case mpsCrashRecovery_Lifting:
    case mpsCrashRecovery_XY_Measure:
    case mpsCrashRecovery_Tool_Pickup:
    case mpsCrashRecovery_XY_HOME:
    case mpsCrashRecovery_HOMEFAIL:
    case mpsCrashRecovery_Axis_NOK:
    case mpsCrashRecovery_Repeated_Crash:
        return PrinterState::Warning;
    case mpsPowerPanic_acFault:
    case mpsPowerPanic_Resume:
    case mpsPowerPanic_AwaitingResume:
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
