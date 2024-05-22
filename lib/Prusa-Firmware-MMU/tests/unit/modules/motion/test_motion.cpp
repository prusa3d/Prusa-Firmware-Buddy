#include "catch2/catch_test_macros.hpp"
#include "motion.h"

using namespace modules::motion;

namespace hal {
namespace shr16 {
extern uint8_t shr16_tmc_ena;
} // namespace shr16
} // namespace hal

// Conveniently read the enable state set into the lower-level shift register
bool getTMCEnabled(const MotorParams &mp) {
    return (hal::shr16::shr16_tmc_ena & (1 << mp.idx));
}

// Perform Step() until all moves are completed, returning the number of steps performed.
// Ensure the move doesn't run forever, making the test fail reliably.
ssize_t stepUntilDone(size_t maxSteps = 100000) {
    for (size_t i = 0; i != maxSteps; ++i)
        if (!motion.Step())
            return i;

    // number of steps exceeded
    return -1;
}

void ResetMotionSim() {
    REQUIRE(motion.QueueEmpty());

    motion.SetPosition(Idler, 0);
    REQUIRE(motion.Position(Idler) == 0);

    motion.SetPosition(Selector, 0);
    REQUIRE(motion.Position(Selector) == 0);

    motion.SetPosition(Pulley, 0);
    REQUIRE(motion.Position(Pulley) == 0);
}

TEST_CASE("motion::basic", "[motion]") {
    // initial state
    ResetMotionSim();

    // enqueue a single move
    motion.PlanMoveTo(Idler, 10, 1);
    REQUIRE(!motion.QueueEmpty());

    // perform the move
    REQUIRE(stepUntilDone() == 10);
    REQUIRE(motion.QueueEmpty());

    // check positions
    REQUIRE(motion.Position(Idler) == 10);
}

TEST_CASE("motion::auto_axis_enable", "[motion]") {
    // TODO: the low-level test performed by getTMCEnabled should be inside a dedicated
    // TMC2130 test, not within regular motion tests (although doesn't harm).

    // by default the axis should start disabled
    REQUIRE(motion.Enabled(Pulley) == false);
    REQUIRE(motion.DriverForAxis(Pulley).Enabled() == false);
    REQUIRE(getTMCEnabled(axisParams[Pulley].params) == false);

    // enable manually the axis
    motion.SetEnabled(Pulley, true);
    REQUIRE(motion.Enabled(Pulley) == true);
    REQUIRE(motion.DriverForAxis(Pulley).Enabled() == true);
    REQUIRE(getTMCEnabled(axisParams[Pulley].params) == true);

    // now disable
    motion.SetEnabled(Pulley, false);
    REQUIRE(motion.Enabled(Pulley) == false);
    REQUIRE(motion.DriverForAxis(Pulley).Enabled() == false);
    REQUIRE(getTMCEnabled(axisParams[Pulley].params) == false);

    // planning a move should enable the axis automatically
    REQUIRE(motion.QueueEmpty());
    motion.PlanMove<Pulley>(1.0_mm, 100.0_mm_s);
    REQUIRE(motion.Enabled(Pulley) == true);
    REQUIRE(motion.DriverForAxis(Pulley).Enabled() == true);
    REQUIRE(getTMCEnabled(axisParams[Pulley].params) == true);
}

TEST_CASE("motion::unit", "[motion]") {
    // test AxisUnit conversion in the PlanMove and PlanMoveTo.
    // Use Selector explicitly, as it has an exact unit/step conversion.
    ResetMotionSim();

    // move with AxisUnit
    pos_t target = config::selector.stepsPerUnit * 10;
    motion.PlanMoveTo<Selector>(10.0_S_mm, 100.0_S_mm_s);
    CHECK(stepUntilDone() != -1);
    REQUIRE(motion.Position(Selector) == target);
    S_pos_t s_target = motion.Position<Selector>();
    REQUIRE(s_target.v == target);

    // move directly with physical units
    motion.PlanMoveTo<Selector>(10.0_mm, 100.0_mm_s);
    REQUIRE(stepUntilDone() == 0);
    REQUIRE(motion.Position(Selector) == target);

    // relative move with AxisUnit
    motion.PlanMove<Selector>(-5.0_S_mm, 100.0_S_mm_s);
    CHECK(stepUntilDone() != -1);
    REQUIRE(motion.Position(Selector) == target / 2);
    s_target = motion.Position<Selector>();
    REQUIRE(s_target.v == target / 2);

    // relative move with physical unit
    motion.PlanMove<Selector>(-5.0_mm, 100.0_mm_s);
    CHECK(stepUntilDone() != -1);
    REQUIRE(motion.Position(Selector) == 0);

    // now test remaining axes
    target = config::pulley.stepsPerUnit * 10;
    motion.PlanMoveTo<Pulley>(10.0_P_mm, 100.0_P_mm_s);
    motion.PlanMove<Pulley>(10.0_mm, 100.0_mm_s);
    CHECK(stepUntilDone() != -1);
    REQUIRE(abs(motion.Position(Pulley) - target * 2) <= 1);

    target = config::idler.stepsPerUnit * 10;
    motion.PlanMoveTo<Idler>(10.0_I_deg, 100.0_I_deg_s);
    motion.PlanMove<Idler>(10.0_deg, 100.0_deg_s);
    CHECK(stepUntilDone() != -1);
    REQUIRE(abs(motion.Position(Idler) - target * 2) <= 1);
}

TEST_CASE("motion::dual_move_fwd", "[motion]") {
    // enqueue moves on two axes
    ResetMotionSim();

    // ensure the same jerk is set on both
    motion.SetJerk(Idler, motion.Jerk(Selector));
    REQUIRE(motion.Jerk(Idler) == motion.Jerk(Selector));

    // ensure the same acceleration is set on both
    motion.SetAcceleration(Idler, motion.Acceleration(Selector));
    REQUIRE(motion.Acceleration(Idler) == motion.Acceleration(Selector));

    // plan two moves at the same speed and acceleration
    motion.PlanMoveTo(Idler, 10, 1);
    motion.PlanMoveTo(Selector, 10, 1);

    // perform the move, which should be perfectly merged
    REQUIRE(stepUntilDone() == 10);
    REQUIRE(motion.QueueEmpty());

    // check for final axis positions
    REQUIRE(motion.Position(Idler) == 10);
    REQUIRE(motion.Position(Selector) == 10);
}

TEST_CASE("motion::dual_move_inv", "[motion]") {
    // enqueue moves on two axes
    ResetMotionSim();

    // ensure the same jerk is set on both
    motion.SetJerk(Idler, motion.Jerk(Selector));
    REQUIRE(motion.Jerk(Idler) == motion.Jerk(Selector));

    // ensure the same acceleration is set on both
    motion.SetAcceleration(Idler, motion.Acceleration(Selector));
    REQUIRE(motion.Acceleration(Idler) == motion.Acceleration(Selector));

    // set two different starting points
    motion.SetPosition(Idler, 0);
    motion.SetPosition(Selector, 5);

    // plan two moves at the same speed and acceleration: like in the previous
    // test this should *also* reduce to the same steps being performed
    motion.PlanMove(Idler, 10, 1);
    motion.PlanMove(Selector, -10, 1);

    // perform the move, which should be perfectly merged
    REQUIRE(stepUntilDone() == 10);
    REQUIRE(motion.QueueEmpty());

    // check for final axis positions
    REQUIRE(motion.Position(Idler) == 10);
    REQUIRE(motion.Position(Selector) == -5);
}

TEST_CASE("motion::dual_move_complex", "[motion]") {
    // enqueue two completely different moves on two axes
    ResetMotionSim();

    // set custom acceleration values
    motion.SetAcceleration(Idler, 10);
    motion.SetAcceleration(Selector, 20);

    // plan two moves with difference accelerations
    motion.PlanMoveTo(Idler, 10, 1);
    motion.PlanMoveTo(Selector, 10, 1);

    // perform the move, which should take less iterations than the sum of both
    REQUIRE(stepUntilDone(20));
    REQUIRE(motion.QueueEmpty());

    // check for final axis positions
    REQUIRE(motion.Position(Idler) == 10);
    REQUIRE(motion.Position(Selector) == 10);
}

TEST_CASE("motion::triple_move", "[motion]") {
    ResetMotionSim();

    constexpr pos_t i = 10;
    constexpr pos_t s = 20;
    constexpr pos_t p = 30;

    // check that we can move three axes at the same time
    motion.PlanMoveTo(Idler, i, 1);
    motion.PlanMoveTo(Selector, s, 1);
    motion.PlanMoveTo(Pulley, p, 1);

    // perform the move with a maximum step limit
    REQUIRE(stepUntilDone(i + s + p) != -1);

    // check queue status
    REQUIRE(motion.QueueEmpty());

    // check for final axis positions
    REQUIRE(motion.Position(Idler) == i);
    REQUIRE(motion.Position(Selector) == s);
    REQUIRE(motion.Position(Pulley) == p);
}

TEST_CASE("motion::queue_abort", "[motion]") {
    // queue should start empty
    ResetMotionSim();

    // enqueue two moves
    motion.PlanMoveTo(Pulley, 10, 1);
    motion.PlanMoveTo(Idler, 10, 1);
    REQUIRE(!motion.QueueEmpty(Pulley));
    REQUIRE(!motion.QueueEmpty(Idler));
    REQUIRE(motion.QueueEmpty(Selector));
    REQUIRE(!motion.QueueEmpty());

    // step ~1/3 way through
    REQUIRE(stepUntilDone(3) == -1);

    // abort the whole queue
    motion.AbortPlannedMoves();
    REQUIRE(motion.QueueEmpty(Pulley));
    REQUIRE(motion.QueueEmpty(Idler));
    REQUIRE(motion.QueueEmpty());
}

TEST_CASE("motion::queue_abort_1", "[motion]") {
    // queue should start empty
    ResetMotionSim();

    // enqueue two moves
    motion.PlanMoveTo(Pulley, 10, 1);
    motion.PlanMoveTo(Idler, 10, 1);
    REQUIRE(!motion.QueueEmpty(Pulley));
    REQUIRE(!motion.QueueEmpty(Idler));
    REQUIRE(motion.QueueEmpty(Selector));
    REQUIRE(!motion.QueueEmpty());

    // step ~1/3 way through
    REQUIRE(stepUntilDone(3) == -1);

    // abort one axis only
    motion.AbortPlannedMoves(Pulley);
    REQUIRE(motion.QueueEmpty(Pulley));
    REQUIRE(!motion.QueueEmpty(Idler));
    REQUIRE(!motion.QueueEmpty());
}

TEST_CASE("motion::queue_abort_2", "[motion]") {
    // queue should start empty
    ResetMotionSim();

    // enqueue two moves on a single axis
    motion.PlanMoveTo(Pulley, 10, 1);
    motion.PlanMoveTo(Pulley, 20, 1);
    REQUIRE(!motion.QueueEmpty(Pulley));

    // abort before scheduling and check that both are gone
    motion.AbortPlannedMoves(Pulley);
    REQUIRE(motion.QueueEmpty(Pulley));
}

TEST_CASE("motion::queue_abort_3", "[motion]") {
    // queue should start empty
    ResetMotionSim();

    // enqueue two moves on a single axis
    motion.PlanMoveTo(Pulley, 10, 1);
    motion.PlanMoveTo(Pulley, 20, 1);
    REQUIRE(!motion.QueueEmpty(Pulley));

    // step ~1/3 way through of the first move
    REQUIRE(stepUntilDone(3) == -1);

    // abort the partial and unscheduled move
    motion.AbortPlannedMoves(Pulley);
    REQUIRE(motion.QueueEmpty(Pulley));
}

TEST_CASE("motion::queue_abort_4", "[motion]") {
    // queue should start empty
    ResetMotionSim();

    // enqueue a move on a single axis
    motion.PlanMoveTo(Pulley, 10, 1);
    REQUIRE(!motion.QueueEmpty(Pulley));

    // abort the single unscheduled move
    motion.AbortPlannedMoves(Pulley);
    REQUIRE(motion.QueueEmpty(Pulley));
}

TEST_CASE("motion::long_pulley_move", "[motion]") {
    ResetMotionSim();
    constexpr auto mm400 = 400._mm;
    constexpr pos_t p = unitToSteps<P_pos_t>(mm400);
    motion.PlanMoveTo<Pulley>(mm400, 1._mm_s);
    REQUIRE(stepUntilDone() == p);
}

TEST_CASE("motion::pos_overflow", "[motion]") {
    ResetMotionSim();

    // set a position _at_ the overflow limit
    mm::pos_t pos_max = std::numeric_limits<mm::pos_t>::max();
    mm::motion.SetPosition(mm::Pulley, pos_max);
    REQUIRE(mm::motion.Position(mm::Pulley) == pos_max);

    // plan a move that will overflow
    mm::pos_t steps = 10;
    mm::motion.PlanMove(mm::Pulley, steps, 1);

    // ensure we did overflow
    REQUIRE(mm::motion.Position(mm::Pulley) < pos_max);

    // step once to setup current_block
    mm::motion.Step();

    // ensure the move direction and step count is correct despite the overflow - abuse
    // CurBlockShift to get both, accounting for the useless single step we performed just above
    REQUIRE(mm::motion.CtrlForAxis(mm::Pulley).CurBlockShift() == steps - 1);
}

TEST_CASE("motion::pos_underflow", "[motion]") {
    ResetMotionSim();

    mm::pos_t pos_min = std::numeric_limits<mm::pos_t>::min();
    mm::motion.SetPosition(mm::Pulley, pos_min);
    REQUIRE(mm::motion.Position(mm::Pulley) == pos_min);

    mm::pos_t steps = 10;
    mm::motion.PlanMove(mm::Pulley, -steps, 1);
    REQUIRE(mm::motion.Position(mm::Pulley) > pos_min);

    mm::motion.Step();

    REQUIRE(mm::motion.CtrlForAxis(mm::Pulley).CurBlockShift() == -(steps - 1));
}
