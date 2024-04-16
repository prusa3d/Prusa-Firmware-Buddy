#pragma once

#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>

namespace marlin_server {

enum class State {
    Idle,
    WaitGui,
    PrintPreviewInit, ///< Print is being initialized
    PrintPreviewImage, ///< Showing print preview and waiting for user to click print
    PrintPreviewConfirmed, ///< Print is confirmed to be printed (either user clicked print, or WUI/Connect started print without confirmation on printer)
    PrintPreviewQuestions, ///< Some problems with print detected, ask user to skip/fix them
#if HAS_TOOLCHANGER() || HAS_MMU2()
    PrintPreviewToolsMapping, ///< Waiting for user to do the tool mapping/spool join
#endif
    PrintInit,
    SerialPrintInit,
    Printing,
    Pausing_Begin,
    Pausing_Failed_Code,
    Pausing_WaitIdle,
    Pausing_ParkHead,
    Paused,
    Resuming_Begin,
    Resuming_Reheating,
    Resuming_UnparkHead_XY,
    Resuming_UnparkHead_ZE,
    Aborting_Begin,
    Aborting_WaitIdle,
    Aborting_UnloadFilament,
    Aborting_ParkHead,
    Aborting_Preview, ///< Print preview can do filament load/unload and needs to be aborted properly
    Aborted,
    Finishing_WaitIdle,
    Finishing_UnloadFilament,
    Finishing_ParkHead,
    Finished,
    Exit, // sets idle, notifies clients to close fsm
    CrashRecovery_Begin,
    CrashRecovery_Retracting,
    CrashRecovery_Lifting,
    CrashRecovery_ToolchangePowerPanic, // Prepare for toolchange after power panic
    CrashRecovery_XY_Measure,
#if HAS_TOOLCHANGER()
    CrashRecovery_Tool_Pickup,
#endif
    CrashRecovery_XY_HOME,
    CrashRecovery_HOMEFAIL, // Shows retry button after homing fails
    CrashRecovery_Axis_NOK,
    CrashRecovery_Repeated_Crash,
    PowerPanic_acFault,
    PowerPanic_Resume,
    PowerPanic_AwaitingResume,
};

} // namespace marlin_server
