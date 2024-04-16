#include <command.hpp>

#include <cstring>
#include <catch2/catch.hpp>

using namespace connect_client;
using std::array;
using std::get;
using std::holds_alternative;
using std::string_view;

namespace {

SharedBuffer buffer;

template <class D>
D command_test(const char *cmd) {
    auto borrow = buffer.borrow();
    char *cmd_str = new char[strlen(cmd) + 1];
    strcpy(cmd_str, cmd);
    // Not claimed / left hanging by previous test.
    REQUIRE(borrow.has_value());
    const auto command = Command::parse_json_command(13, cmd_str, strlen(cmd_str), std::move(*borrow));
    REQUIRE(command.id == 13);
    REQUIRE(holds_alternative<D>(command.command_data));
    return get<D>(command.command_data);
}

} // namespace

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

TEST_CASE("Start connect download - encrypted") {
    auto cmd = command_test<StartEncryptedDownload>("{\"command\":\"START_ENCRYPTED_DOWNLOAD\",\"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"key\": \"000102030405060708090a0B0c0D0e0F\", \"iv\": \"101112131415161718191a1B1c1D1e1F\", \"orig_size\": 42}}");
    REQUIRE(strcmp(cmd.path.path(), "/usb/whatever.gcode") == 0);
    REQUIRE(cmd.orig_size == 42);
    array<uint8_t, 16> expected;
    for (uint8_t i = 0; i < expected.size(); i++) {
        expected[i] = i;
    }
    REQUIRE(cmd.key == expected);
    for (uint8_t i = 0; i < expected.size(); i++) {
        expected[i] = 16 + i;
    }
    REQUIRE(cmd.iv == expected);
}

TEST_CASE("Set token") {
    auto cmd = command_test<SetToken>("{\"command\": \"SET_TOKEN\",\"kwargs\": {\"token\":\"toktoktok\"}}");
    REQUIRE(strcmp(reinterpret_cast<const char *>(cmd.token->data()), "toktoktok") == 0);
}

TEST_CASE("Set token ‒ missing params") {
    command_test<BrokenCommand>("{\"command\":\"SET_TOKEN\",\"kwargs\": {}}");
}

TEST_CASE("Set token ‒ Too long") {
    command_test<BrokenCommand>("{\"command\":\"SET_TOKEN\",\"kwargs\": {\"token\":\"123456789012345678901234567890\"}}");
}
