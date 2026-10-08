#include <render.hpp>
#include <transfers/monitor.hpp>

#include "mock_printer.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

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

Printer::Params params_printing() {
    static SharedBuffer buffer;
    static optional<BorrowPaths> paths(*buffer.borrow());
    strcpy(paths->path(), print_file);
    strcpy(paths->name(), "box.gcode");
    Printer::Params params(paths);

    params.job_id = 42;
    params.has_job = true;
    params.progress_percent = 12;
    params.temp_bed = 65;
    params.physical_tools[PhysicalToolIndex::from_raw(0)].temp_nozzle = 200;
    params.target_bed = 70;
    params.physical_tools[PhysicalToolIndex::from_raw(0)].target_nozzle = 195;
    params.physical_tool_mask = 1;
    params.virtual_tool_mask = 1;
    params.virtual_tools[VirtualToolIndex::from_raw(0)].odometer_id = 1234;
    params.virtual_tools[VirtualToolIndex::from_raw(0)].odometer_e_mm = 5678;
    params.active_physical_tool = PhysicalToolIndex::from_raw(0);
    params.state = printer_state::DeviceState::Printing;

    return params;
};

/// Renders the action in chunks of @p chunk_size, calling @p between_chunks after each but the last
string render_chunked(const Printer &printer, const Action &action, size_t chunk_size, auto &&between_chunks) {
    Renderer renderer(RenderState(printer, action, nullopt));
    string output;
    while (true) {
        uint8_t buffer[1024];
        const auto [result, amount] = renderer.render(buffer, chunk_size);
        REQUIRE((result == JsonResult::Complete || result == JsonResult::Incomplete));
        output.append(reinterpret_cast<const char *>(buffer), amount);
        if (result == JsonResult::Complete) {
            return output;
        }
        between_chunks();
    }
}

} // namespace

TEST_CASE("connect::Render", "[connect]") {
    string expected;
    optional<Printer::Params> params;
    Action action;
    optional<Monitor::Slot> transfer_slot = nullopt;
    optional<CommandId> background_command_id = nullopt;
    optional<FilamentTypeParameters> material = nullopt;

    SECTION("Telemetry - reduced") {
        params.emplace(params_printing());
        action = SendTelemetry { SendTelemetry::Mode::Reduced };
        // clang-format off
        expected = "{"
            "\"job_id\":42,"
            "\"time_printing\":0,"
            "\"time_remaining\":0,"
            "\"filament_change_in\":0,"
            "\"progress\":12,"
            "\"state\":\"PRINTING\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - printing") {
        params.emplace(params_printing());
        action = SendTelemetry { SendTelemetry::Mode::Full };
        // clang-format off
        expected = "{"
            "\"job_id\":42,"
            "\"time_printing\":0,"
            "\"time_remaining\":0,"
            "\"filament_change_in\":0,"
            "\"progress\":12,"
            "\"temp_bed\":65.0,"
            "\"target_bed\":70.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":{\"type\":\"FFF_PRINT_HEAD\",\"active\":true,\"nozzle_temp\":200.0,\"nozzle_target_temp\":195,\"heatbreak_fan_rpm\":0,\"print_fan_rpm\":0,\"consumed_weight\":null,\"odometer_id\":1234,\"odometer_e\":5678}},"
            "\"state\":\"PRINTING\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - idle") {
        params.emplace(params_idle());
        action = SendTelemetry { SendTelemetry::Mode::Full };
        // clang-format off
        expected = "{"
            "\"temp_bed\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":{\"type\":\"FFF_PRINT_HEAD\",\"active\":true,\"nozzle_temp\":0.0,\"nozzle_target_temp\":null,\"heatbreak_fan_rpm\":0,\"print_fan_rpm\":0,\"consumed_weight\":null,\"odometer_id\":0,\"odometer_e\":0}},"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - disabled head") {
        auto idle = params_idle();
        // No tool enabled, like the error printer reports
        idle.physical_tool_mask = 0;
        params.emplace(idle);
        action = SendTelemetry { SendTelemetry::Mode::Full };
        // clang-format off
        expected = "{"
            "\"temp_bed\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":null},"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - no head picked") {
        auto idle = params_idle();
        idle.active_physical_tool = NoTool {};
        params.emplace(idle);
        action = SendTelemetry { SendTelemetry::Mode::Full };
        // clang-format off
        expected = "{"
            "\"temp_bed\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":{\"type\":\"FFF_PRINT_HEAD\",\"active\":false,\"nozzle_temp\":0.0,\"nozzle_target_temp\":null,\"heatbreak_fan_rpm\":0,\"print_fan_rpm\":0,\"consumed_weight\":null,\"odometer_id\":0,\"odometer_e\":0}},"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
    }

    SECTION("Telemetry - transferring") {
        params.emplace(params_idle());
        action = SendTelemetry { SendTelemetry::Mode::Full };
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"transfer_id\":" << (*id).to_uint32_t() << ","
            "\"transfer_transferred\":0,"
            "\"transfer_time_remaining\":0,"
            "\"transfer_progress\":0.0,"
            "\"temp_bed\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":{\"type\":\"FFF_PRINT_HEAD\",\"active\":true,\"nozzle_temp\":0.0,\"nozzle_target_temp\":null,\"heatbreak_fan_rpm\":0,\"print_fan_rpm\":0,\"consumed_weight\":null,\"odometer_id\":0,\"odometer_e\":0}},"
            "\"state\":\"IDLE\""
        "}";
        // clang-format on
        expected = e.str();
    }

    SECTION("Telemetry with background command") {
        params.emplace(params_idle());
        action = SendTelemetry { SendTelemetry::Mode::Full };
        background_command_id = 13;
        // clang-format off
        expected = "{"
            "\"temp_bed\":0.0,"
            "\"target_bed\":0.0,"
            "\"speed\":0,"
            "\"flow\":0,"
            "\"chamber\":{\"temp\":0.0,\"target_temp\":0,\"fan_1_rpm\":0,\"fan_2_rpm\":0,\"fan_pwm_target\":-1,\"led_intensity\":0},"
            "\"axis_x\":0.00,"
            "\"axis_y\":0.00,"
            "\"axis_z\":0.00,"
            "\"tools\":{\"0\":{\"type\":\"FFF_PRINT_HEAD\",\"active\":true,\"nozzle_temp\":0.0,\"nozzle_target_temp\":null,\"heatbreak_fan_rpm\":0,\"print_fan_rpm\":0,\"consumed_weight\":null,\"odometer_id\":0,\"odometer_e\":0}},"
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
        params.emplace(params_idle());
        expected = rejected_event_idle;
    }

    SECTION("Event - job info") {
        action = Event {
            EventType::JobInfo,
            11,
            42,
        };
        params.emplace(params_printing());
        // clang-format off
        expected = "{"
            "\"job_id\":42,"
            R"("data":{"state":"PRINTING","display_name":"box.gcode","path":"/usb/box.gco"},)"
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
        params.emplace(params_idle());
        expected = rejected_event_idle_no_job;
    }

    SECTION("Even - job info - invalid job ID") {
        action = Event {
            EventType::JobInfo,
            11,
            13,
        };
        params.emplace(params_printing());
        expected = rejected_event_printing;
    }

    SECTION("Even - job info - old job ID FINISHED") {
        action = Event {
            EventType::JobInfo,
            11,
            41,
        };
        params.emplace(params_printing());
        // clang-format off
        expected = "{"
            "\"job_id\":42,"
            R"("data":{"state":"FIN_OK"},)"
            "\"state\":\"PRINTING\","
            "\"command_id\":11,"
            "\"event\":\"JOB_INFO\""
        "}";
        // clang-format on
    }

    SECTION("Even - job info - old job ID ABORTED") {
        action = Event {
            EventType::JobInfo,
            11,
            40,
        };
        params.emplace(params_idle());
        // clang-format off
        expected = "{"
            R"("data":{"state":"FIN_STOPPED"},)"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"JOB_INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - info") {
        action = Event {
            EventType::Info,
            11,
        };
        params.emplace(params_idle());

        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"firmware\":\"TST-1234\","
                "\"printer_type\":\"2.3.0\","
                "\"sn\":\"FAKE-1234\","
                "\"appendix\":false,"
                "\"fingerprint\":\"DEADBEEF\","
                "\"transfer_paused\":true,"
                "\"storages\":[],"
                "\"network_info\":{\"hostname\":\"\"},"
                "\"tools\":{"
                    "\"0\":{"
                        "\"type\":\"FFF_PRINT_HEAD\","
                        "\"features\":{"
                            "\"nozzle_diameter\":0.40,"
                            "\"nozzle_hardened\":false,"
                            "\"nozzle_high_flow\":false"
                        "},"
                        "\"feeder\":null,"
                        "\"material_package_instance\":null"
                    "}"
                "},"
                "\"addon_power\":false,"
                "\"tool_count\":1"
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - info - disabled head") {
        action = Event {
            EventType::Info,
            11,
        };
        auto idle = params_idle();
        // No tool enabled, like the error printer reports
        idle.physical_tool_mask = 0;
        params.emplace(idle);

        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"firmware\":\"TST-1234\","
                "\"printer_type\":\"2.3.0\","
                "\"sn\":\"FAKE-1234\","
                "\"appendix\":false,"
                "\"fingerprint\":\"DEADBEEF\","
                "\"transfer_paused\":true,"
                "\"storages\":[],"
                "\"network_info\":{\"hostname\":\"\"},"
                "\"tools\":{"
                    "\"0\":null"
                "},"
                "\"addon_power\":false,"
                "\"tool_count\":1"
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - info - material") {
        action = Event {
            EventType::Info,
            11,
        };
        params.emplace(params_idle());
        material = FilamentTypeParameters {
            .name = "PETG",
            .nozzle_temperature = 230,
            .nozzle_preheat_temperature = 170,
            .heatbed_temperature = 90,
            .is_abrasive = true,
        };

        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"firmware\":\"TST-1234\","
                "\"printer_type\":\"2.3.0\","
                "\"sn\":\"FAKE-1234\","
                "\"appendix\":false,"
                "\"fingerprint\":\"DEADBEEF\","
                "\"transfer_paused\":true,"
                "\"storages\":[],"
                "\"network_info\":{\"hostname\":\"\"},"
                "\"tools\":{"
                    "\"0\":{"
                        "\"type\":\"FFF_PRINT_HEAD\","
                        "\"features\":{"
                            "\"nozzle_diameter\":0.40,"
                            "\"nozzle_hardened\":false,"
                            "\"nozzle_high_flow\":false"
                        "},"
                        "\"feeder\":null,"
                        "\"material_package_instance\":{"
                            "\"class\":\"FFF\","
                            "\"package\":{"
                                "\"class\":\"FFF\","
                                "\"material\":{"
                                    "\"class\":\"FFF\","
                                    "\"abbreviation\":\"PETG\","
                                    "\"tags\":[\"abrasive\"],"
                                    "\"properties\":{"
                                        "\"min_print_temperature\":230,"
                                        "\"max_print_temperature\":230,"
                                        "\"preheat_temperature\":170,"
                                        "\"min_bed_temperature\":90,"
                                        "\"max_bed_temperature\":90"
                                    "}"
                                "}"
                            "}"
                        "}"
                    "}"
                "},"
                "\"addon_power\":false,"
                "\"tool_count\":1"
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"event\":\"INFO\""
        "}";
        // clang-format on
    }

    SECTION("Event - info - material with color") {
        action = Event {
            EventType::Info,
            11,
        };
        params.emplace(params_idle());
        material = FilamentTypeParameters {
            .name = "FLEX",
            .color = Color::from_rgb(0x12, 0xab, 0xef),
            .nozzle_temperature = 240,
            .nozzle_preheat_temperature = 170,
            .heatbed_temperature = 50,
        };

        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"firmware\":\"TST-1234\","
                "\"printer_type\":\"2.3.0\","
                "\"sn\":\"FAKE-1234\","
                "\"appendix\":false,"
                "\"fingerprint\":\"DEADBEEF\","
                "\"transfer_paused\":true,"
                "\"storages\":[],"
                "\"network_info\":{\"hostname\":\"\"},"
                "\"tools\":{"
                    "\"0\":{"
                        "\"type\":\"FFF_PRINT_HEAD\","
                        "\"features\":{"
                            "\"nozzle_diameter\":0.40,"
                            "\"nozzle_hardened\":false,"
                            "\"nozzle_high_flow\":false"
                        "},"
                        "\"feeder\":null,"
                        "\"material_package_instance\":{"
                            "\"class\":\"FFF\","
                            "\"package\":{"
                                "\"class\":\"FFF\","
                                "\"material\":{"
                                    "\"class\":\"FFF\","
                                    "\"abbreviation\":\"FLEX\","
                                    "\"primary_color\":{\"color_rgba\":\"#12abef\"},"
                                    "\"tags\":[],"
                                    "\"properties\":{"
                                        "\"min_print_temperature\":240,"
                                        "\"max_print_temperature\":240,"
                                        "\"preheat_temperature\":170,"
                                        "\"min_bed_temperature\":50,"
                                        "\"max_bed_temperature\":50"
                                    "}"
                                "}"
                            "}"
                        "}"
                    "}"
                "},"
                "\"addon_power\":false,"
                "\"tool_count\":1"
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
        params.emplace(params_idle());
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
        params.emplace(params_idle());
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"data\":{"
                "\"size\":1024,"
                "\"transferred\":0,"
                "\"progress\":0.0,"
                "\"time_remaining\":0,"
                "\"time_transferring\":0,"
                "\"path\":\"/usb/whatever.gcode\","
                "\"type\":\"FROM_CONNECT\""
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << (*id).to_uint32_t() << ","
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
        params.emplace(params_idle());
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/whatever.gcode", 1024);
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << (*id).to_uint32_t() << ","
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
        params.emplace(params_idle());
        transfer_slot = Monitor::instance.allocate(Monitor::Type::Connect, nullptr, 1024);
        REQUIRE(transfer_slot.has_value());
        auto id = Monitor::instance.id();
        REQUIRE(id.has_value());
        stringstream e;
        // clang-format off
        e << "{"
            "\"data\":{"
                "\"size\":1024,"
                "\"transferred\":0,"
                "\"progress\":0.0,"
                "\"time_remaining\":0,"
                "\"time_transferring\":0,"
                "\"type\":\"FROM_CONNECT\""
            "},"
            "\"state\":\"IDLE\","
            "\"command_id\":11,"
            "\"transfer_id\":" << (*id).to_uint32_t() << ","
            "\"event\":\"TRANSFER_INFO\""
        "}";
        // clang-format on
        expected = e.str();
    }

    SECTION("Event - state changed with dialog") {
        action = Event {
            EventType::StateChanged,
            11
        };
        params.emplace(params_dialog());
        // clang-format off
        expected = "{"
            "\"data\":{"
                "\"code\":\"00000\","
                "\"buttons\":[\"Yes\",\"No\"]"
            "},"
            "\"dialog_id\":42,"
            "\"state\":\"ATTENTION\","
            "\"command_id\":11,"
            "\"event\":\"STATE_CHANGED\""
        "}";
        // clang-format on
    }

    MockPrinter printer(params.value());
    printer.loaded_material = material;
    RenderState state(printer, action, background_command_id);
    Renderer renderer(std::move(state));
    uint8_t buffer[1024];
    const auto [result, amount] = renderer.render(buffer, sizeof buffer);

    REQUIRE(result == JsonResult::Complete);

    const uint8_t *b = buffer;
    string_view output(reinterpret_cast<const char *>(b), amount);
    REQUIRE(output == expected);
}

TEST_CASE("connect::Render - tool set and active tool are sampled once per response", "[connect]") {
    Action action;
    SECTION("Telemetry") {
        action = SendTelemetry { SendTelemetry::Mode::Full };
    }
    SECTION("Info") {
        action = Event { EventType::Info, 11 };
    }

    auto params = params_idle();
    MockPrinter printer(params);

    const string in_one_go = render_chunked(printer, action, 1024, [] {});

    // The tools change after every chunk; the chunk sizes move the change points across the whole message,
    // the tool loops included. The smallest chunk still fits the longest field.
    for (size_t chunk_size = 40; chunk_size <= 256; chunk_size++) {
        CAPTURE(chunk_size);
        params.physical_tool_mask = 1;
        params.active_physical_tool = PhysicalToolIndex::from_raw(0);
        bool changed = false;
        const string chunked = render_chunked(printer, action, chunk_size, [&] {
            changed = !changed;
            params.physical_tool_mask = changed ? 0 : 1;
            params.active_physical_tool = changed ? decltype(params.active_physical_tool)(NoTool {}) : PhysicalToolIndex::from_raw(0);
        });
        REQUIRE(chunked == in_one_go);
    }
}

TEST_CASE("connect::Render - the material is sampled once per tool", "[connect]") {
    const Action action = Event { EventType::Info, 11 };
    const auto params = params_idle();
    MockPrinter printer(params);

    const FilamentTypeParameters pla {
        .name = "PLA",
        .nozzle_temperature = 215,
        .heatbed_temperature = 60,
    };
    const FilamentTypeParameters asa {
        .name = "ASA",
        .color = Color::from_rgb(0xff, 0, 0),
        .nozzle_temperature = 260,
        .heatbed_temperature = 100,
        .is_abrasive = true,
    };

    printer.loaded_material = pla;
    const string with_pla = render_chunked(printer, action, 1024, [] {});
    printer.loaded_material = asa;
    const string with_asa = render_chunked(printer, action, 1024, [] {});
    REQUIRE(with_pla != with_asa);

    // The test printer has a single tool. Swapping its material after every chunk, at every possible place in the
    // message, must still give the whole message of one of the two materials, never a mix of both.
    for (size_t chunk_size = 40; chunk_size <= 256; chunk_size++) {
        CAPTURE(chunk_size);
        printer.loaded_material = pla;
        const string chunked = render_chunked(printer, action, chunk_size, [&] {
            printer.loaded_material = printer.loaded_material->is_abrasive ? pla : asa;
        });
        REQUIRE((chunked == with_pla || chunked == with_asa));
    }
}
