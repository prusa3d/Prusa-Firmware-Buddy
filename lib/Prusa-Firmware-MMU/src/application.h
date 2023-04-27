/// @file
#pragma once
#include <stdint.h>
#include "modules/timebase.h"
#include "modules/protocol.h"
#include "logic/error_codes.h"

namespace logic {
class CommandBase;
}

class Application {
public:
    Application();

    inline void CommandFinishedCorrectly() {
        lastCommandProcessedMs = mt::timebase.Millis();
    }

    /// Perform firmware panic handling
    void Panic(ErrorCode ec);

    /// Perform one step of top level
    void Step();

    uint8_t CurrentProgressCode();
    uint16_t CurrentErrorCode();

    /// Types of Reset parameters
    enum class ResetTypes : uint8_t {
        Software = 0,
        EEPROMAndSoftware = 42,
    };

    /// Performs a reset of the MMU board.
    /// @param resetType == EEPROMAndSoftware, the EEPROM is cleared before resetting the board. Otherwise the MMU board "just" resets.
    void ProcessReset(ResetTypes resetType);

#ifndef UNITTEST
private:
#endif
    /// Checks if the MMU can enter manual mode (user can move the selector with buttons)
    /// The MMU enters idle mode after 5s from the last command finished and there must be no filament present in the selector.
    void CheckManualOperation();

    /// Checks messages on the UART
    bool CheckMsgs();

    /// Tries to plan a logic::command if possible
    void PlanCommand(const mp::RequestMsg &rq);

    mp::ResponseCommandStatus RunningCommandStatus() const;
    void ReportCommandAccepted(const mp::RequestMsg &rq, mp::ResponseMsgParamCodes status);
    void ReportFINDA(const mp::RequestMsg &rq);
    void ReportRunningCommand();
    void ReportReadRegister(const mp::RequestMsg &rq);
    void ReportWriteRegister(const mp::RequestMsg &rq);
    void ProcessRequestMsg(const mp::RequestMsg &rq);

    uint16_t lastCommandProcessedMs;

    /// A command that resulted in the currently on-going operation
    logic::CommandBase *currentCommand;

    /// Remember the request message that started the currently running command
    /// For the start we report "Reset finished" which in fact corresponds with the MMU state pretty closely
    /// and plays nicely even with the protocol implementation.
    /// And, since the default startup command is the noCommand, which always returns "Finished"
    /// the implementation is clean and straightforward - the response to the first Q0 messages
    /// will look like "X0 F" until a command (T, L, U ...) has been issued.
    mp::RequestMsg currentCommandRq;
};

extern Application application;
