#include <command.hpp>

#include <cstring>
#include <catch2/catch.hpp>

using namespace connect_client;
using std::get;
using std::holds_alternative;
using std::string_view;

namespace {

SharedBuffer buffer;

template <class D>
D command_test(const string_view cmd) {
    auto borrow = buffer.borrow();
    // Not claimed / left hanging by previous test.
    REQUIRE(borrow.has_value());
    const auto command = Command::parse_json_command(13, cmd, std::move(*borrow));
    REQUIRE(command.id == 13);
    REQUIRE(holds_alternative<D>(command.command_data));
    return get<D>(command.command_data);
}

}

TEST_CASE("Invalid JSON command") {
    command_test<BrokenCommand>("This is not a JSON");
}

TEST_CASE("Unknown command") {
    command_test<UnknownCommand>("{\"command\": \"SOME_CRAP\"}");
}

TEST_CASE("Array command") {
    command_test<BrokenCommand>("[\"hello\"]");
}

TEST_CASE("Send info command") {
    command_test<SendInfo>("{\"command\": \"SEND_INFO\"}");
}

TEST_CASE("Send info with params") {
    command_test<SendInfo>("{\"command\": \"SEND_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Send job info") {
    REQUIRE(command_test<SendJobInfo>("{\"command\": \"SEND_JOB_INFO\", \"args\": [42], \"kwargs\": {\"job_id\": 42}}").job_id == 42);
}

TEST_CASE("Send job info - missing args") {
    command_test<BrokenCommand>("{\"command\": \"SEND_JOB_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Send file info") {
    REQUIRE(strcmp(command_test<SendFileInfo>("{\"command\": \"SEND_FILE_INFO\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("Send file info - missing args") {
    command_test<BrokenCommand>("{\"command\": \"SEND_FILE_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Pause print") {
    command_test<PausePrint>("{\"command\": \"PAUSE_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Resume print") {
    command_test<ResumePrint>("{\"command\": \"RESUME_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Stop print") {
    command_test<StopPrint>("{\"command\": \"STOP_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Start print") {
    REQUIRE(strcmp(command_test<StartPrint>("{\"command\": \"START_PRINT\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("Start print - SFN") {
    REQUIRE(strcmp(command_test<StartPrint>("{\"command\": \"START_PRINT\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path_sfn\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("Start print - missing args") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Set printer ready") {
    command_test<SetPrinterReady>("{\"command\": \"SET_PRINTER_READY\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Send transfer info") {
    command_test<SendTransferInfo>("{\"command\": \"SEND_TRANSFER_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("Start connect download - missing params") {
    command_test<BrokenCommand>("{\"command\": \"START_CONNECT_DOWNLOAD\", \"args\": [], \"kwargs\": {}}");
    command_test<BrokenCommand>("{\"command\": \"START_CONNECT_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"hash\": \"abcdef\"}}");
    command_test<BrokenCommand>("{\"command\": \"START_CONNECT_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42}}");
    command_test<BrokenCommand>("{\"command\": \"START_CONNECT_DOWNLOAD\", \"args\": [], \"kwargs\": {\"team_id\": 42, \"hash\": \"abcdef\"}}");
}

TEST_CASE("Start connect download") {
    auto cmd = command_test<StartConnectDownload>("{\"command\": \"START_CONNECT_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42, \"hash\": \"abcdef\"}}");
    REQUIRE(strcmp(cmd.hash, "abcdef") == 0);
    REQUIRE(cmd.team == 42);
    REQUIRE(strcmp(cmd.path.path(), "/usb/whatever.gcode") == 0);
}

TEST_CASE("Start connect download - reversed") {
    // The command field is after the args, check that we are able to deal with it in the wrong order too.
    auto cmd = command_test<StartConnectDownload>("{\"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42, \"hash\": \"abcdef\"}, \"command\": \"START_CONNECT_DOWNLOAD\"}");
    REQUIRE(strcmp(cmd.hash, "abcdef") == 0);
    REQUIRE(cmd.team == 42);
    REQUIRE(strcmp(cmd.path.path(), "/usb/whatever.gcode") == 0);
}
