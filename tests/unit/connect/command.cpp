#include <command.hpp>

#include <module/prusa/tool_mapper.hpp>
#include <buddy/filename_defs.hpp>
#include <option/websocket.h>
#include <cstring>
#include <format>
#include <string>
#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("connect::Invalid JSON command", "[connect]") {
    command_test<BrokenCommand>("This is not a JSON");
}

TEST_CASE("connect::Unknown command", "[connect]") {
    command_test<UnknownCommand>("{\"command\": \"SOME_CRAP\"}");
}

TEST_CASE("connect::Array command", "[connect]") {
    command_test<BrokenCommand>("[\"hello\"]");
}

TEST_CASE("connect::Send info command", "[connect]") {
    command_test<SendInfo>("{\"command\": \"SEND_INFO\"}");
}

TEST_CASE("connect::Send info with params", "[connect]") {
    command_test<SendInfo>("{\"command\": \"SEND_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Send job info", "[connect]") {
    REQUIRE(command_test<SendJobInfo>("{\"command\": \"SEND_JOB_INFO\", \"args\": [42], \"kwargs\": {\"job_id\": 42}}").job_id == 42);
}

TEST_CASE("connect::Send job info - missing args", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"SEND_JOB_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Send file info", "[connect]") {
    REQUIRE(strcmp(command_test<SendFileInfo>("{\"command\": \"SEND_FILE_INFO\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("connect::Send file info - missing args", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"SEND_FILE_INFO\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Send file info - path too long", "[connect]") {
    // Reject a path that would overflow the shared buffer downstream.
    std::string long_path(filename_defs::max_hybrid_path_length + 1, 'x');
    std::string json = std::format(R"({{"command":"SEND_FILE_INFO","kwargs":{{"path":"{}"}}}})", long_path);
    command_test<BrokenCommand>(json.c_str());
}

TEST_CASE("connect::Send file info - path at max length", "[connect]") {
    std::string long_path(filename_defs::max_hybrid_path_length, 'x');
    std::string json = std::format(R"({{"command":"SEND_FILE_INFO","kwargs":{{"path":"{}"}}}})", long_path);
    auto cmd = command_test<SendFileInfo>(json.c_str());
    REQUIRE(strcmp(cmd.path.path(), long_path.c_str()) == 0);
}

TEST_CASE("connect::Create folder - hybrid path at max length", "[connect]") {
    // Creation commands legitimately carry an SFN prefix + LFN tail, which
    // can exceed max_path_length; only the hybrid length must be enforced.
    std::string long_path(filename_defs::max_hybrid_path_length, 'x');
    std::string json = std::format(R"({{"command":"CREATE_FOLDER","kwargs":{{"path":"{}"}}}})", long_path);
    auto cmd = command_test<CreateFolder>(json.c_str());
    REQUIRE(strcmp(cmd.path.path(), long_path.c_str()) == 0);
}

TEST_CASE("connect::Create folder - path too long", "[connect]") {
    std::string long_path(filename_defs::max_hybrid_path_length + 1, 'x');
    std::string json = std::format(R"({{"command":"CREATE_FOLDER","kwargs":{{"path":"{}"}}}})", long_path);
    command_test<BrokenCommand>(json.c_str());
}

TEST_CASE("connect::Pause print", "[connect]") {
    command_test<PausePrint>("{\"command\": \"PAUSE_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Resume print", "[connect]") {
    command_test<ResumePrint>("{\"command\": \"RESUME_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Stop print", "[connect]") {
    command_test<StopPrint>("{\"command\": \"STOP_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Start print", "[connect]") {
    REQUIRE(strcmp(command_test<StartPrint>("{\"command\": \"START_PRINT\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("connect::Start print - tool mapping", "[connect]") {
    auto cmd = command_test<StartPrint>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"1\": [2, 3], \"3\": [4, 5, 1]}}}");
    ToolMapping expected;
    for (auto &tool : expected) {
        for (auto &num : tool) {
            num = ToolMapper::NO_TOOL_MAPPED;
        }
    }
    // NOTE: the index here shopuld always -1 from the original (0 based vs 1 based)
    expected[0] = { 1, 2, 255, 255, 255, 255 };
    expected[2] = { 3, 4, 0, 255, 255, 255 };
    REQUIRE(cmd.tool_mapping.has_value());
    auto tm = cmd.tool_mapping.value();
    REQUIRE(tm == expected);
}

TEST_CASE("connect::Start print - tool mapping too many tools", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"1\": [2, 3, 4, 5, 1, 3, 2]}}}");
}

TEST_CASE("connect::Start print - tool mapping 7th tool", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"kwargs\": {\"path\": \"/usb/x.gcode\", \"tool_mapping\": {\"7\": [2, 3, 4, 5]}}}");
}

TEST_CASE("connect::Start print - SFN", "[connect]") {
    REQUIRE(strcmp(command_test<StartPrint>("{\"command\": \"START_PRINT\", \"args\": [\"/usb/x.gcode\"], \"kwargs\": {\"path_sfn\": \"/usb/x.gcode\"}}").path.path(), "/usb/x.gcode") == 0);
}

TEST_CASE("connect::Start print - missing args", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"START_PRINT\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Set printer ready", "[connect]") {
    command_test<SetPrinterReady>("{\"command\": \"SET_PRINTER_READY\", \"args\": [], \"kwargs\": {}}");
}

TEST_CASE("connect::Send transfer info", "[connect]") {
    command_test<SendTransferInfo>("{\"command\": \"SEND_TRANSFER_INFO\", \"args\": [], \"kwargs\": {}}");
}

#if !WEBSOCKET()
TEST_CASE("connect::Start connect download - encrypted", "[connect]") {
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
#endif

TEST_CASE("connect::Start inline download", "[connect]") {
    auto cmd = command_test<StartInlineDownload>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42, \"hash\": \"abcdef\", \"orig_size\":1024}}");
    REQUIRE(strcmp(cmd.path.path(), "/usb/whatever.gcode") == 0);
    REQUIRE(strcmp(cmd.hash, "abcdef") == 0);
    REQUIRE(cmd.team_id == 42);
    REQUIRE(cmd.orig_size == 1024);
}

TEST_CASE("connect::Start inline download - missing params", "[connect]") {
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"hash\": \"abcdef\"}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"path\":\"/usb/whatever.gcode\", \"team_id\": 42}}");
    command_test<BrokenCommand>("{\"command\": \"START_INLINE_DOWNLOAD\", \"args\": [], \"kwargs\": {\"team_id\": 42, \"hash\": \"abcdef\"}}");
}

TEST_CASE("connect::Set token", "[connect]") {
    auto cmd = command_test<SetToken>("{\"command\": \"SET_TOKEN\",\"kwargs\": {\"token\":\"toktoktok\"}}");
    REQUIRE(strcmp(reinterpret_cast<const char *>(cmd.token->data()), "toktoktok") == 0);
}

TEST_CASE("connect::Set token ‒ missing params", "[connect]") {
    command_test<BrokenCommand>("{\"command\":\"SET_TOKEN\",\"kwargs\": {}}");
}

TEST_CASE("connect::Set token ‒ Too long", "[connect]") {
    command_test<BrokenCommand>("{\"command\":\"SET_TOKEN\",\"kwargs\": {\"token\":\"123456789012345678901234567890\"}}");
}

TEST_CASE("connect::Set value - hostname", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\": {\"hostname\":\"Nice_hostname\"}}");
    REQUIRE(cmd.name == PropertyName::HostName);
    REQUIRE(holds_alternative<SharedBorrow>(cmd.value));
    REQUIRE(strcmp(reinterpret_cast<const char *>(get<SharedBorrow>(cmd.value)->data()), "Nice_hostname") == 0);
}

TEST_CASE("connect::Set value - first tool nozzle diameter", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.0.nozzle_diameter\":0.6}}");
    REQUIRE(cmd.name == PropertyName::NozzleDiameter);
    REQUIRE(cmd.idx == 0);
    REQUIRE(holds_alternative<float>(cmd.value));
    REQUIRE(get<float>(cmd.value) == 0.6f);
}

TEST_CASE("connect::Set value - nozzle diameter", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.1.nozzle_diameter\":0.25}}");
    REQUIRE(cmd.name == PropertyName::NozzleDiameter);
    REQUIRE(cmd.idx == 1);
    REQUIRE(holds_alternative<float>(cmd.value));
    REQUIRE(get<float>(cmd.value) == 0.25);
}

TEST_CASE("connect::Set value - anti abrasive", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.2.hardened\":true}}");
    REQUIRE(cmd.name == PropertyName::NozzleHardened);
    REQUIRE(cmd.idx == 2);
    REQUIRE(holds_alternative<bool>(cmd.value));
    REQUIRE(get<bool>(cmd.value));
}

TEST_CASE("connect::Set value - high flow", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.3.high_flow\":true}}");
    REQUIRE(cmd.name == PropertyName::NozzleHighFlow);
    REQUIRE(cmd.idx == 3);
    REQUIRE(holds_alternative<bool>(cmd.value));
    REQUIRE(get<bool>(cmd.value));
}

TEST_CASE("connect::Set value - high tool index nozzle diameter", "[connect]") {
    auto cmd = command_test<SetValue>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.7.nozzle_diameter\":0.4}}");
    REQUIRE(cmd.name == PropertyName::NozzleDiameter);
    REQUIRE(cmd.idx == 7);
}

TEST_CASE("connect::Set value - tool index past the last tool", "[connect]") {
    command_test<BrokenCommand>("{\"command\":\"SET_VALUE\",\"kwargs\":{\"tools.8.nozzle_diameter\":0.4}}");
}

void set_value_chamber_target_temp(uint32_t temperature) {
    std::string json = std::format(R"({{"command":"SET_VALUE","kwargs":{{"chamber.target_temp": {}}}}})", temperature);

    auto cmd = command_test<SetValue>(json.c_str());
    REQUIRE(cmd.name == PropertyName::ChamberTargetTemp);
    REQUIRE(holds_alternative<uint32_t>(cmd.value));
    REQUIRE(get<uint32_t>(cmd.value) == temperature);
}

TEST_CASE("connect::Set value - chamber.target_temp", "[connect]") {
    // Note: for value logic verification see related TEST_CASE("connect::Command Set value - chamber.target_temp set/unset logic")
    // in connect_planner unit test suite.
    SECTION("0") { // 0 is a special case - target_temp off. However, on the level of Set value, it is no different from any other value
        set_value_chamber_target_temp(0);
    }

    SECTION("35") { // some normal target temp
        set_value_chamber_target_temp(35);
    }

    SECTION("55") { // 55 is the limit value specified by the Connect protocol. However, we are not checking the range
        set_value_chamber_target_temp(55);
    }
}

void set_value_chamber_fan_pwm_target(int8_t pwm) {
    std::string json = std::format(R"({{"command":"SET_VALUE","kwargs":{{"chamber.fan_pwm_target": {}}}}})", pwm);

    auto cmd = command_test<SetValue>(json.c_str());
    REQUIRE(cmd.name == PropertyName::ChamberFanPwmTarget);
    REQUIRE(holds_alternative<int8_t>(cmd.value));
    REQUIRE(get<int8_t>(cmd.value) == pwm);
}

TEST_CASE("connect::Set value - chamber.fan_pwm_target", "[connect]") {
    // Note: for value logic verification see related TEST_CASE("connect::Command Set value - xbuddy_extension fan1, 2 set/unset logic")
    // in connect_planner unit test suite.
    SECTION("0") {
        set_value_chamber_fan_pwm_target(0);
    }

    SECTION("35") {
        set_value_chamber_fan_pwm_target(35);
    }

    SECTION("100") {
        set_value_chamber_fan_pwm_target(100);
    }
}

void set_value_chamber_led_intensity(int8_t pwm) {
    std::string json = std::format(R"({{"command":"SET_VALUE","kwargs":{{"chamber.led_intensity": {}}}}})", pwm);

    auto cmd = command_test<SetValue>(json.c_str());
    REQUIRE(cmd.name == PropertyName::ChamberLedIntensity);
    REQUIRE(holds_alternative<int8_t>(cmd.value));
    REQUIRE(get<int8_t>(cmd.value) == pwm);
}

TEST_CASE("connect::Set value - chamber.led_intensity", "[connect]") {
    // Note: for value logic verification see related TEST_CASE("connect::Command Set value - xbuddy_extension LED intensity logic")
    // in connect_planner unit test suite.
    SECTION("0") {
        set_value_chamber_led_intensity(0);
    }

    SECTION("35") {
        set_value_chamber_led_intensity(35);
    }

    SECTION("100") {
        set_value_chamber_led_intensity(100);
    }
}

void set_value_addon_power(bool b) {
    std::string json = std::format(R"({{"command":"SET_VALUE","kwargs":{{"addon_power": {}}}}})", b);

    auto cmd = command_test<SetValue>(json.c_str());
    REQUIRE(cmd.name == PropertyName::AddonPower);
    REQUIRE(holds_alternative<bool>(cmd.value));
    REQUIRE(get<bool>(cmd.value) == b);
}

TEST_CASE("connect::Set value - addon_power", "[connect]") {
    // Note: for value logic verification see related TEST_CASE("connect::Command Set value - xbuddy_extension usb addon power logic")
    // in connect_planner unit test suite.
    SECTION("true") {
        set_value_addon_power(true);
    }

    SECTION("false") {
        set_value_addon_power(false);
    }
}

TEST_CASE("connect::Set value - hostname too long", "[connect]") {
    command_test<BrokenCommand>("{\"command\":\"SET_VALUE\",\"kwargs\": {\"hostname\":\"Nice_hostname_but_far_too_long_for_us_to_process\"}}");
}

TEST_CASE("connect::Set value - missing params", "[connect]") {
    command_test<BrokenCommand>("{\"command\":\"SET_VALUE\",\"kwargs\": {}}");
}

TEST_CASE("connect::Cancel object", "[connect]") {
    REQUIRE(command_test<CancelObject>("{\"command\":\"CANCEL_OBJECT\",\"kwargs\":{\"id\":3}}").id == 3);
}

TEST_CASE("connect::Uncancel object", "[connect]") {
    REQUIRE(command_test<UncancelObject>("{\"command\":\"UNCANCEL_OBJECT\",\"kwargs\":{\"id\":3}}").id == 3);
}
