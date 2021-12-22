#include <until_comma.h>
#include "test_execution.h"

#include <catch2/catch.hpp>

using namespace automata;

namespace {

/*
 * An automaton that accepts everything until a comma.
 */
const Automaton until_comma(test::until_comma::paths, test::until_comma::transitions, test::until_comma::states);

}

TEST_CASE("Fallbacks and collons") {
    TestExecution ex(until_comma);

    REQUIRE(ex.feed("Hello world") == ExecutionControl::Continue);
    REQUIRE(ex.events.empty());
    REQUIRE(ex.feed("xp,") == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    REQUIRE(ex.events[0].leaving_state == 0);
    REQUIRE(ex.events[0].entering_state == test::until_comma::Names::Comma);
    REQUIRE(ex.events[0].payload == ',');
}
