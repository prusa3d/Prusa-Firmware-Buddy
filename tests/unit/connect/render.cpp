#include <render.hpp>

#include "mock_printer.h"

#include <catch2/catch.hpp>

#include <cstring>
#include <string_view>

using std::nullopt;
using std::optional;
using std::string_view;
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
    string_view expected;
    Printer::Params params {};
    Action action;

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
            "\"temp_nozzle\":200.0,"
            "\"temp_bed\":65.0,"
            "\"target_nozzle\":195.0,"
            "\"target_bed\":70.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"axis_z\":0.00,"
            "\"job_id\":42,"
            "\"time_printing\":0,"
            "\"time_remaining\":0,"
            "\"progress\":12,"
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

    MockPrinter printer(params);
    RenderState state(printer, action);
    Renderer renderer(std::move(state));
    uint8_t buffer[1024];
    const auto [result, amount] = renderer.render(buffer, sizeof buffer);

    REQUIRE(result == JsonResult::Complete);

    const uint8_t *b = buffer;
    string_view output(reinterpret_cast<const char *>(b), amount);
    REQUIRE(output == expected);
}
