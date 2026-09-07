#include "filament_change_phases.hpp"

#include <option/has_mmu2.h>

namespace ClientResponses {

// declare 2d arrays of single buttons for radio buttons
constinit const EnumArray<PhasesLoadUnload, PhaseResponses, CountPhases<PhasesLoadUnload>()> LoadUnloadResponses {
    { PhasesLoadUnload::initial, {} },
        { PhasesLoadUnload::ChangingTool, {} },
        { PhasesLoadUnload::Parking_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Parking_unstoppable, {} },
        { PhasesLoadUnload::WaitingTemp_stoppable, { Response::Stop } },
        { PhasesLoadUnload::WaitingTemp_unstoppable, {} },
        { PhasesLoadUnload::Ramming_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Ramming_unstoppable, {} },
        { PhasesLoadUnload::Unloading_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Unloading_unstoppable, {} },
        { PhasesLoadUnload::IsFilamentUnloaded, { Response::Yes, Response::No } },
        { PhasesLoadUnload::FilamentNotInFS, { Response::Help } },
        { PhasesLoadUnload::ManualUnload_continuable, { Response::Continue, Response::Retry } },
        { PhasesLoadUnload::ManualUnload_uncontinuable, { Response::Help, Response::Retry } },
        { PhasesLoadUnload::UserPush_stoppable, { Response::Continue, Response::Stop } },
        { PhasesLoadUnload::UserPush_unstoppable, { Response::Continue } },
        { PhasesLoadUnload::MakeSureInserted_stoppable, { Response::Stop, Response::Help } },
        { PhasesLoadUnload::MakeSureInserted_unstoppable, { Response::Help } },
        { PhasesLoadUnload::Inserting_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Inserting_unstoppable, {} },
        { PhasesLoadUnload::IsFilamentInGear, { Response::Yes, Response::No } },
        { PhasesLoadUnload::Ejecting_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Ejecting_unstoppable, {} },
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
        { PhasesLoadUnload::LoadingObstruction_stoppable, { Response::Retry, Response::Stop } },
        { PhasesLoadUnload::LoadingObstruction_unstoppable, { Response::Retry, Response::Help } },
#endif
        { PhasesLoadUnload::Loading_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Loading_unstoppable, {} },
        { PhasesLoadUnload::LoadingToGears_stoppable, { Response::Stop } },
        { PhasesLoadUnload::LoadingToGears_unstoppable, {} },
        { PhasesLoadUnload::Purging_stoppable, { Response::Stop } },
        { PhasesLoadUnload::Purging_unstoppable, {} },
        { PhasesLoadUnload::AwaitingFilament_stoppable, { Response::Stop } },
        { PhasesLoadUnload::AwaitingFilament_unstoppable, {} },
        { PhasesLoadUnload::IsColor, { Response::Yes, Response::Purge_more, Response::Retry } },
        { PhasesLoadUnload::IsColorPurge, { Response::Yes, Response::Purge_more } },
        { PhasesLoadUnload::Unparking, {} },
#if HAS_NOZZLE_CLEANER()
        { PhasesLoadUnload::UnloadNozzleCleaning, {} },
        { PhasesLoadUnload::LoadNozzleCleaning, {} },
#endif
#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
        { PhasesLoadUnload::FilamentStuck, { Response::Unload, Response::Ignore, Response::Disable } },
#endif
#if HAS_AUTO_RETRACT()
        { PhasesLoadUnload::AutoRetracting, {} },
#endif
#if HAS_ANFC()
        { PhasesLoadUnload::OPT_UncommitedUsage, { Response::Ignore } },
#endif

#if HAS_INDX()
        { PhasesLoadUnload::FilamentCalibrationFailed, { Response::Retry, Response::Unload } },
#endif
#if HAS_MMU2()
        { PhasesLoadUnload::LoadFilamentIntoMMU, { Response::Continue } },
        { PhasesLoadUnload::MMUDummyStartNoAttention, {} },

        { PhasesLoadUnload::MMU_EngagingIdler, {} },
        { PhasesLoadUnload::MMU_DisengagingIdler, {} },
        { PhasesLoadUnload::MMU_UnloadingToFinda, {} },
        { PhasesLoadUnload::MMU_UnloadingToPulley, {} },
        { PhasesLoadUnload::MMU_FeedingToFinda, {} },
        { PhasesLoadUnload::MMU_FeedingToBondtech, {} },
        { PhasesLoadUnload::MMU_FeedingToNozzle, {} },
        { PhasesLoadUnload::MMU_AvoidingGrind, {} },
        { PhasesLoadUnload::MMU_FinishingMoves, {} },
        { PhasesLoadUnload::MMU_ERRDisengagingIdler, {} },
        { PhasesLoadUnload::MMU_ERREngagingIdler, {} },
        { PhasesLoadUnload::MMU_ERRWaitingForUser, { Response::Retry, Response::Slowly, Response::Continue, Response::Restart, Response::Unload, Response::Stop, Response::MMU_disable } },
        { PhasesLoadUnload::MMU_ERRInternal, {} },
        { PhasesLoadUnload::MMU_ERRHelpingFilament, {} },
        { PhasesLoadUnload::MMU_ERRTMCFailed, {} },
        { PhasesLoadUnload::MMU_UnloadingFilament, {} },
        { PhasesLoadUnload::MMU_LoadingFilament, {} },
        { PhasesLoadUnload::MMU_SelectingFilamentSlot, {} },
        { PhasesLoadUnload::MMU_PreparingBlade, {} },
        { PhasesLoadUnload::MMU_PushingFilament, {} },
        { PhasesLoadUnload::MMU_PerformingCut, {} },
        { PhasesLoadUnload::MMU_ReturningSelector, {} },
        { PhasesLoadUnload::MMU_ParkingSelector, {} },
        { PhasesLoadUnload::MMU_EjectingFilament, {} },
        { PhasesLoadUnload::MMU_RetractingFromFinda, {} },
        { PhasesLoadUnload::MMU_Homing, {} },
        { PhasesLoadUnload::MMU_MovingSelector, {} },
        { PhasesLoadUnload::MMU_FeedingToFSensor, {} },
        { PhasesLoadUnload::MMU_HWTestBegin, {} },
        { PhasesLoadUnload::MMU_HWTestIdler, {} },
        { PhasesLoadUnload::MMU_HWTestSelector, {} },
        { PhasesLoadUnload::MMU_HWTestPulley, {} },
        { PhasesLoadUnload::MMU_HWTestCleanup, {} },
        { PhasesLoadUnload::MMU_HWTestExec, {} },
        { PhasesLoadUnload::MMU_HWTestDisplay, {} },
        { PhasesLoadUnload::MMU_ErrHwTestFailed, {} },
#endif
};

} // namespace ClientResponses
