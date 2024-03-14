/// @file command_base.h
#pragma once
#include <stdint.h>
#include "error_codes.h"
#include "progress_codes.h"
#include "result_codes.h"

namespace modules {
namespace motion {
class MovableBase;
}
}

/// The logic namespace handles the application logic on top of the modules.
namespace logic {

/// Bitwise OR (ErrorCode::TMC_PULLEY_BIT << axis) into ec
/// where axis ranges from 0 to 2
ErrorCode AddErrorAxisBit(ErrorCode ec, uint8_t axis);

/// @brief Base class defining common API for high-level operations/commands/state machines
///
/// Which state machines are high-level? Those which are being initiated either by a command over the serial line or from a button
/// - they report their progress to the printer
/// - they can be composed of other sub automata
///
/// Tasks derived from this base class are the top-level operations inhibited by the printer.
/// These tasks report their progress and only one of these tasks is allowed to run at once.
class CommandBase {
public:
    inline constexpr CommandBase()
        : state(ProgressCode::OK)
        , error(ErrorCode::OK)
        , deferredErrorCode(ErrorCode::OK)
        , stateBeforeModuleFailed(ProgressCode::Empty)
        , errorBeforeModuleFailed(ErrorCode::OK)
        , recoveringMovableErrorAxisMask(0) {}

    // Normally, a base class should (must) have a virtual destructor to enable correct deallocation of superstructures.
    // However, in our case we don't want ANY destruction of these objects and moreover - adding a destructor like this
    // makes the linker complain about missing operator delete(), which is really not something we want/need in our case.
    // Without the destructor, the linker is "happy" ;)
    // virtual ~CommandBase() = default;

    /// resets the automaton
    /// @param param numerical parameter that comes with some commands (e.g. T1 for tool change 1)
    /// @returns true if the command was accepted and started (which may not be possible e.g. due to filament position)
    virtual bool Reset(uint8_t param) = 0;

    /// Steps the state machine. This is the preferred way of stepping the machine
    /// as it handles the global HW error states uniformly (so that the derived classes do not have to deal
    /// with these error states on their own).
    /// Each derived class then only implements its own logic via the virtual #StepInner method.
    /// @returns true if the automaton finished its work
    bool Step();

    /// Each derived class shall implement its own state machine logic in this method
    /// It is being called from #Step after the HW error states have been checked
    virtual bool StepInner() = 0;

    /// @returns progress of operation - each automaton consists of several internal states
    /// which should be reported to the user via the printer's LCD
    /// E.g. Tool change: first tries to unload filament, then selects another slot and then tries to load filament
    ///
    /// Beware - derived automata report detailed states of underlying state machines if any
    /// E.g. Eject filament first tries to unload filament, which is a standalone automaton.
    /// Therefore until the unload is finished, this method will report the internal state of Unload filament.
    /// The reason for this is to be able to report exactly what is happening to the printer, especially loading and unloading sequences (and errors)
    virtual ProgressCode State() const { return state; }

    /// @returns progress of operation of only this state machine - regardless of any underlying automata (if any)
    /// Therefore it is not a vitual method.
    ProgressCode TopLevelState() const { return state; }

    /// @returns status of the operation - e.g. RUNNING, OK, or an error code if the operation failed.
    ///
    /// Beware - the same rule about composite operations as with State() applies to Error() as well.
    /// Please see ErrorCode for more details
    virtual ErrorCode Error() const { return error; }

    /// @returns result of a command - only valid after the command finished its work.
    /// Default returned value is OK for all commands.
    /// So far there is only one example usage: LoadFilament can be terminated with a button -> Result will be Cancelled.
    /// The printer then can display "Loading cancelled"
    virtual ResultCode Result() const { return ResultCode::OK; }

    /// Switches the state machine into an error state of code ec.
    /// It shall be used to halt the firmware while retaining the capability of reporting the error state to the printer
    /// - a kind of similar to runtime assertions.
    /// Called from main.cpp's global funtion Panic() .
    /// The derived state machines have no (implemented) way of getting out of this state (intentionally).
    /// The only way out is to reset the board.
    void Panic(ErrorCode ec);

    /// Invalidates homing state on Idler and Selector - doesn't change anything about filament load status
    static void InvalidateHoming();

    /// Invalidates homing state on Idler and Selector + resets the knowledge about
    /// filament presence according to known sensors (FINDA+FSensor)
    static void InvalidateHomingAndFilamentState();

    /// Put Idler and Selector on-hold - they shall not move (not even home) until ResumeIdlerSelector is called
    static void HoldIdlerSelector();

    /// Allow Idler and Selector to move/home again. Any move needs to be newly planned.
    static void ResumeIdlerSelector();

#ifndef UNITTEST
protected:
#endif
    /// @returns true if the slot/tool index is within specified range (0 - config::toolCount)
    /// If not, it returns false and sets the error to ErrorCode::INVALID_TOOL
    bool CheckToolIndex(uint8_t index);

    /// Checks for errors of modules - that includes TMC errors, Idler and Selector errors and possibly more.
    /// The idea is to check blocking errors at one spot consistently.
    /// Some of the detected errors can be irrecoverable (i.e. need power cycling the MMU).
    /// @returns true if waiting for a recovery, false if the state machine can continue.
    bool WaitForModulesErrorRecovery();

    /// @returns true when still waiting for a module to recover, false otherwise.
    bool WaitForOneModuleErrorRecovery(ErrorCode iState, modules::motion::MovableBase &m, uint8_t axisMask);

    /// Perform disengaging idler in ErrDisengagingIdler state
    void ErrDisengagingIdler();

    /// Transit the state machine into ErrDisengagingIdler
    void GoToErrDisengagingIdler(ErrorCode deferredEC);

    /// Transit the state machine into ErrEngagingIdler
    void GoToErrEngagingIdler();

    /// Process end of command which finished OK
    void FinishedOK();

    ProgressCode state; ///< current progress state of the state machine
    ErrorCode error; ///< current error code
    ErrorCode deferredErrorCode; ///< planned error code - occurs when doing GoToErrDisengagingIdler - after the idler disengaged, the error is set (not before)
    ProgressCode stateBeforeModuleFailed; ///< saved state of the state machine before a common error happened
    ErrorCode errorBeforeModuleFailed; ///< saved error of the state machine before a common error happened
    uint8_t recoveringMovableErrorAxisMask;
};

} // namespace logic
