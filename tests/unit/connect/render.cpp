#include <render.hpp>
#include <transfers/monitor.hpp>

#include "mock_printer.h"

#include <catch2/catch.hpp>

#include <cstring>
#include <string>
#include <string_view>
#include <sstream>

using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::stringstream;
using transfers::Monitor;
using namespace connect_client;
using namespace json;

namespace {

constexpr const char *print_file = "/usb/box.gco";
constexpr const char *rejected_event_printing = R"({"reason":"Job ID doesn't match","state":"PRINTING","command_id":11,"event":"REJECTED"})";
constexpr const char *rejected_event_idle = R"({"state":"IDLE","command_id":11,"event":"REJECTED"})";
constexpr const char *rejected_event_idle_no_job = R"({"reason":"No job in progress","state":"IDLE","command_id":11,"event":"REJECTED"})";

constexpr Printer::Params params_printing() {
    Printer::Params params {};

    params.job_id = 42;
    params.progress_percent = 12;
    params.job_path = print_file;
    params.job_lfn = "box.gcode";
    params.temp_bed = 65;
    params.temp_nozzle = 200;
    params.target_bed = 70;
    params.target_nozzle = 195;
    params.state = Printer::DeviceState::Printing;

    return params;
};

}

TEST_CASE("Render") {
    string expected;
    Printer::Params params {};
    Action action;
    optional<Monitor::Slot> transfer_slot = nullopt;
    optional<CommandId> background_command_id = nullopt;

    SECTION("Telemetry - empty") {
        params = params_printing();
        action = SendTelemetry { true };
        expected = "{}";
    }

    SECTION("Telemetry - printing") {
        params = params_printing();
        action = SendTelemetry { false };
        // clang-format off
        expected = "{"
            "\"time_printing\":0,"
            "\"time_remaining\":0,"
            "\"progress\":12,"
            "\"temp_nozzle\":200.0,"
            "\"temp_bed\":65.0,"
            "\"target_nozzle\":195.0,"
            "\"target_bed\":70.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"axis_z\":0.00,"
            "\"job_id\":42,"
            "\"fan_extruder\":0,"
            "\"fan_print\":0,"
            "\"filament\":0.0,"
            "\"state\":\"PRINTING\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - idle") {
        params = params_idle();
        action = SendTelemetry { false };
        // clang-format off
        expected = "{"
            "\"temp_nozzle\":0.0,"
            "\"temp_bed\":0.0,"
            "\"target_nozzle\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - transferring") {
        params = params_idle();
        action = SendTelemetry { false };
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"transfer_id\":" << *id << ","
            "\"transfer_transferred\":0,"
            "\"temp_nozzle\":0.0,"
            "\"temp_bed\":0.0,"
            "\"target_nozzle\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
        expected = e.str();
    }

    SECTION("Telemetry with background command") {
        params = params_idle();
        action = SendTelemetry { false };
        stringstream e;
        background_command_id = 13;
        // clang-format off
        expected = "{"
            "\"temp_nozzle\":0.0,"
            "\"temp_bed\":0.0,"
            "\"target_nozzle\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"command_id\":13,"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
    }

    SECTION("Event - rejected") {
        action = Event {
            EventType::Rejected,
            11,
        };
        params = params_idle();
        expected = rejected_event_idle;
    }

    SECTION("Event - job info") {
        action = Event {
            EventType::JobInfo,
            11,
            42,
        };
        params = params_printing();
        // clang-format off
        expected = "{"
            "\"job_id\":42,"
            R"("data":{"display_name":"box.gcode","path":"/usb/box.gco"},)"
            "\"state\":\"PRINTING\","
            "\"command_id\":11,"
            "\"event\":\"JOB_INFO\""
        "}";
        // clang-format on
    }

    SECTION("Even - job info not printing") {
        action = Event {
            EventType::JobInfo,
            11,
            42,
        };
        params = params_idle();
        expected = rejected_event_idle_no_job;
    }

    SECTION("Even - job info - invalid job ID") {
        action = Event {
            EventType::JobInfo,
            11,
            13,
        };
        params = params_printing();
        expected = rejected_event_printing;
    }

    SECTION("Event - info") {
        action = Event {
            EventType::Info,
            11,
        };
        params = params_idle();

        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"firmware\":\"TST-1234\","
                "\"sn\":\"FAKE-1234\","
                "\"appendix\":false,"
                "\"fingerprint\":\"DEADBEEF\","
                "\"storages\":[],"
                "\"network_info\":{}"
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - transfer info, no transfer") {
        action = Event {
            EventType::TransferInfo,
            11,
        };
        params = params_idle();
        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"type\":\"NO_TRANSFER\""
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"TRANSFER_INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - transfer info") {
        action = Event {
            EventType::TransferInfo,
            11,
        };
        params = params_idle();
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"data\":{"
                "\"transfer_id\":" << *id << ","
                "\"size\":1024,"
                "\"transferred\":0,"
                "\"time_transferring\":0,"
                "\"path\":\"/usb/whatever.gcode\","
                "\"type\":\"FROM_CONNECT\""
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << *id << ","
            "\"event\":\"TRANSFER_INFO\""
        "}";
        // clang-format on
        expected = e.str();
    }

    SECTION("Event - rejected with transfer") {
        action = Event {
            EventType::Rejected,
            11,
        };
        params = params_idle();
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << *id << ","
            "\"event\":\"REJECTED\""
        "}";
        // clang-format on
        expected = e.str();
    }
    // if nullptr passed instead of a path, the resulting
    // response should omit the path param.
    SECTION("Event - transfer info no upload path") {
        action = Event {
            EventType::TransferInfo,
            11,
        };
        params = params_idle();
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, nullptr, 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"data\":{"
                "\"transfer_id\":" << *id << ","
                "\"size\":1024,"
                "\"transferred\":0,"
                "\"time_transferring\":0,"
                "\"type\":\"FROM_CONNECT\""
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << *id << ","
            "\"event\":\"TRANSFER_INFO\""
        "}";
        // clang-format on
        expected = e.str();
    }

    MockPrinter printer(params);
    Tracked telemetry_changes;
    RenderState state(printer, action, telemetry_changes, background_command_id);
    Renderer renderer(std::move(state));
    uint8_t buffer[1024];
    const auto [result, amount] = renderer.render(buffer, sizeof buffer);

    REQUIRE(result == JsonResult::Complete);

    const uint8_t *b = buffer;
    string_view output(reinterpret_cast<const char *>(b), amount);
    REQUIRE(output == expected);
}
