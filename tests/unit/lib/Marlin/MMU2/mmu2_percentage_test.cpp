#include "catch2/catch.hpp"

#include <mmu2_progress.hpp>

using namespace MMU2;

constexpr float StepOf(uint8_t step, uint8_t max) {
    return step * 100 / (max + 1);
}

// Need some simple infrastructure to repeat and/or drop some steps and a cartesian crossproduct out of it
#define REPEAT(min_, max_, progressCode, stepOf)                                                                     \
    {                                                                                                                \
        uint8_t steps = GENERATE(range(min_, max_ + 1));                                                             \
        for (uint8_t i = 0; i < steps; ++i) {                                                                        \
            pct.ProcessReport(ProgressData(pct.GetCommandInProgress(), static_cast<RawProgressCode>(progressCode))); \
            CHECK(float(pct.GetProgressPercentage()) == StepOf(stepOf, maxSteps));                                   \
        }                                                                                                            \
    }

TEST_CASE("MMU2::MMU2::PCT CutFilament", "[MMU2][pct]") {
    ProgressTrackingManager pct;
    pct.ProcessReport(MMU2::ProgressData(MMU2::CommandInProgress::CutFilament));

    // step usual progress codes and check the percentage (beware - hex values)
    static constexpr uint8_t maxSteps = 10;

    // unload filament usually doesn't happen
    REPEAT(0, 3, ProgressCode::EngagingIdler, 0);
    REPEAT(0, 3, ProgressCode::UnloadingFilament, 1);
    REPEAT(0, 1, ProgressCode::SelectingFilamentSlot, 2); // K0 P12*90.
    REPEAT(1, 3, ProgressCode::FeedingToFinda, 3); // K0 P5*ac.
    REPEAT(0, 1, ProgressCode::RetractingFromFinda, 4); // retracting from FINDA may be reported here, usually it is not
    REPEAT(0, 1, ProgressCode::PreparingBlade, 5); // K0 P13*85.
    REPEAT(0, 1, ProgressCode::PushingFilament, 6); // K0 P14*ee.
    REPEAT(0, 1, ProgressCode::DisengagingIdler, 7); // K0 P2*c7.
    REPEAT(0, 1, ProgressCode::PerformingCut, 8); // K0 P15*fb.
    REPEAT(1, 3, ProgressCode::Homing, 9); // K0 P1a*38.
    REPEAT(1, 3, ProgressCode::ReturningSelector, 10); // K0 P16*c4.
}

TEST_CASE("MMU2::MMU2::PCT EjectFilament", "[MMU2][pct]") {
    ProgressTrackingManager pct;
    pct.ProcessReport(MMU2::ProgressData(MMU2::CommandInProgress::EjectFilament));

    static constexpr uint8_t maxSteps = 5;

    REPEAT(0, 3, ExtendedProgressCode::WaitingForTemperature, 0);
    REPEAT(0, 2, ExtendedProgressCode::UnloadingFromExtruder, 1); // E0 P17*f7.
    REPEAT(0, 3, ProgressCode::UnloadingToFinda, 2);
    REPEAT(0, 2, ProgressCode::ParkingSelector, 3); // E0 P17*f7.
    REPEAT(0, 1, ProgressCode::EngagingIdler, 4); // E0 P1*de.
    REPEAT(1, 3, ProgressCode::EjectingFilament, 5); // E0 P18*34.
    // E0 E800c*dc. repeated many times, depends on the user
}

TEST_CASE("MMU2::MMU2::PCT Preload2MMU", "[MMU2][pct]") {
    ProgressTrackingManager pct;
    pct.ProcessReport(MMU2::ProgressData(MMU2::CommandInProgress::LoadFilament));

    // step usual progress codes and check the percentage (beware - hex values)
    static constexpr uint8_t maxSteps = 3;
    REPEAT(1, 3, ProgressCode::EngagingIdler, 0);
    REPEAT(1, 3, ProgressCode::FeedingToFinda, 1); // L0 P5*bf.
    REPEAT(1, 2, ProgressCode::RetractingFromFinda, 2); // L0 P19*14.
    REPEAT(1, 3, ProgressCode::DisengagingIdler, 3); // L0 P2*d4.
}

TEST_CASE("MMU2::MMU2::PCT ToolChange", "[MMU2][pct]") {
    ProgressTrackingManager pct;
    pct.ProcessReport(MMU2::ProgressData(MMU2::CommandInProgress::ToolChange));

    static constexpr uint8_t maxSteps = 7;
    REPEAT(1, 3, ProgressCode::UnloadingToFinda, 1); // T0 P3*f8. UnloadingToFinda
    REPEAT(1, 1, ProgressCode::RetractingFromFinda, 2); // T0 P19*2d. RetractingFromFinda
    REPEAT(0, 1, ProgressCode::DisengagingIdler, 2); // T0 P2*ed. DisengagingIdler - keeps the same status as previous

    REPEAT(1, 3, ProgressCode::FeedingToFinda, 4); // T0 P5*86. FeedingToFinda
    REPEAT(1, 2, ProgressCode::FeedingToBondtech, 5); // T0 P6*b9. FeedingToBondtech
    REPEAT(1, 3, ProgressCode::FeedingToFSensor, 6); // T0 P1c*6c. FeedingToFSensor
    REPEAT(0, 1, ProgressCode::DisengagingIdler, 6); // T0 P2*ed. DisengagingIdler - keeps the same status as previous
}
