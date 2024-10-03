#include "printer_animation_state.hpp"
#include <logging/log.hpp>

LOG_COMPONENT_DEF(Led, logging::Severity::info);

using namespace marlin_server;

namespace {
/**
 * @brief Animation and color storing in eeprom was deprecated during eeprom migration to configuration_store. This is a remnant of it for ease of access
 *
 */
constexpr Animation_model default_animations[] {
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_IDLE
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PRINTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PAUSING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_RESUMING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_ABORTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 255, 0, 0, 0, 0 }, // EEVAR_ANIMATION_FINISHING
    { static_cast<uint8_t>(AnimationTypes::Fading), 255, 255, 0, 1000, 0, 0 }, // EEVAR_ANIMATION_WARNING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_POWER_PANIC
    { static_cast<uint8_t>(AnimationTypes::Fading), 0, 255, 0, 1500, 0, 0 }, // EEVAR_ANIMATION_POWER_UP
};
static_assert(std::size(default_animations) == 9, "That's how many there were in old eeprom");
} // namespace

PrinterState leds::mpsToAnimationState(marlin_server::State state) {
    switch (state) {

    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
    case State::Exit:
        return PrinterState::Idle;

    case State::WaitGui:
    case State::Printing:
    case State::PrintInit:
    case State::SerialPrintInit:
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
    case State::Aborting_Preview:
    case State::Aborting_UnloadFilament:
    case State::Aborted:
        return PrinterState::Aborting;

    case State::Finishing_WaitIdle:
    case State::Finishing_ParkHead:
    case State::Finishing_UnloadFilament:
    case State::Finished:
        return PrinterState::Finishing;

    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
    case State::CrashRecovery_XY_Measure:
#if HAS_TOOLCHANGER()
    case State::CrashRecovery_Tool_Pickup:
#endif
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
        return PrinterState::Warning;

    case State::PowerPanic_acFault:
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
        return PrinterState::PowerPanic;
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
    Animation_model animationModel = default_animations[static_cast<uint32_t>(state)];
    switch (static_cast<AnimationTypes>(animationModel.type)) {
    case AnimationTypes::SolidColor:
        return SolidColor(leds::ColorRGBW(animationModel.R, animationModel.G, animationModel.B));
    case AnimationTypes::Fading:
        return Fading(leds::ColorRGBW(animationModel.R, animationModel.G, animationModel.B), animationModel.period);
    }
    return SolidColor(leds::ColorRGBW(leds::ColorHSV { 0, 0, 0 }));
}
AnimatorLCD::AnimationGuard leds::start_animation(PrinterState state, int priority) {
    Animations animation = get_animation(state);
    return start_animation(animation, priority);
}
PrinterStateAnimation &PrinterStateAnimation::Access() {
    static PrinterStateAnimation controller;
    return controller;
}
