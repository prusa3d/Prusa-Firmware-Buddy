#include <nhttp/job_command.h>
#include <nhttp/handler.h>

#include <catch2/catch.hpp>

using nhttp::handler::ConnectionState;
using nhttp::handler::Continue;
using nhttp::handler::StatusPage;
using nhttp::handler::Step;
using nhttp::printer::JobCommand;
using std::get_if;
using std::optional;
using std::string_view;

namespace {

enum class Cmd {
    Pause,
    Resume,
    Toggle,
    Stop,
};

std::vector<Cmd> history;

void do_test(string_view data, optional<Cmd> expected_command) {
    history.clear();

    JobCommand command(data.size(), true, false);
    Step result = { 0, 0, Continue() };
    command.step(data, false, nullptr, 0, result);

    REQUIRE(result.written == 0);
    REQUIRE(result.read == data.size());
    auto *state = get_if<ConnectionState>(&result.next);
    REQUIRE(state != nullptr);

    auto *page = get_if<StatusPage>(state);
    REQUIRE(page != nullptr);
    // Too lazy to take that page apart and check the state...
    if (expected_command.has_value()) {
        REQUIRE(history.size() == 1);
        REQUIRE(history[0] == *expected_command);
    } else {
        REQUIRE(history.empty());
    }
}

} // namespace

namespace nhttp::printer {

/*
 * In the real application, these call into marlin and that would be
 * pain to deal with in tests. So we have a separate implementation of
 * them here.
 */

bool JobCommand::resume() {
    history.push_back(Cmd::Resume);
    return true;
}

bool JobCommand::pause() {
    history.push_back(Cmd::Pause);
    return true;
}

bool JobCommand::pause_toggle() {
    history.push_back(Cmd::Toggle);
    return true;
}

bool JobCommand::stop() {
    history.push_back(Cmd::Stop);
    return true;
}

} // namespace nhttp::printer

TEST_CASE("Job abort") {
    do_test("{\"command\": \"cancel\"}", Cmd::Stop);
}

TEST_CASE("Job pause") {
    SECTION("In order") {
        do_test("{\"command\": \"pause\", \"action\": \"pause\"}", Cmd::Pause);
    }

    SECTION("Out of order") {
        do_test("{\"action\": \"pause\", \"command\": \"pause\"}", Cmd::Pause);
    }
}

TEST_CASE("Job resume") {
    SECTION("In order") {
        do_test("{\"command\": \"pause\", \"action\": \"resume\"}", Cmd::Resume);
    }

    SECTION("Out of order") {
        do_test("{\"action\": \"resume\", \"command\": \"pause\"}", Cmd::Resume);
    }
}

TEST_CASE("Extra stuff") {
    SECTION("Before") {
        do_test("{\"extra\": 42, \"command\": \"cancel\"}", Cmd::Stop);
    }

    SECTION("After") {
        do_test("{\"command\": \"cancel\", \"extra\": 42}", Cmd::Stop);
    }

    SECTION("Array") {
        SECTION("After") {
            do_test("{\"command\": \"cancel\", \"extra\": [42, 15]}", Cmd::Stop);
        }

        SECTION("Before") {
            do_test("{\"extra\": [42, 15], \"command\": \"cancel\"}", Cmd::Stop);
        }
    }

    SECTION("Object") {
        SECTION("After") {
            do_test("{\"command\": \"cancel\", \"extra\": {\"a\": true}}", Cmd::Stop);
        }

        SECTION("Before") {
            do_test("{\"extra\": {\"b\": false}, \"command\": \"cancel\"}", Cmd::Stop);
        }
    }
}
