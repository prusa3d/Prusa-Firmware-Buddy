#include "mmu2_progress.hpp"

#include <array>
#include <algorithm>

/// Convenience prefix +op for enum values, works same as to_underlying
/// It's being used a lot in this file so writing to_underlying everywhere would be a mess
template <typename T>
static constexpr auto operator+(T e) noexcept
    -> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>> {
    return static_cast<std::underlying_type_t<T>>(e);
}

namespace MMU2 {

static constexpr bool IsErrorCode(ProgressCode ec) {
    switch (ec) {

    case ProgressCode::ERRDisengagingIdler:
    case ProgressCode::ERRWaitingForUser:
    case ProgressCode::ERREngagingIdler:
    case ProgressCode::ERRHelpingFilament:
        return true;

    default:
        return false;
    }
}

void ProgressTrackingManager::ProcessReport(ProgressData d) {
    // Set by default, gets overwritten for specific cases
    loadUnloadMode_ = LoadUnloadMode::Load;

    // Reset progress percentage when starting a new command
    if ((progressCode_ == +ProgressCode::OK && d.progressCode != ProgressCode::OK) || d.rawCommandInProgress != commandInProgress_) {
        progressPercentage_ = 0;
    }

    const auto calcProgress = [&]<auto arr>() -> void {
        static_assert(arr.size() > 2);
        static constexpr int maxUsedProgressCode = std::ranges::max(arr);

        using StepIndex = uint8_t;
        using LookupTable = std::array<StepIndex, maxUsedProgressCode + 1>;

        static constexpr StepIndex invalidStepIndex = -1;
        static constexpr StepIndex repeatingStepIndex = -2;

        /// Progress code -> step index lookup table
        static constexpr LookupTable lookupTable = [] {
            LookupTable r;
            r.fill(invalidStepIndex);

            int i = 0;
            for (const auto pc : arr) {
                auto &rec = r[+pc];

                if (rec != invalidStepIndex) {
                    rec = repeatingStepIndex;
                } else {
                    rec = i;
                }

                i++;
            }

            return r;
        }();

        // Do not update progress percentage on step index that is outside of the table bounds
        if (d.rawProgressCode >= lookupTable.size()) {
            return;
        }

        auto stepIndex = lookupTable[d.rawProgressCode];

        // Do not update progress percentage on step index that is not in the table
        // or that is there multiple times
        if (stepIndex == invalidStepIndex || stepIndex == repeatingStepIndex) {
            return;
        }

        const float stepProgress = std::clamp(d.stepProgressPercent / 100.0f, 0.0f, 1.0f);
        progressPercentage_ = static_cast<uint8_t>((stepIndex + stepProgress) * 100.0f / arr.size());
    };

    switch (d.rawCommandInProgress) {

    case CommandInProgress::CutFilament: {
        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ProgressCode::EngagingIdler,
            +ProgressCode::UnloadingFilament,
            +ProgressCode::SelectingFilamentSlot,
            +ProgressCode::FeedingToFinda,
            +ProgressCode::RetractingFromFinda,
            +ProgressCode::PreparingBlade,
            +ProgressCode::PushingFilament,
            +ProgressCode::DisengagingIdler,
            +ProgressCode::PerformingCut,
            +ProgressCode::Homing,
            +ProgressCode::ReturningSelector,
        };
        calcProgress.operator()<steps>();
        loadUnloadMode_ = LoadUnloadMode::Cut;
        break;
    }

    case CommandInProgress::EjectFilament: {
        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ExtendedProgressCode::WaitingForTemperature,
            +ExtendedProgressCode::UnloadingFromExtruder,
            +ProgressCode::UnloadingToFinda,
            +ProgressCode::ParkingSelector,
            +ProgressCode::EngagingIdler,
            +ProgressCode::EjectingFilament,
        };
        calcProgress.operator()<steps>();
        loadUnloadMode_ = LoadUnloadMode::Eject;
        break;
    }

    case CommandInProgress::Homing:
        progressPercentage_ = 50;
        break;

    case CommandInProgress::LoadFilament: {
        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ProgressCode::EngagingIdler,
            +ProgressCode::FeedingToFinda,
            +ProgressCode::RetractingFromFinda,
            +ProgressCode::DisengagingIdler,
        };
        calcProgress.operator()<steps>();
        break;
    }

    case static_cast<RawCommandInProgress>(ExtendedCommandInProgress::LoadToNozzle): {
        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ExtendedProgressCode::WaitingForTemperature,

            +ProgressCode::FeedingToFinda,
            +ProgressCode::FeedingToBondtech,
            +ProgressCode::FeedingToFSensor,
            +ProgressCode::DisengagingIdler,

            +ExtendedProgressCode::LoadingToNozzle,
        };
        calcProgress.operator()<steps>();
        break;
    }

    case CommandInProgress::Reset:
        progressPercentage_ = 50;
        break;

    case TestLoad: // test load is almost the same like a toolchange, just different visualization
    case ToolChange: {
        // disengaging idler comes 2x at different spots, not necessary for visualization of progress

        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ProgressCode::EngagingIdler,
            +ProgressCode::UnloadingToFinda,
            +ProgressCode::RetractingFromFinda,
            +ProgressCode::DisengagingIdler,

            +ProgressCode::FeedingToFinda,
            +ProgressCode::FeedingToBondtech,
            +ProgressCode::FeedingToFSensor,
            +ProgressCode::DisengagingIdler,
        };
        calcProgress.operator()<steps>();
        loadUnloadMode_ = d.commandInProgress == CommandInProgress::TestLoad ? LoadUnloadMode::Test : LoadUnloadMode::Load;
        break;
    }

    case UnloadFilament: {
        // Using +op prefix operator defined on the beginning of this file, works same as to_underlying
        static constexpr std::array steps = {
            +ExtendedProgressCode::WaitingForTemperature,
            +ExtendedProgressCode::Ramming,
            +ProgressCode::UnloadingToFinda,
            // +ProgressCode::RetractingFromFinda, Too short, we can skip
            // +ProgressCode::Homing, Too short, we can skip
            +ProgressCode::DisengagingIdler,
        };
        calcProgress.operator()<steps>();
        loadUnloadMode_ = LoadUnloadMode::Unload;
        break;
    }

    default:
        progressPercentage_ = 0;
        break;
    }

    commandInProgress_ = d.rawCommandInProgress;
    progressCode_ = d.rawProgressCode;
}

} // namespace MMU2
