#include <command.hpp>

#include <catch2/catch.hpp>

using con::Command;
using con::CommandType;
using std::string_view;

namespace {

void command_test(const string_view cmd, CommandType type) {
    const auto command = Command::parse_json_command(13, cmd);
    REQUIRE(command.id == 13);
    REQUIRE(command.type == type);
}

}

TEST_CASE("Invalid JSON command") {
    command_test("This is not a JSON", CommandType::Broken);
}

TEST_CASE("Unknown command") {
    command_test("{\"command\": \"SOME_CRAP\"}", CommandType::Unknown);
}

TEST_CASE("Send info command") {
    command_test("{\"command\": \"SEND_INFO\"}", CommandType::SendInfo);
}

TEST_CASE("Send info with params") {
    command_test("{\"command\": \"SEND_INFO\", \"args\": [], \"kwargs\": {}}", CommandType::SendInfo);
}
