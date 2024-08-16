#include <command.hpp>

#include <module/prusa/tool_mapper.hpp>
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

TEST_CASE("Start print - tool mapping") {
    auto cmd = command_test<StartPrint>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"1\": [2, 3], \"3\": [4, 5, 1]}}}");
    ToolMapping expected;
    for (auto &tool : expected) {
        for (auto &num : tool) {
            num = ToolMapper::NO_TOOL_MAPPED;
        }
    }
    // NOTE: the index here shopuld always -1 from the original (0 based vs 1 based)
    expected[0] = { 1, 2, 255, 255, 255 };
    expected[2] = { 3, 4, 0, 255, 255 };
    REQUIRE(cmd.tool_mapping.has_value());
    auto tm = cmd.tool_mapping.value();
    REQUIRE(tm == expected);
}

TEST_CASE("Start print - tool mapping too many tools") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"1\": [2, 3, 4, 5, 1, 3]}}}");
}

TEST_CASE("Start print - tool mapping 6th tool") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"6\": [2, 3, 4, 5]}}}");
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

TEST_CASE("Start inline download") {
    auto cmd = command_test<StartInlineDownload>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42, \"hash\": \"abcdef\", \"orig_size\":1024}}");
    REQUIRE(strcmp(cmd.path.path(), "/usb/whatever.gcode") == 0);
    REQUIRE(strcmp(cmd.hash, "abcdef") == 0);
    REQUIRE(cmd.team_id == 42);
    REQUIRE(cmd.orig_size == 1024);
}

TEST_CASE("Start inline download - missing params") {
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"hash\": \"abcdef\"}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"team_id\": 42, \"hash\": \"abcdef\"}}");
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

TEST_CASE("Set value - hostname") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\": {\"hostname\":\"Nice_hostname\"}}");
    REQUIRE(cmd.name == PropertyName::HostName);
    REQUIRE(holds_alternative<SharedBorrow>(cmd.value));
    REQUIRE(strcmp(reinterpret_cast<const char *>(get<SharedBorrow>(cmd.value)->data()), "Nice_hostname") == 0);
}

TEST_CASE("Set value - nozzle diameter") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.2.nozzle_diameter\":0.25}}");
    REQUIRE(cmd.name == PropertyName::Nozzle1Diameter);
    REQUIRE(holds_alternative<float>(cmd.value));
    REQUIRE(get<float>(cmd.value) == 0.25);
}

TEST_CASE("Set value - anti abrasive") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.3.anti_abrasive\":true}}");
    REQUIRE(cmd.name == PropertyName::Nozzle2AntiAbrasive);
    REQUIRE(holds_alternative<bool>(cmd.value));
    REQUIRE(get<bool>(cmd.value));
}

TEST_CASE("Set value - anti abrasive") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.4.high_flow\":true}}");
    REQUIRE(cmd.name == PropertyName::Nozzle3HighFlow);
    REQUIRE(holds_alternative<bool>(cmd.value));
    REQUIRE(get<bool>(cmd.value));
}

TEST_CASE("Set value - hostname too long") {
    command_test<BrokenCommand>("{\"command\":\"SET_VALUE\",\"kwargs\": {\"hostname\":\"Nice_hostname_but_far_too_long_for_us_to_process\"}}");
}

TEST_CASE("Set value - missing params") {
    command_test<BrokenCommand>("{\"command\":\"SET_VALUE\",\"kwargs\": {}}");
}

TEST_CASE("Cancel object") {
    REQUIRE(command_test<CancelObject>("{\"command\":\"CANCEL_OBJECT\",\"kwargs\":{\"id\":3}}").id == 3);
}

TEST_CASE("Uncancel object") {
    REQUIRE(command_test<UncancelObject>("{\"command\":\"UNCANCEL_OBJECT\",\"kwargs\":{\"id\":3}}").id == 3);
}
