#include "selftest_revise_printer_setup.hpp"

#include <selftest_part.hpp>

using namespace selftest;

namespace {

struct Result {
    static void Pass() {}
    static void Fail() {}
    static void Abort() {}
};

// Some useless selftest classes
struct Config {
    using type_evaluation = Result;
    static constexpr SelftestParts part_type = SelftestParts::RevisePrinterSetup;
};

class Part {
    friend class Factory;

public:
    static auto create() {
        return Factory::CreateDynamical<Part>(
            Config(),
            *reinterpret_cast<Result *>(0), // Whatever, we just need to pass a reference due to the selftest machinery design
            &Part::phase_ask_revise,
            &Part::phase_revise,
            &Part::phase_ask_retry);
    }

    Part(IPartHandler &handler, const Config &, Result &)
        : handler_(handler) {
    }

private:
    LoopResult phase_ask_revise() {
        IPartHandler::SetFsmPhase(PhasesSelftest::RevisePrinterStatus_ask_revise);

        switch (handler_.GetButtonPressed()) {

        case Response::Adjust:
            return LoopResult::RunNext;

        case Response::Skip:
            return LoopResult::Abort;

        default:
            return LoopResult::RunCurrent;
        }
    }

    LoopResult phase_revise() {
        IPartHandler::SetFsmPhase(PhasesSelftest::RevisePrinterStatus_revise);

        switch (handler_.GetButtonPressed()) {

        case Response::Done:
            return LoopResult::RunNext;

        default:
            return LoopResult::RunCurrent;
        }
    }

    LoopResult phase_ask_retry() {
        IPartHandler::SetFsmPhase(PhasesSelftest::RevisePrinterStatus_ask_retry);

        switch (handler_.GetButtonPressed()) {

        case Response::Yes:
            return LoopResult::RunNext;

        case Response::No:
            return LoopResult::Abort;

        default:
            return LoopResult::RunCurrent;
        }
    }

private:
    IPartHandler &handler_;
};

decltype(Part::create()) machine = nullptr;

} // namespace

RevisePrinterSetupResult selftest::phase_revise_printer_setup() {
    if (!machine) {
        machine = Part::create();
    }

    const bool in_progress = machine->Loop();
    marlin_server::fsm_change(IPartHandler::GetFsmPhase(), {});

    if (in_progress) {
        return RevisePrinterSetupResult::running;
    }

    const auto result = (machine->GetResult() == TestResult_Skipped) ? RevisePrinterSetupResult::do_not_retry : RevisePrinterSetupResult::retry;
    delete machine;
    machine = nullptr;
    return result;
}
