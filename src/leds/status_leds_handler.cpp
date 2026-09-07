#include "leds/status_leds_handler.hpp"

#include <led_animation_controller/animation_controller.hpp>
#include <led_animation_controller/frame_animation.hpp>
#include "marlin_vars.hpp"
#include <fsm/filament_change_phases.hpp>
#include <option/has_mmu2.h>
#include <option/has_tool_mapping.h>
#include <option/has_side_fsensor.h>
#include <option/has_tool_crash_recovery.h>

namespace leds {

using namespace marlin_server;

static StateAnimation marlin_to_anim_state() {
    fsm::States::State load_unload_state;
    bool is_preheating;
    bool is_warning;
    marlin_vars().peek_fsm_states([&](const auto &states) {
        load_unload_state = states[ClientFSM::Load_unload];
        is_preheating = states.is_active(ClientFSM::Preheat);
        is_warning = states.is_active(ClientFSM::Warning);
    });

    if (is_warning) {
        return StateAnimation::Warning;
    }

#if PRINTER_IS_PRUSA_iX()
    /*
     * These comments all refer to states in diagram provided in [BFW-6938]
     */
    if (load_unload_state.has_value()) {
        const PhasesLoadUnload phase = static_cast<PhasesLoadUnload>(load_unload_state->GetPhase());
        switch (phase) {
        // Unloading + waiting for filament removal (there is no way to distinguish between them (extruder fs = in gears))
        case PhasesLoadUnload::Ramming_stoppable:
        case PhasesLoadUnload::Ramming_unstoppable:
        case PhasesLoadUnload::Unloading_stoppable:
        case PhasesLoadUnload::Unloading_unstoppable:
        case PhasesLoadUnload::Ejecting_stoppable:
        case PhasesLoadUnload::Ejecting_unstoppable:
            return StateAnimation::Unloading;
        // Filament in removal
        case PhasesLoadUnload::UnloadNozzleCleaning:
        case PhasesLoadUnload::FilamentNotInFS:
            return StateAnimation::WaitingForFilamentRemoval;
        // Filament Removed
        case PhasesLoadUnload::AwaitingFilament_stoppable:
        case PhasesLoadUnload::AwaitingFilament_unstoppable:
            return StateAnimation::FilamentRemoved;
        // Inserting filament
        case PhasesLoadUnload::Inserting_stoppable:
        case PhasesLoadUnload::Inserting_unstoppable:
            return StateAnimation::Inserting;
        // PreLoading
        case PhasesLoadUnload::LoadingToGears_stoppable:
        case PhasesLoadUnload::LoadingToGears_unstoppable:
    #if HAS_SIDE_FSENSOR()
        case PhasesLoadUnload::LoadingObstruction_stoppable:
        case PhasesLoadUnload::LoadingObstruction_unstoppable:
    #endif
            return StateAnimation::Loading;
        // Loading filament
        case PhasesLoadUnload::Loading_stoppable:
        case PhasesLoadUnload::Loading_unstoppable:
        case PhasesLoadUnload::AutoRetracting:
        case PhasesLoadUnload::Purging_stoppable:
        case PhasesLoadUnload::Purging_unstoppable:
        case PhasesLoadUnload::LoadNozzleCleaning:
            return StateAnimation::Loading;

        case PhasesLoadUnload::WaitingTemp_stoppable:
        case PhasesLoadUnload::WaitingTemp_unstoppable:
        case PhasesLoadUnload::Parking_stoppable:
        case PhasesLoadUnload::Parking_unstoppable:
        case PhasesLoadUnload::Unparking:
        case PhasesLoadUnload::ChangingTool:
            return StateAnimation::WaitingForPrinter;

        case PhasesLoadUnload::IsFilamentUnloaded:
        case PhasesLoadUnload::ManualUnload_continuable:
        case PhasesLoadUnload::ManualUnload_uncontinuable:
        case PhasesLoadUnload::UserPush_stoppable:
        case PhasesLoadUnload::UserPush_unstoppable:
        case PhasesLoadUnload::MakeSureInserted_stoppable:
        case PhasesLoadUnload::MakeSureInserted_unstoppable:
        case PhasesLoadUnload::IsFilamentInGear:
        case PhasesLoadUnload::IsColor:
        case PhasesLoadUnload::IsColorPurge:
            return StateAnimation::WaitingForUser;

        case PhasesLoadUnload::FilamentStuck:
            return StateAnimation::Warning;

        // Other phases, let them be handled by the printer state in next switch
        case PhasesLoadUnload::initial:
        case PhasesLoadUnload::_cnt:
            break;
        }
    }
#endif
    const marlin_server::State printer_state = marlin_vars().print_state;

    switch (printer_state) {
    case State::Idle:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
#if HAS_TOOL_MAPPING()
    case State::PrintPreviewToolsMapping:
#endif
    case State::Exit:
        return StateAnimation::Idle;

    case State::Printing:
    case State::PrintInit:
    case State::SerialPrintInit:
    case State::Finishing_WaitIdle:
    case State::Finishing_ParkHead:
    case State::Finishing_UnloadFilament:
    case State::Pausing_Begin:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
    case State::Resuming_BufferData:
    case State::Resuming_Begin:
    case State::Resuming_Reheating:
    case State::Resuming_ExecutingGCodeInterrupt:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE: {
        if (load_unload_state.has_value() || is_preheating) {
            return StateAnimation::Warning;
        } else {
            return StateAnimation::Printing;
        }
    }
    case State::Paused:
    case State::MediaErrorRecovery_BufferData:
        return StateAnimation::Warning;

    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_ParkHead:
    case State::Aborting_Preview:
    case State::Aborting_UnloadFilament:
    case State::Aborted:
        return StateAnimation::Aborted;

    case State::Finished:
        return StateAnimation::Finished;

    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
#if HAS_INDX()
    case State::PowerPanic_FinishIndxToolchange:
#endif
    case State::CrashRecovery_XY_Measure:
#if HAS_TOOL_CRASH_RECOVERY()
    case State::CrashRecovery_Tool_Pickup:
#endif
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
        return StateAnimation::Warning;

    case State::PowerPanic_acFault:
    case State::PowerPanic_Resume:
    case State::PowerPanic_AwaitingResume:
        return StateAnimation::PowerPanic;
    }
    return StateAnimation::Idle;
}
namespace {

    using StateAnimationController = AnimationController<FrameAnimation, 3>;

    constexpr auto solid = std::to_array<FrameAnimation<3>::Frame>({ { 100, 100, 100 } });

    constexpr auto pulsing = std::to_array<FrameAnimation<3>::Frame>({ { 100, 100, 100 },
        { 0, 0, 0 } });
#if PRINTER_IS_PRUSA_iX()
    constexpr auto running_left = std::to_array<FrameAnimation<3>::Frame>({ { 0, 0, 100 },
        { 0, 100, 0 }, { 100, 0, 0 } });
    constexpr auto running_right = std::to_array<FrameAnimation<3>::Frame>({ { 100, 0, 0 },
        { 0, 100, 0 }, { 0, 0, 100 } });
    constexpr auto alternating = std::to_array<FrameAnimation<3>::Frame>({ { 100, 0, 100 },
        { 0, 100, 0 } });
    constexpr auto pulsing_left = std::to_array<FrameAnimation<3>::Frame>({ { 100, 0, 0 },
        { 0, 0, 0 } });
    constexpr auto pulsing_right = std::to_array<FrameAnimation<3>::Frame>({ { 0, 0, 100 },
        { 0, 0, 0 } });
#endif
    constexpr EnumArray<StateAnimation, typename FrameAnimation<3>::Params, static_cast<int>(StateAnimation::_last) + 1> animations {
#if PRINTER_IS_PRUSA_iX()
        { StateAnimation::Idle, { { 0, 0, 255 }, 1000, 0, 400, solid } },
            { StateAnimation::Printing, { { 0, 255, 0 }, 1000, 0, 400, solid } },
            { StateAnimation::Finished, { { 0, 0, 255 }, 500, 0, 250, pulsing } },
            { StateAnimation::Aborted, { { 0, 0, 255 }, 1000, 0, 400, solid } },
            { StateAnimation::Warning, { { 128, 32, 0 }, 1000, 0, 1000, pulsing } },
            { StateAnimation::PowerPanic, { { 0, 255, 0 }, 1000, 0, 400, solid } },
#else
        { StateAnimation::Idle, { { 0, 0, 0 }, 1000, 0, 400, solid } },
            { StateAnimation::Printing, { { 0, 150, 255 }, 1000, 0, 400, solid } },
            { StateAnimation::Finished, { { 0, 255, 0 }, 1000, 0, 400, solid } },
            { StateAnimation::Aborted, { { 0, 0, 0 }, 1000, 0, 400, solid } },
            { StateAnimation::Warning, { { 255, 255, 0 }, 1000, 0, 1000, pulsing } },
            { StateAnimation::PowerPanic, { { 0, 0, 0 }, 1000, 0, 400, solid } },
#endif
            { StateAnimation::PowerUp, { { 0, 255, 0 }, 1500, 0, 1500, pulsing } },
#if PRINTER_IS_PRUSA_iX()
            { StateAnimation::WaitingForPrinter, { { 0, 0, 255 }, 500, 0, 250, alternating } },
            { StateAnimation::WaitingForUser, { { 0, 0, 255 }, 250, 0, 100, alternating } },
            { StateAnimation::Unloading, { { 0, 0, 255 }, 500, 0, 0, pulsing_right } },
            { StateAnimation::WaitingForFilamentRemoval, { { 0, 0, 255 }, 500, 250, 0, running_left } },
            { StateAnimation::FilamentRemoved, { { 0, 0, 255 }, 500, 0, 0, pulsing_left } },
            { StateAnimation::Inserting, { { 0, 0, 255 }, 500, 250, 0, running_right } },
            { StateAnimation::Loading, { { 0, 0, 255 }, 250, 0, 0, running_right } },
#endif
            { StateAnimation::Error, { { 255, 0, 0 }, 500, 0, 500, pulsing } },
    };

    constexpr EnumArray<AnimationType, std::span<const FrameAnimation<3>::Frame>, static_cast<int>(AnimationType::_last) + 1> custom_frames {
#if PRINTER_IS_PRUSA_iX()
        { AnimationType::RunningLeft, running_left },
            { AnimationType::RunningRight, running_right },
            { AnimationType::PulsingLeft, pulsing_left },
            { AnimationType::PulsingRight, pulsing_right },
            { AnimationType::Alternating, alternating },
#endif
            { AnimationType::Solid, solid },
            { AnimationType::Pulsing, pulsing },
    };

} // namespace

StateAnimationController &controller_instance() {
    static StateAnimationController instance { animations[StateAnimation::Idle] };
    return instance;
}

StatusLedsHandler &StatusLedsHandler::instance() {
    static StatusLedsHandler instance;
    return instance;
}

void StatusLedsHandler::set_error() {
    std::lock_guard lock(mutex);
    is_error_state = true;
}

void StatusLedsHandler::set_animation(StateAnimation state) {
    std::lock_guard lock(mutex);
    controller_instance().set(animations[state]);
}

ColorRGBW StatusLedsHandler::get_color() const {
    return color;
}

StateAnimation StatusLedsHandler::current_animation() {
    std::lock_guard lock(mutex);
    return is_error_state ? StateAnimation::Error : marlin_to_anim_state();
}

bool StatusLedsHandler::get_active() {
    std::lock_guard lock(mutex);
    return active;
}

void StatusLedsHandler::set_custom_animation(const ColorRGBW &color, AnimationType type, uint16_t period_ms) {
    std::lock_guard lock(mutex);
    auto &controller = controller_instance();

    auto &custom_params = custom_params_banks[custom_params_bank_index];

    custom_params.color = color;
    custom_params.frames = custom_frames[type];

    if (period_ms > 0) {
        custom_params.frame_length = period_ms / custom_params.frames.size();
        custom_params.blend_time = custom_params.frame_length / 4;
    } else {
        custom_params.frame_length = 1000;
        custom_params.blend_time = 300;
    }

    controller.set(custom_params);
    custom_params_bank_index = custom_params_bank_index > 0 ? 0 : 1;
}

void StatusLedsHandler::set_active(bool val) {
    std::lock_guard lock(mutex);
    active = val;
    config_store().run_leds.set(val);
}

void StatusLedsHandler::update() {
    std::lock_guard lock(mutex);

    StateAnimation state;
    if (!active) {
        state = StateAnimation::Idle; // assuming LEDs are off in Idle
    } else if (is_error_state) {
        state = StateAnimation::Error;
    } else {
        state = marlin_to_anim_state();
    }

    const auto &animation = animations[state];
    color = animation.color;
    if (state != old_state) {
        old_state = state;
        controller_instance().set(animation);
    }

    controller_instance().update();
}

std::span<const ColorRGBW, 3> StatusLedsHandler::led_data() {
    std::lock_guard lock(mutex);
    return controller_instance().data();
}

} // namespace leds
