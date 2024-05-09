#include "test_execution.h"

#include <catch2/catch.hpp>

using namespace automata;
using std::get;

namespace {

/*
 * A testing automaton.
 *
 * Accepts:
 *
 * 0 -> Hello -> 1
 *   -> (X|x|Z)+ -> Y ->
 *   -> .* : ->
 *
 *
 * Unlike most other automata out there, this one is written by hand instead of
 * being generated. Beware the dragons and indices.
 *
 * Also note that the string label is used _wrong_ in these tests and
 * implicitly accepts things like HeXY! The generator will not do such things
 * in here.
 */

const StrPath paths[] = {
    { "hello" },
};

const Transition transitions[] = {
    /* Target state, type, label */
    // From 0
    { 1, LabelType::Path, 0, false },
    { 2, LabelType::CharNoCase, 'x', false },
    { 2, LabelType::Char, 'Z', false },
    { 3, LabelType::Char, ':', false },
    { 4, LabelType::Special, SpecialLabel::All, false },
    // From 1
    // (nothing, terminal)
    // From 2
    { 2, LabelType::CharNoCase, 'x', false },
    { 2, LabelType::Char, 'Z', false },
    { 1, LabelType::Char, 'Y', false },
    // From 3
    // (nothing, terminal)
    // From 4
    { 3, LabelType::Char, ':', false },
    { 4, LabelType::Special, SpecialLabel::All, false },
};

const State states[] = {
    /* trans IDX, enter, leave, path IDX, has path, path nocase */
    /* 0 */ { 0, false, false }, // Start
    /* 1 */ { 5, true, false }, // After Hello (implicit + 1 for path); terminal (also for others)
    /* 2 */ { 5, false, false }, // After some X|x|Z
    /* 3 */ { 8, true, false }, // The accepting ':', terminal
    /* 4 */ { 8, false, false }, // The .*
    /* -- */ { 10, false, false }, // Sentinel state to terminate previous transitions
};

enum NamedStates {
    // Terminals
    StringyAccept = 1,
    ColonAccept = 3,

    // The previous states where it went from
    Start = 0,
    Letters = 2,
    Fallback = 4,
};

const Automaton test_automaton(paths, transitions, states);

} // namespace

TEST_CASE("Raw Hello") {
    TestExecution ex(test_automaton);

    REQUIRE(get<0>(ex.consume("Hell")) == ExecutionControl::Continue);
    REQUIRE(ex.events.empty());
    REQUIRE(ex.feed('o') == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    const auto &ev = ex.events[0];
    REQUIRE(ev.leaving_state == NamedStates::Start);
    REQUIRE(ev.entering_state == NamedStates::StringyAccept);
    REQUIRE_FALSE(ev.triggered_by_leave);
    REQUIRE(ev.triggered_by_enter);
    REQUIRE(ev.payload == 'o');

    // Won't move further
    REQUIRE(ex.feed('o') == ExecutionControl::NoTransition);
    REQUIRE(ex.events.size() == 1);
}

TEST_CASE("Xs and Zs") {
    TestExecution ex(test_automaton);

    REQUIRE(get<0>(ex.consume("XXxXXXZZxxXX")) == ExecutionControl::Continue);
    REQUIRE(ex.events.empty());

    // Accepts only the first Y
    REQUIRE(get<0>(ex.consume("YY")) == ExecutionControl::NoTransition);
    REQUIRE(ex.events.size() == 1);
    const auto &ev = ex.events[0];
    REQUIRE(ev.leaving_state == NamedStates::Letters);
    REQUIRE(ev.entering_state == NamedStates::StringyAccept);
    REQUIRE_FALSE(ev.triggered_by_leave);
    REQUIRE(ev.triggered_by_enter);
    REQUIRE(ev.payload == 'Y');
}

TEST_CASE("Case & Nocase") {
    TestExecution ex(test_automaton);

    REQUIRE(ex.feed('x') == ExecutionControl::Continue);
    REQUIRE(ex.feed('X') == ExecutionControl::Continue);
    REQUIRE(ex.feed('Z') == ExecutionControl::Continue);
    REQUIRE(ex.feed('z') == ExecutionControl::NoTransition);
    REQUIRE(ex.events.empty());

    // It can "recover" from a "wrong" letter
    REQUIRE(ex.feed('Y') == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    const auto &ev = ex.events[0];
    REQUIRE(ev.leaving_state == NamedStates::Letters);
    REQUIRE(ev.entering_state == NamedStates::StringyAccept);
    REQUIRE_FALSE(ev.triggered_by_leave);
    REQUIRE(ev.triggered_by_enter);
    REQUIRE(ev.payload == 'Y');
}

TEST_CASE("Fallbacks and collons") {
    TestExecution ex(test_automaton);

    REQUIRE(get<0>(ex.consume("ABCDEFgh   sht")) == ExecutionControl::Continue);
    REQUIRE(ex.events.empty());

    REQUIRE(ex.feed(':') == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    const auto &ev = ex.events[0];
    REQUIRE(ev.leaving_state == NamedStates::Fallback);
    REQUIRE(ev.entering_state == NamedStates::ColonAccept);
    REQUIRE_FALSE(ev.triggered_by_leave);
    REQUIRE(ev.triggered_by_enter);
    REQUIRE(ev.payload == ':');
}
