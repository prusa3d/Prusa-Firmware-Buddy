/**
 * @file i_selftest_part.hpp
 * @author Radek Vana
 * @brief abstract parent of all selftest parts (like fans, axis ...)
 * @date 2021-10-14
 */
#pragma once
#include "selftest_result_type.hpp"
#include <array>
#include "selftest_sub_state.hpp"
#include "selftest_loop_result.hpp"
#include "client_response.hpp"
#include <cstdint>

namespace selftest {

class IPartHandler {
public:
    using progress_fnc = float (*)();

    IPartHandler(size_t sz, SelftestParts part);
    virtual ~IPartHandler() = default;

    static constexpr int IndexIdle() { return -1; }
    inline int IndexFinished() const { return state_count; }
    inline int IndexAborted() const { return state_count + 1; }
    inline int IndexFailed() const { return state_count + 2; }

    bool Loop();
    TestResult GetResult() const; // used to write status to eeprom

    static PhasesSelftest GetFsmPhase() { return fsm_phase_index; }
    static void SetFsmPhase(PhasesSelftest phase) { fsm_phase_index = phase; }

    void Pass();
    void Fail();
    void Abort();

    uint32_t IsInState_ms();
    Response GetButtonPressed() const { return button_pressed; }
    bool WaitSoLastStateIsVisible() const; // cannot apply to all states, only those which change GUI state
    void SetTimeToShowResult(uint32_t ms) { time_to_show_result = ms; }

protected:
    bool isInProgress() const; // do not make this method public, it does not wait for state to be shown, use Loop result instead
    virtual LoopResult invokeCurrentState() = 0;
    virtual Response processButton() = 0;
    void next();
    virtual void pass() = 0;
    virtual void fail() = 0;
    virtual void abort() = 0;
    int currentState() { return current_state; }

    virtual void hook_state_changed() = 0;
    virtual void hook_remained_in_state() = 0;

private:
    void changeCurrentState(int new_state);
    int current_state;
    uint32_t current_state_enter_time;
    int state_count; // did not use size_t to be able to compare with int
    int loop_marks[LoopMarkCount] = {}; // used in cyclic states
    Response button_pressed;
    uint32_t time_to_show_result = 2048; // ms

    // multiple selftests can run at the same time,
    // if so they must be compatible to run together (use same fsm)
    static PhasesSelftest fsm_phase_index;
};

/**
 * @brief Helper class to return from selftest instead of bool.
 * Backwards compatible with bool.
 * This can be used if a test part is aborted, but was successful before. In this case
 * the result in eeprom is success, but user aborted and doesn't want to continue with next tests.
 */
class TestReturn {
    bool in_progress; ///< true if selftest is still in progress
    bool skipped; ///< true if this test was skipped

public:
    TestReturn(bool in_progress_, bool skipped_)
        : in_progress(in_progress_)
        , skipped(skipped_) {}

    TestReturn(bool in_progress_)
        : in_progress(in_progress_)
        , skipped(false) {}

    bool StillInProgress() const { return in_progress; }
    operator bool() const { return StillInProgress(); }
    bool WasSkipped() const { return skipped; }
};

}; // namespace selftest
