/**
 * @file selftest_part.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "i_selftest_part.hpp"
#include "selftest_part.hpp"
#include "i_selftest.hpp"
#include "selftest_log.hpp"

using namespace selftest;

PhasesSelftest IPartHandler::fsm_phase_index = PhasesSelftest::_none;

IPartHandler::IPartHandler(size_t sz, SelftestParts part)
    : current_state(IndexIdle())
    , current_state_enter_time(SelftestInstance().GetTime())
    , state_count(sz)
    , loop_mark(0) {
    fsm_phase_index = SelftestGetFirstPhaseFromPart(part);
}

bool IPartHandler::Loop() {
    //exit idle state
    if (current_state == -1)
        current_state = 0; // TODO i might want to use Start() somewhere in code instead

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

    switch (invokeCurrentState()) {
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
    case LoopResult::MarkLoop:
        loop_mark = current_state;
        next();
        return true;
    case LoopResult::GoToMark:
        changeCurrentState(loop_mark);
        return true;
    }

    //we should never get here
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

TestResult_t IPartHandler::GetResult() const {
    //cannot use switch, cases would be evaluated at runtime
    if (current_state == IndexAborted())
        return TestResult_t::Skipped;
    if (current_state == IndexFinished())
        return TestResult_t::Passed;
    if (current_state == IndexFailed())
        return TestResult_t::Failed;
    return TestResult_t::Unknown;
}

void IPartHandler::next() {
    if (!isInProgress())
        return;
    changeCurrentState(current_state + 1); // state[count] == Passed
    if (current_state == IndexFinished()) {
        Pass();
    }
}

void IPartHandler::Pass() {
    LogDebug("IPartHandler::Pass");
    current_state = IndexFinished();
    current_state_enter_time = SelftestInstance().GetTime();
    pass();
}

void IPartHandler::Fail() {
    LogDebug("IPartHandler::Fail");
    if (current_state != IndexFailed()) {
        current_state = IndexFailed();
        current_state_enter_time = SelftestInstance().GetTime();
        fail();
    }
}

void IPartHandler::Abort() {
    LogDebug("IPartHandler::Abort");
    current_state = IndexAborted();
    abort();
}

bool IPartHandler::WaitSoLastStateIsVisible() const {
    return (SelftestInstance().GetTime() - current_state_enter_time) >= time_to_show_result;
}
