/// @file
#include "application.h"
#include "registers.h"

#include "modules/finda.h"
#include "modules/fsensor.h"
#include "modules/globals.h"
#include "modules/leds.h"
#include "modules/permanent_storage.h"
#include "modules/serial.h"
#include "modules/user_input.h"

#include "logic/command_base.h"
#include "logic/cut_filament.h"
#include "logic/eject_filament.h"
#include "logic/home.h"
#include "logic/hw_sanity.h"
#include "logic/load_filament.h"
#include "logic/move_selector.h"
#include "logic/no_command.h"
#include "logic/set_mode.h"
#include "logic/start_up.h"
#include "logic/tool_change.h"
#include "logic/unload_filament.h"

#include "version.hpp"

#include "panic.h"

/// Global instance of the protocol codec
static mp::Protocol protocol;

Application application;

Application::Application()
    : lastCommandProcessedMs(0)
    , currentCommand(&logic::startUp)
    , currentCommandRq(mp::RequestMsgCodes::Reset, 0) {}

void __attribute__((noinline)) Application::CheckManualOperation() {
    uint16_t ms = mt::timebase.Millis();
    constexpr uint16_t idleDelay = 1000U;
    if (ms - lastCommandProcessedMs < idleDelay) {
        if (currentCommand->State() == ProgressCode::OK) {
            mui::userInput.Clear(); // consume bogus UI events while no command in progress and not in idle state yet
        }
        return;
    }

    lastCommandProcessedMs = ms - idleDelay; // prevent future overflows

    if (currentCommand->State() == ProgressCode::OK && mg::globals.FilamentLoaded() <= mg::FilamentLoadState::AtPulley) {
        if (mui::userInput.AnyEvent()) {
            switch (mui::userInput.ConsumeEvent()) {
            case mui::Event::Left:
                // move selector left if possible
                if (mg::globals.ActiveSlot() > 0) {
                    logic::moveSelector.Reset(mg::globals.ActiveSlot() - 1);
                    currentCommand = &logic::moveSelector;
                }
                break;
            case mui::Event::Middle:
                // plan load
                if (mg::globals.ActiveSlot() < config::toolCount) { // do we have a meaningful selector position?
                    logic::loadFilament.Reset(mg::globals.ActiveSlot());
                    currentCommand = &logic::loadFilament;
                }
                break;
            case mui::Event::Right:
                // move selector right if possible (including the park position)
                if (mg::globals.ActiveSlot() < config::toolCount) {
                    logic::moveSelector.Reset(mg::globals.ActiveSlot() + 1); // we allow also the park position
                    currentCommand = &logic::moveSelector;
                }
                break;
            default:
                break;
            }
        }
    }
}

mp::ResponseCommandStatus Application::RunningCommandStatus() const {
    if (mui::userInput.PrinterInCharge()) {
        mui::Event event = mui::userInput.ConsumeEventForPrinter();
        if (event != mui::Event::NoEvent) {
            return mp::ResponseCommandStatus(mp::ResponseMsgParamCodes::Button, (uint8_t)event);
        }
    }

    switch (currentCommand->Error()) {
    case ErrorCode::RUNNING:
        return mp::ResponseCommandStatus(mp::ResponseMsgParamCodes::Processing, (uint16_t)currentCommand->State());
    case ErrorCode::OK:
        return mp::ResponseCommandStatus(mp::ResponseMsgParamCodes::Finished, (uint16_t)currentCommand->Result());
    default:
        return mp::ResponseCommandStatus(mp::ResponseMsgParamCodes::Error, (uint16_t)currentCommand->Error());
    }
}

void Application::ReportCommandAccepted(const mp::RequestMsg &rq, mp::ResponseMsgParamCodes status) {
    uint8_t tmp[mp::Protocol::MaxResponseSize()];
    uint8_t len = protocol.EncodeResponseCmdAR(rq, status, tmp);
    modules::serial::WriteToUSART(tmp, len);
}

void __attribute__((noinline)) Application::PlanCommand(const modules::protocol::RequestMsg &rq) {
    if (currentCommand->State() == ProgressCode::OK) {
        // We are allowed to start a new command as the previous one is in the OK finished state
        // The previous command may be in an error state, but as long as it is in ProgressCode::OK (aka finished)
        // we are safe here. It is the responsibility of the printer to ask for a command error code
        // before issuing another one - if needed.
        switch (rq.code) {
        case mp::RequestMsgCodes::Cut:
            currentCommand = &logic::cutFilament;
            break;
        case mp::RequestMsgCodes::Eject:
            currentCommand = &logic::ejectFilament;
            break;
        case mp::RequestMsgCodes::Home:
            currentCommand = &logic::home;
            break;
        case mp::RequestMsgCodes::Load:
            currentCommand = &logic::loadFilament;
            break;
        case mp::RequestMsgCodes::Tool:
            currentCommand = &logic::toolChange;
            break;
        case mp::RequestMsgCodes::Unload:
            currentCommand = &logic::unloadFilament;
            break;
        case mp::RequestMsgCodes::Mode:
            currentCommand = &logic::setMode;
            break;
        default:
            currentCommand = &logic::noCommand;
            break;
        }
        currentCommandRq = rq; // save the Current Command Request for indentification of responses
        bool accepted = currentCommand->Reset(rq.value);
        ReportCommandAccepted(rq, accepted ? mp::ResponseMsgParamCodes::Accepted : mp::ResponseMsgParamCodes::Rejected);
    } else {
        ReportCommandAccepted(rq, mp::ResponseMsgParamCodes::Rejected);
    }
}

void Application::ReportFINDA(const mp::RequestMsg &rq) {
#ifdef DEBUG_FINDA
    using namespace hal;
    hu::usart1.puts("FINDA:");
    if (hal::gpio::ReadPin(FINDA_PIN) == hal::gpio::Level::high) {
        hu::usart1.puts(" TIRGGERED\n");
    } else {
        hu::usart1.puts(" NOT TRIGGERED\n");
    }
#endif //DEBUG_FINDA
    uint8_t rsp[mp::Protocol::MaxResponseSize()];
    uint8_t len = protocol.EncodeResponseReadFINDA(rq, mf::finda.Pressed(), rsp);
    modules::serial::WriteToUSART(rsp, len);
}

void Application::ReportRunningCommand() {
    uint8_t rsp[mp::Protocol::MaxResponseSize()];
    uint8_t len = protocol.EncodeResponseQueryOperation(currentCommandRq, RunningCommandStatus(), rsp);
    modules::serial::WriteToUSART(rsp, len);
}

void Application::ReportReadRegister(const mp::RequestMsg &rq) {
    uint8_t rsp[mp::Protocol::MaxResponseSize()];
    uint16_t value2;
    bool accepted = ReadRegister(rq.value, value2);
    uint8_t len = protocol.EncodeResponseRead(rq, accepted, value2, rsp);
    modules::serial::WriteToUSART(rsp, len);
}

void Application::ReportWriteRegister(const mp::RequestMsg &rq) {
    uint8_t rsp[mp::Protocol::MaxResponseSize()];
    mp::ResponseMsgParamCodes ar = WriteRegister(rq.value, rq.value2) ? mp::ResponseMsgParamCodes::Accepted : mp::ResponseMsgParamCodes::Rejected;
    uint8_t len = protocol.EncodeResponseCmdAR(rq, ar, rsp);
    modules::serial::WriteToUSART(rsp, len);
}

/// Performs a blocking (!) LED "snake" from one side to the other and back.
/// Intended only for visualization of EEPROM reset, not to be used in while the MMU is running normally
namespace EEPROMResetVis {

static void Delay() {
    uint32_t start = mt::timebase.Millis();
    while (!mt::timebase.Elapsed(start, 50)) {
        ml::leds.Step();
    }
}

static void __attribute__((noinline)) Green(uint8_t i) {
    ml::leds.SetPairButOffOthers(i, ml::on, ml::off);
    Delay();
}

static void __attribute__((noinline)) Red(uint8_t i) {
    ml::leds.SetPairButOffOthers(i, ml::off, ml::on);
    Delay();
}

static void Run() {
    for (uint8_t i = 0; i < ml::leds.LedPairsCount(); ++i) {
        Red(i);
        Green(i);
    }
    for (uint8_t i = ml::leds.LedPairsCount(); i != 0; --i) {
        Green(i);
        Red(i);
    }
}

} // namespace EEPROMResetVis

void Application::ProcessReset(ResetTypes resetType) {
    switch (resetType) {
    case ResetTypes::EEPROMAndSoftware: // perform an EEPROM reset if the resetType == The Answer to the Ultimate Question of Life, the Universe, and Everything :)
        mps::EraseAll();
        EEPROMResetVis::Run();
        [[fallthrough]];
    default: // anything else "just" restarts the board
        hal::cpu::Reset();
    }
}

void Application::ProcessRequestMsg(const mp::RequestMsg &rq) {
    switch (rq.code) {
    case mp::RequestMsgCodes::Button:
        // behave just like if the user pressed a button
        mui::userInput.ProcessMessage(rq.value);
        ReportCommandAccepted(rq, mp::ResponseMsgParamCodes::Accepted);
        break;
    case mp::RequestMsgCodes::Finda:
        // immediately report FINDA status
        ReportFINDA(rq);
        break;
    case mp::RequestMsgCodes::Query:
        // immediately report progress of currently running command
        ReportRunningCommand();
        break;
    case mp::RequestMsgCodes::Reset:
        // immediately reset the board - there is no response in this case
        ProcessReset((ResetTypes)rq.value);
        break;
    case mp::RequestMsgCodes::Version:
    case mp::RequestMsgCodes::Read:
        ReportReadRegister(rq);
        break;
    case mp::RequestMsgCodes::Write:
        ReportWriteRegister(rq);
        break;
    case mp::RequestMsgCodes::Cut:
    case mp::RequestMsgCodes::Eject:
    case mp::RequestMsgCodes::Home:
    case mp::RequestMsgCodes::Load:
    case mp::RequestMsgCodes::Tool:
    case mp::RequestMsgCodes::Unload:
        PlanCommand(rq);
        break;
    case mp::RequestMsgCodes::FilamentSensor: // set filament sensor state in the printer
        mfs::fsensor.ProcessMessage(rq.value != 0);
        ReportCommandAccepted(rq, mp::ResponseMsgParamCodes::Accepted);
        break;
    default:
        // respond with an error message
        break;
    }
}

bool Application::CheckMsgs() {
    using mpd = mp::DecodeStatus;
    while (modules::serial::Available()) {
        switch (protocol.DecodeRequest(modules::serial::ConsumeByte())) {
        case mpd::MessageCompleted:
            // process the input message
            return true;
            break;
        case mpd::NeedMoreData:
            // just continue reading
            break;
        case mpd::Error:
            // @@TODO what shall we do? Start some watchdog? We cannot send anything spontaneously
            break;
        }
    }
    return false;
}

void Application::Panic(ErrorCode ec) {
    currentCommand->Panic(ec);
}

void Application::Step() {
    CheckManualOperation();

    if (CheckMsgs()) {
        ProcessRequestMsg(protocol.GetRequestMsg());
    }

    currentCommand->Step();
}

uint8_t Application::CurrentProgressCode() {
    return (uint8_t)currentCommand->State();
}

uint16_t Application::CurrentErrorCode() {
    return (uint16_t)currentCommand->Error();
}
