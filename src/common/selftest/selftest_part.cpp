/**
 * @file selftest_part.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "i_selftest_part.hpp"
#include "selftest_loop_result.hpp"
#include "selftest_part.hpp"
#include "i_selftest.hpp"
#include "selftest_log.hpp"
#include "utility_extensions.hpp"

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

PhasesSelftest IPartHandler::fsm_phase_index = PhasesSelftest::_none;

IPartHandler::IPartHandler(size_t sz, SelftestParts part)
    : current_state(IndexIdle())
    , current_state_enter_time(SelftestInstance().GetTime())
    , state_count(sz) {
    fsm_phase_index = SelftestGetFirstPhaseFromPart(part);
}

bool IPartHandler::Loop() {
    // exit idle state
    if (current_state == -1) {
        current_state = 0; // TODO i might want to use Start() somewhere in code instead
    }

    if (current_state < 0 || current_state >= state_count) {
        // wait a bit so result is visible
        if (current_state == IndexFailed() || current_state == IndexFinished()) {
            return !WaitSoLastStateIsVisible();
        }
        return false;
    }

    button_pressed = processButton();
    switch (button_pressed) {
    case Response::Abort:
        Abort();
        return false; // exit instantly
    default:
        break;
    }

    LoopResult current_loop_result = invokeCurrentState();
    if (defer_fail && !is_now_failing_result(current_loop_result)) {
        current_loop_result = LoopResult::Fail;
        defer_fail = false;
    }
    switch (current_loop_result) {
    case LoopResult::Abort:
        Abort();
        return false; // exit instantly
    case LoopResult::Fail:
        Fail();
        return true;
    case LoopResult::RunCurrent:
        hook_remained_in_state();
        return true;
    case LoopResult::RunNext:
        next(); // it will call Pass(), when switched to finished
        return true;
    case LoopResult::RunNextAndFailAfter:
        if (current_state + 1 == IndexFinished()) {
            bsod("No next state to fail after");
        }
        defer_fail = true;
        next();
        return true;
    default: {
        auto loop_mark = ftrstd::to_underlying(current_loop_result);

        // LoopResult::MarkLoop0 also serves as flag, see enum definition
        if (loop_mark & ftrstd::to_underlying(LoopResult::MarkLoop0)) {
            loop_mark &= ~ftrstd::to_underlying(LoopResult::MarkLoop0);
            if (loop_mark >= LoopMarkCount) {
                bsod("MarkLoop out of range");
            }
            loop_marks[loop_mark] = current_state;
            next();
            return true;
        }

        // LoopResult::GoToMark0 also serves as flag, see enum definition
        if (loop_mark & ftrstd::to_underlying(LoopResult::GoToMark0)) {
            loop_mark &= ~ftrstd::to_underlying(LoopResult::GoToMark0);
            if (loop_mark >= LoopMarkCount) {
                bsod("GoToMark out of range");
            }
            changeCurrentState(loop_marks[loop_mark]);
            next();
            return true;
        }

        bsod("Undefined LoopResult");
    }
    }

    // we should never get here
    return false;
}

void IPartHandler::changeCurrentState(int new_state) {
    if (current_state != new_state) {
        current_state = new_state;
        current_state_enter_time = SelftestInstance().GetTime();
        hook_state_changed();
    } else {
        hook_remained_in_state();
    }
}
uint32_t IPartHandler::IsInState_ms() {
    return SelftestInstance().GetTime() - current_state_enter_time;
}

bool IPartHandler::isInProgress() const {
    return ((current_state >= 0) && (current_state < state_count));
}

TestResult IPartHandler::GetResult() const {
    // cannot use switch, cases would be evaluated at runtime
    if (current_state == IndexAborted()) {
        return TestResult_Skipped;
    }
    if (current_state == IndexFinished()) {
        return TestResult_Passed;
    }
    if (current_state == IndexFailed()) {
        return TestResult_Failed;
    }
    return TestResult_Unknown;
}

void IPartHandler::next() {
    if (!isInProgress()) {
        return;
    }
    changeCurrentState(current_state + 1); // state[count] == Passed
    if (current_state == IndexFinished()) {
        Pass();
    }
}

void IPartHandler::Pass() {
    log_debug(Selftest, "IPartHandler::Pass");
    current_state = IndexFinished();
    current_state_enter_time = SelftestInstance().GetTime();
    pass();
}

void IPartHandler::Fail() {
    log_debug(Selftest, "IPartHandler::Fail");
    if (current_state != IndexFailed()) {
        current_state = IndexFailed();
        current_state_enter_time = SelftestInstance().GetTime();
        fail();
    }
}

void IPartHandler::Abort() {
    log_debug(Selftest, "IPartHandler::Abort");
    current_state = IndexAborted();
    abort();

    // Terminate all moves (the hard way)
    marlin_server::quick_stop();

    // Wait till all commands are processed before calling Abort
    // Abort might be restoring phase stepping config, which would trigger some asserts if quick_stop is not processed
    while (planner.processing()) {
        idle(true, true);
    }
}

bool IPartHandler::WaitSoLastStateIsVisible() const {
    return (SelftestInstance().GetTime() - current_state_enter_time) >= time_to_show_result;
}
