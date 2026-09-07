#include <cyphal_application.hpp>

#include "cyphal_presentation.hpp"
#include <tool_offset_sensor/types.hpp>
#include <algorithm>
#include <cyphal_application_impl.hpp>
#include <master_activity.hpp>
#include <catch2/catch_test_macros.hpp>
#include <modbus/traits.hpp>
#include <vector>
#include <utils/byte_utils.hpp>

// This is needed for linking but we don't need the actual implementation in tests
void cyphal::Application::log_from_app([[maybe_unused]] std::string_view s) {
}

using Application = cyphal::ApplicationImpl;
using cyphal::Command;
using cyphal::Health;
using cyphal::Heartbeat;
using cyphal::Mode;
using cyphal::NodeId;
using cyphal::Presentation;
using cyphal::Severity;
using cyphal::TimePoint;
using cyphal::TransferId;
using cyphal::UniqueId;

std::vector<uint8_t> mock_firmware_gen() {
    constexpr size_t firmware_size = 10 * 256;
    std::vector<uint8_t> result;
    for (size_t i = 0; i < firmware_size; i++) {
        result.push_back(random());
    }
    return result;
}

std::array<std::byte, 32> generate_random_digest() {
    std::array<std::byte, 32> result;
    for (auto &b : result) {
        b = std::byte(random());
    }
    return result;
}

uint32_t hal::rng::get() {
    static uint32_t counter = 0;
    return ++counter;
}

// Pretend that this is a firmware for the node.
//
// We don't want some all-zeroes or predictable pattern - that one could hide
// some reorderings or repetitions of parts or other such mistakes in transfer;
// random "noise" is likely to show them.
const std::vector<uint8_t> mock_firmware = mock_firmware_gen();

struct PnpAllocation {
    UniqueId unique_id;
    NodeId node_id;
    constexpr auto operator<=>(const PnpAllocation &) const = default;
};

struct ExecuteCommandRequest {
    NodeId node_id;
    Command command;
    std::vector<std::byte> parameter;
    constexpr auto operator<=>(const ExecuteCommandRequest &) const = default;
};

namespace cyphal {
std::ostream &operator<<(std::ostream &os, const NodeId &node_id) {
    return os << (int)(uint8_t)node_id;
}
std::ostream &operator<<(std::ostream &os, const Command &command) {
    switch (command) {
    case Command::start_app:
        return os << "start_app";
    case Command::get_app_salted_hash:
        return os << "get_app_salted_hash";
    case Command::software_update:
        return os << "software_update";
    case Command::restart:
        return os << "restart";
    }
    return os << "invalid";
}
} // namespace cyphal

std::ostream &operator<<(std::ostream &os, const ExecuteCommandRequest &request) {
    os << "{ " << request.node_id << ", " << request.command << ", {";
    for (const auto &byte : request.parameter) {
        os << (int)byte << ", ";
    }
    return os << "} }";
}

class MockPresentation final : public Presentation {
public:
    std::vector<PnpAllocation> pnp_allocation;
    std::vector<NodeId> node_get_info_request;
    uint32_t last_uptime = 0;
    uint32_t heartbeat_count = 0;
    bool last_heartbeat_healthy = true;

    std::vector<ExecuteCommandRequest> node_execute_command_request;

    struct FileResponse {
        NodeId remote_node_id;
        TransferId transfer_id;
        std::vector<std::byte> data;

        constexpr auto operator<=>(const FileResponse &) const = default;
    };

    std::vector<FileResponse> file_responses;

    void transmit_heartbeat(uint32_t uptime, bool healthy) {
        last_uptime = uptime;
        ++heartbeat_count;
        last_heartbeat_healthy = healthy;
    }

    void transmit_pnp_allocation(const UniqueId &unique_id, NodeId node_id) {
        pnp_allocation.emplace_back(unique_id, node_id);
    }
    void transmit_diagnostic_record(Severity, const char *text) {
        WARN(text);
    }
    void transmit_node_get_info_request(NodeId remote_node_id) {
        node_get_info_request.emplace_back(remote_node_id);
    }
    void transmit_node_execute_command_request(NodeId remote_node_id, Command command, Bytes parameter) {
        node_execute_command_request.emplace_back(remote_node_id, command, std::vector<std::byte> { parameter.begin(), parameter.end() });
    }
    void transmit_file_read_response(NodeId remote_node_id, TransferId transfer_id, WritableBytes data) {
        file_responses.push_back(FileResponse { remote_node_id, transfer_id, std::vector(data.begin(), data.end()) });
    }

    void transmit_ac_controller_config_request([[maybe_unused]] NodeId remote_node_id, [[maybe_unused]] const ac_controller::Config &) {
        abort();
    }

    void transmit_ac_controller_leds_config_request([[maybe_unused]] NodeId, [[maybe_unused]] const ac_controller::LedConfig &) {
        abort();
    }

    struct ToolOffsetSensorConfigRequest {
        NodeId node_id;
        tool_offset_sensor::Config config;
        constexpr auto operator<=>(const ToolOffsetSensorConfigRequest &) const = default;
    };
    std::vector<ToolOffsetSensorConfigRequest> tool_offset_sensor_config_requests;

    void transmit_tool_offset_sensor_config_request(NodeId node_id, const tool_offset_sensor::Config &config) {
        tool_offset_sensor_config_requests.push_back({ node_id, config });
    }

    struct NfcCommand {
        NodeId node_id;
        std::vector<std::byte> data;
        constexpr auto operator<=>(const NfcCommand &) const = default;
    };
    std::vector<NfcCommand> nfc_requests;
    std::vector<NfcCommand> nfc_accept_events;

    bool transmit_nfc_command_request(NodeId remote_node_id, Bytes data) final {
        nfc_requests.push_back({ remote_node_id, std::vector<std::byte>(data.begin(), data.end()) });
        return true;
    }

    bool transmit_nfc_command_accept_event(NodeId remote_node_id, Bytes data) final {
        nfc_accept_events.push_back({ remote_node_id, std::vector<std::byte>(data.begin(), data.end()) });
        return true;
    }

    constexpr auto operator<=>(const MockPresentation &) const = default;
};

std::ostream &operator<<(std::ostream &os, const MockPresentation &mock) {
    os << "{ ";
    os << ".last_uptime = " << mock.last_uptime << ",";
    os << ".heartbeat_count = " << mock.heartbeat_count << ",";
    os << ".node_execute_command_request = {";
    for (const auto &x : mock.node_execute_command_request) {
        os << x << ",";
    }
    os << "}, ";
    os << ".node_get_info_request = {";
    for (const auto &x : mock.node_get_info_request) {
        os << x << ",";
    }
    os << "}, ";
    os << " }";
    return os;
}

UniqueId make_unique_id(uint32_t x) {
    union {
        uint8_t u8[16];
        uint32_t u32[4];
    } data;
    data.u32[0] = x;
    data.u32[1] = data.u32[2] = data.u32[3] = 0;
    return UniqueId { data.u8 };
};

TimePoint make_timepoint(uint32_t ms) {
    return TimePoint { std::chrono::milliseconds { ms } };
}

ExecuteCommandRequest make_salted_hash_request(NodeId node_id, uint32_t salt) {
    const auto bytes = trivial_as_bytes(salt);
    return ExecuteCommandRequest { node_id, Command::get_app_salted_hash, { bytes.begin(), bytes.end() } };
}

std::vector<std::byte> as_bytes(const anfc::modbus::Event &event) {
    auto span = modbus::payload(event);
    return { span.begin(), span.end() };
}

std::vector<std::byte> as_bytes(const anfc::modbus::Request &request) {
    auto span = modbus::payload(request);
    return { span.begin(), span.end() };
}

template <size_t N>
std::vector<std::byte> as_bytes(const std::array<std::byte, N> &arr) {
    return { arr.begin(), arr.end() };
}

MockPresentation run_without_master_activity(Application &app, MockPresentation &mock, TimePoint timepoint) {
    auto mock_before = mock;
    while (app.step(mock, timepoint)) {
    }
    return mock_before;
}

MockPresentation run(Application &app, MockPresentation &mock, TimePoint timepoint) {
    master_activity.fetch_add(1);
    return run_without_master_activity(app, mock, timepoint);
}

const auto known_node_name = [] {
    std::string_view name { "cz.prusa3d.honeybee.ac_controller" };
    return as_bytes(std::span { name.data(), name.size() });
}();

const auto nfc_node_name = [] {
    std::string_view name { "cz.prusa3d.honeybee.nfc" };
    return as_bytes(std::span { name.data(), name.size() });
}();

const auto unknown_node_name = [] {
    std::string_view name { "cz.prusa3d.honeybee.unknown" };
    return as_bytes(std::span { name.data(), name.size() });
}();

const auto tool_offset_sensor_node_name = [] {
    std::string_view name { "cz.prusa3d.honeybee.tool_offset_sensor" };
    return as_bytes(std::span { name.data(), name.size() });
}();

void setup_nfc_node(Application &app, MockPresentation &mock, NodeId node_id, uint32_t unique_id, uint32_t &time) {
    // node asks for PNP allocation, it is given a node id
    {
        app.receive_pnp_allocation(make_unique_id(unique_id));
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.pnp_allocation.size() == mock_before.pnp_allocation.size() + 1);
        REQUIRE(mock.pnp_allocation.back().node_id == node_id);
    }
    // node sends a heartbeat, it receives a node info request
    {
        const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };
        app.receive_node_heartbeat(node_id, make_timepoint(time), healthy_firmware);
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.node_get_info_request.size() == mock_before.node_get_info_request.size() + 1);
        REQUIRE(mock.node_get_info_request.back() == node_id);
    }
    // node responds with node info, it receives a digest request
    {
        app.receive_node_get_info_response(node_id, nfc_node_name);
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
        REQUIRE(mock.node_execute_command_request.back().command == Command::get_app_salted_hash);
        REQUIRE(app.request().hash_request == cyphal::FirmwareFile::firmware_anfc);
    }
    // node and motherboard respond with a matching hash, no more work is done
    {
        const auto digest = generate_random_digest();
        const auto received = app.receive_digest(cyphal::FirmwareFile::firmware_anfc, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest);
        REQUIRE(received);
        app.receive_node_execute_command_response(node_id, 0, digest);
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock == mock_before);
    }
}

// Drive a tool offset sensor node from anonymous through full firmware
// verification into the ToolOffsetSensorAlive state.
void setup_tool_offset_sensor_node(Application &app, MockPresentation &mock, NodeId node_id, uint32_t unique_id, uint32_t &time) {
    const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };

    // node asks for PNP allocation, it is given a node id
    {
        app.receive_pnp_allocation(make_unique_id(unique_id));
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.pnp_allocation.size() == mock_before.pnp_allocation.size() + 1);
        REQUIRE(mock.pnp_allocation.back().node_id == node_id);
    }
    // node sends a heartbeat from its firmware, it receives a node info request
    {
        app.receive_node_heartbeat(node_id, make_timepoint(time), healthy_firmware);
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.node_get_info_request.size() == mock_before.node_get_info_request.size() + 1);
        REQUIRE(mock.node_get_info_request.back() == node_id);
    }
    // node responds with node info, it receives a hash request (verification is required on first boot)
    {
        app.receive_node_get_info_response(node_id, tool_offset_sensor_node_name);
        const auto mock_before = run(app, mock, make_timepoint(time++));
        REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
        REQUIRE(mock.node_execute_command_request.back().command == Command::get_app_salted_hash);
        REQUIRE(app.request().hash_request == cyphal::FirmwareFile::firmware_tool_offset_sensor);
    }
    // node and motherboard respond with a matching hash, the node becomes alive
    {
        const auto digest = generate_random_digest();
        const auto received = app.receive_digest(cyphal::FirmwareFile::firmware_tool_offset_sensor, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest);
        REQUIRE(received);
        app.receive_node_execute_command_response(node_id, 0, digest);
        run(app, mock, make_timepoint(time++));
        REQUIRE(app.request().hash_request == cyphal::FirmwareFile::none);
    }
}

SCENARIO("heart beats") {
    GIVEN("initial application state") {
        MockPresentation mock;
        Application app;

        THEN("there are no heartbeats") {
            CHECK(mock.heartbeat_count == 0);
        }

        WHEN("application runs") {
            run(app, mock, make_timepoint(1200));

            THEN("heartbeat is transmitted") {
                CHECK(mock.heartbeat_count == 1);
                CHECK(mock.last_uptime == 1);
                CHECK(mock.last_heartbeat_healthy);

                AND_WHEN("not enough time has passed") {
                    run(app, mock, make_timepoint(1400));

                    THEN("another heartbeat is not transmitted") {
                        CHECK(mock.heartbeat_count == 1);
                        CHECK(mock.last_uptime == 1);
                    }
                }

                AND_WHEN("enough time has passed") {
                    run(app, mock, make_timepoint(4400));

                    THEN("another heartbeat is transmitted") {
                        CHECK(mock.heartbeat_count == 2);
                        CHECK(mock.last_uptime == 4);
                        CHECK(mock.last_heartbeat_healthy);
                    }
                }

                AND_WHEN("master activity stops") {
                    run_without_master_activity(app, mock, make_timepoint(4400));

                    THEN("heartbeat marked as warning is transmitted") {
                        CHECK(mock.heartbeat_count == 2);
                        CHECK(mock.last_uptime == 4);
                        CHECK_FALSE(mock.last_heartbeat_healthy);
                    }
                }
            }
        }
    }
}

SCENARIO("basic allocation") {
    GIVEN("initial application state") {
        MockPresentation mock;
        Application app;

        THEN("there are no allocation responses") {
            REQUIRE(mock.pnp_allocation.empty());
        }

        WHEN("anonymous node with some uuid requests node id") {
            app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
            run(app, mock, make_timepoint(0));

            THEN("node id is allocated for that uuid") {
                REQUIRE(mock.pnp_allocation.size() == 1);
                CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
                CHECK(mock.pnp_allocation.back().node_id == NodeId { 1 });
            }

            THEN("allocated node id is not 0") {
                REQUIRE(mock.pnp_allocation.size() == 1);
                CHECK(mock.pnp_allocation.back().node_id != NodeId { 0 });
            }

            AND_WHEN("the same uuid requests some node id") {
                app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
                run(app, mock, make_timepoint(1));

                THEN("same node id is allocated") {
                    REQUIRE(mock.pnp_allocation.size() == 2);
                    CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
                    CHECK(mock.pnp_allocation.back().node_id == NodeId { 1 });
                }
            }

            AND_WHEN("different uuid requests some node id") {
                app.receive_pnp_allocation(make_unique_id(0xcafebabe));
                run(app, mock, make_timepoint(1));

                THEN("it should allocate different node id") {
                    REQUIRE(mock.pnp_allocation.size() == 2);
                    CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xcafebabe));
                    CHECK(mock.pnp_allocation.back().node_id == NodeId { 2 });
                }
            }
        }
    }
}

SCENARIO("full allocation table") {
    GIVEN("maximum number of different allocation requests") {
        MockPresentation mock;
        Application app;
        const int max = 15;
        for (int i = 1; i <= max; ++i) {
            app.receive_pnp_allocation(make_unique_id(i * i));
        }
        run(app, mock, make_timepoint(0));

        THEN("there have been that many allocation responses") {
            REQUIRE(mock.pnp_allocation.size() == max);
            CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(max * max));
            CHECK(mock.pnp_allocation.back().node_id == NodeId { max });
        }

        WHEN("another different allocation is requested") {
            app.receive_pnp_allocation(make_unique_id(7));
            run(app, mock, make_timepoint(1));

            THEN("allocation response is not sent") {
                REQUIRE(mock.pnp_allocation.size() == max);
            }
        }
    }
}

SCENARIO("heartbeat response") {
    GIVEN("node id was requested") {
        MockPresentation mock;
        Application app;
        app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
        run(app, mock, make_timepoint(0));

        THEN("node id was allocated") {
            REQUIRE(mock.pnp_allocation.size() == 1);
            CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
            CHECK(mock.pnp_allocation.back().node_id == NodeId { 1 });
        }

        WHEN("heartbeat comes from that node id") {
            app.receive_node_heartbeat(NodeId { 1 }, make_timepoint(1), { Health::nominal, Mode::operational, 0 });
            run(app, mock, make_timepoint(1));

            THEN("node details are requested") {
                REQUIRE(mock.node_get_info_request.size() == 1);
                CHECK(mock.node_get_info_request.back() == NodeId { 1 });
            }

            AND_WHEN("another heartbeat comes") {
                app.receive_node_heartbeat(NodeId { 1 }, make_timepoint(2), { Health::nominal, Mode::operational, 0 });
                run(app, mock, make_timepoint(2));

                THEN("node info is not requested again") {
                    REQUIRE(mock.node_get_info_request.size() == 1);
                }
            }
        }

        WHEN("heartbeat comes from a different node id") {
            app.receive_node_heartbeat(NodeId { 2 }, make_timepoint(1), { Health::nominal, Mode::operational, 0 });
            run(app, mock, make_timepoint(1));

            THEN("heartbeat is ignored") {
                CHECK(mock.node_get_info_request.empty());
            }
        }

        WHEN("heartbeat comes from a very large node id") {
            app.receive_node_heartbeat(NodeId { 64 }, make_timepoint(1), { Health::nominal, Mode::operational, 0 });
            run(app, mock, make_timepoint(1));

            THEN("heartbeat is ignored") {
                CHECK(mock.node_get_info_request.empty());
            }
        }
    }
}

SCENARIO("happy case") {
    const auto lingering_bootloader = Heartbeat { Health::advisory, Mode::software_update, 0 };
    // Means no app, only the bootloader
    const auto empty_bootloader = Heartbeat { Health::warning, Mode::software_update, 0 };
    const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };

    const auto node_id = NodeId { 1 };

    const auto start_app = ExecuteCommandRequest { node_id, Command::start_app };
    const auto restart = ExecuteCommandRequest { node_id, Command::restart };
    static const std::string_view dummy_parameter_sv = "/path/to/fw";
    static const std::vector<std::byte> dummy_parameter { (std::byte *)dummy_parameter_sv.begin(), (std::byte *)dummy_parameter_sv.end() };
    const auto software_update = ExecuteCommandRequest { node_id, Command::software_update, dummy_parameter };

    GIVEN("node id was requested") {
        MockPresentation mock;
        Application app;

        auto test_flashing = [&]() {
            THEN("software update command is executed") {
                const auto now = make_timepoint(4000);
                const auto mock_before = run(app, mock, now);
                REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                CHECK(mock.node_execute_command_request.back() == software_update);

                // The bootloader wants a bit of a file.
                uint8_t id = 12;
                app.receive_file_read_request(node_id, now, TransferId { id }, 0);
                run(app, mock, make_timepoint(4001));
                auto modbus_request = app.request();
                CHECK(modbus_request.flash_request == cyphal::FirmwareFile::firmware_ac_controller);
                CHECK(modbus_request.offset == 0);

                std::optional<size_t> last_sent = std::nullopt;
                uint32_t time = 4002;

                std::vector<uint8_t> data;

                while (data.size() < mock_firmware.size()) {
                    INFO("Timepoint " << time);
                    const size_t rest = mock_firmware.size() - modbus_request.offset;
                    constexpr size_t max_chunk = sizeof(xbuddy_extension::modbus::Chunk::data);
                    const size_t size = std::min(rest, max_chunk);
                    bool progress = false;
                    if (last_sent != modbus_request.offset) {
                        last_sent = modbus_request.offset;
                        CHECK(app.receive_chunk(mock_firmware.data() + modbus_request.offset, size, modbus_request.offset + size == mock_firmware.size(), static_cast<uint16_t>(modbus_request.flash_request), modbus_request.offset));
                        progress = true;
                    }

                    size_t old_offset = modbus_request.offset;

                    const auto now = make_timepoint(time);
                    run(app, mock, now);

                    // Note: Node sends only one request, so we send only one response.
                    REQUIRE(mock.file_responses.size() <= 1);
                    if (!mock.file_responses.empty()) {
                        const auto &response = mock.file_responses[0];
                        CHECK(response.remote_node_id == node_id);
                        CHECK(response.transfer_id == TransferId { id });

                        for (auto byte : response.data) {
                            data.push_back(static_cast<uint8_t>(byte));
                        }

                        mock.file_responses.clear();

                        if (data.size() < mock_firmware.size()) {
                            id++;
                            app.receive_file_read_request(node_id, now, TransferId { id }, data.size());
                        }

                        progress = true;
                    }

                    REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                    modbus_request = app.request();
                    if (old_offset != modbus_request.offset) {
                        progress = true;
                    }

                    REQUIRE(progress);

                    time++;
                }

                CHECK(data == mock_firmware);
            }

            WHEN("Application boots and requests ID again") {
                app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
                run(app, mock, make_timepoint(7000));

                THEN("No new ID is assigned") {
                    CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
                    CHECK(mock.pnp_allocation.back().node_id == node_id);
                }
            }
        };

        app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
        run(app, mock, make_timepoint(0));

        THEN("node id was allocated") {
            REQUIRE(mock.pnp_allocation.size() == 1);
            CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
            CHECK(mock.pnp_allocation.back().node_id == node_id);
        }

        WHEN("node sends heartbeat from lingering bootloader") {
            app.receive_node_heartbeat(node_id, make_timepoint(1), lingering_bootloader);
            run(app, mock, make_timepoint(1));

            THEN("node info is requested") {
                REQUIRE(mock.node_get_info_request.size() == 1);
                CHECK(mock.node_get_info_request.back() == node_id);
            }

            AND_WHEN("unknown node info is received") {
                std::string_view name { "com.dot" };
                app.receive_node_get_info_response(node_id, as_bytes(std::span { name.data(), name.size() }));
                run(app, mock, make_timepoint(2));

                THEN("no command is executed") {
                    CHECK(mock.node_execute_command_request.size() == 0);
                }
            }

            AND_WHEN("known node info is received") {
                app.receive_node_get_info_response(node_id, known_node_name);
                run(app, mock, make_timepoint(2));

                THEN("start_app command is executed") {
                    REQUIRE(mock.node_execute_command_request.size() == 1);
                    CHECK(mock.node_execute_command_request.back() == start_app);
                    const auto modbus_request = app.request();
                    CHECK(modbus_request.hash_request == cyphal::FirmwareFile::none);

                    AND_WHEN("nothing happens right away") {
                        const auto mock_before = run(app, mock, make_timepoint(3));

                        THEN("nothing happens") {
                            CHECK(mock == mock_before);
                        }
                    }

                    AND_WHEN("nothing happens for a long time") {
                        const auto mock_before = run(app, mock, make_timepoint(1035));

                        THEN("start_app command is retried") {
                            REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                            CHECK(mock.node_execute_command_request.back() == start_app);
                        }
                    }

                    AND_WHEN("node sends heartbeat from firmware") {
                        app.receive_node_heartbeat(node_id, make_timepoint(3), healthy_firmware);
                        const auto mock_before = run(app, mock, make_timepoint(3));

                        THEN("application requests file hash from parent system") {
                            CHECK(mock == mock_before);
                            const auto modbus_request = app.request();
                            CHECK(modbus_request.hash_request == cyphal::FirmwareFile::firmware_ac_controller);

                            AND_WHEN("parent system sends file hash to application") {
                                std::byte digest[32] = {};
                                (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest);
                                app.receive_node_heartbeat(node_id, make_timepoint(1000), healthy_firmware);
                                app.receive_node_heartbeat(node_id, make_timepoint(2000), healthy_firmware);
                                const auto mock_before = run(app, mock, make_timepoint(2500));

                                THEN("application requests hash from node") {
                                    REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                                    CHECK(mock.node_execute_command_request.back() == make_salted_hash_request(node_id, app.request().hash_salt));

                                    AND_WHEN("node responds with the matching hash") {
                                        app.receive_node_execute_command_response(node_id, 0, digest);
                                        const auto mock_before = run(app, mock, make_timepoint(3000));

                                        THEN("what happens?") {
                                            CHECK(mock == mock_before);
                                        }
                                    }

                                    AND_WHEN("node responds with mismatching hash") {
                                        std::byte wrong_digest[32] = {};
                                        wrong_digest[6] = (std::byte)'K';
                                        app.receive_node_execute_command_response(node_id, 0, wrong_digest);
                                        const auto mock_before = run(app, mock, make_timepoint(3000));

                                        THEN("restart command is executed") {
                                            REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                                            CHECK(mock.node_execute_command_request.back() == restart);

                                            AND_WHEN("new heartbeat from bootloader is received") {
                                                app.receive_node_heartbeat(node_id, make_timepoint(3001), lingering_bootloader);
                                                test_flashing();
                                            }

                                            AND_WHEN("no heartbeat is sent") {
                                                const auto mock_before = run(app, mock, make_timepoint(4000));
                                                THEN("we send no other command") {
                                                    CHECK(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size());
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        WHEN("Node sends heartbeat from empty bootloader") {
            app.receive_node_heartbeat(node_id, make_timepoint(1), empty_bootloader);
            run(app, mock, make_timepoint(1));

            THEN("node info is requested") {
                REQUIRE(mock.node_get_info_request.size() == 1);
                CHECK(mock.node_get_info_request.back() == node_id);

                AND_WHEN("known node info is received") {
                    app.receive_node_get_info_response(node_id, known_node_name);
                    run(app, mock, make_timepoint(2));

                    test_flashing();
                }
            }
        }

        WHEN("bootloader found valid app, started the app and the app sent a heartbeat") {
            app.receive_node_heartbeat(node_id, make_timepoint(1), healthy_firmware);
            run(app, mock, make_timepoint(1));

            THEN("node info is requested") {
                REQUIRE(mock.node_get_info_request.size() == 1);
                CHECK(mock.node_get_info_request.back() == node_id);

                AND_WHEN("known node info is received") {
                    app.receive_node_get_info_response(node_id, known_node_name);
                    const auto mock_before = run(app, mock, make_timepoint(2));

                    THEN("application requests hash from node and parent system") {
                        REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                        CHECK(mock.node_execute_command_request.back() == make_salted_hash_request(node_id, app.request().hash_salt));
                        const auto modbus_request = app.request();
                        CHECK(modbus_request.hash_request == cyphal::FirmwareFile::firmware_ac_controller);

                        AND_WHEN("they respond with matching hash") {
                            const auto digest = generate_random_digest();
                            (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest);
                            app.receive_node_execute_command_response(node_id, 0, digest);
                            const auto mock_before = run(app, mock, make_timepoint(3));

                            THEN("nothing happens anymore") {
                                CHECK(mock == mock_before);
                            }
                        }

                        AND_WHEN("they respond with mismatching hash") {
                            const auto digest1 = generate_random_digest();
                            const auto digest2 = generate_random_digest();
                            (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest1);
                            app.receive_node_execute_command_response(node_id, 0, digest2);
                            const auto mock_before = run(app, mock, make_timepoint(3));

                            THEN("restart command is executed") {
                                REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                                CHECK(mock.node_execute_command_request.back() == restart);

                                AND_WHEN("node id was requested") {
                                    app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
                                    const auto mock_before = run(app, mock, make_timepoint(4));

                                    THEN("node id was allocated") {
                                        REQUIRE(mock.pnp_allocation.size() == mock_before.pnp_allocation.size() + 1);
                                        CHECK(mock.pnp_allocation.back().unique_id == make_unique_id(0xdeadbeef));
                                        CHECK(mock.pnp_allocation.back().node_id == node_id);

                                        AND_WHEN("node sends heartbeat from lingering bootloader") {
                                            app.receive_node_heartbeat(node_id, make_timepoint(5), lingering_bootloader);
                                            const auto mock_before = run(app, mock, make_timepoint(5));

                                            test_flashing();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                AND_WHEN("unknown node info is received") {
                    app.receive_node_get_info_response(node_id, unknown_node_name);
                    run(app, mock, make_timepoint(2));

                    // TODO BFW-7918
                    // Check that no communication happens with that node...
                }
            }
        }
    }
}

SCENARIO("motherboard signals firmware unavailable") {
    const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };
    const auto node_id = NodeId { 1 };

    GIVEN("node is verifying its firmware") {
        MockPresentation mock;
        Application app;

        app.receive_pnp_allocation(make_unique_id(0xdeadbeef));
        run(app, mock, make_timepoint(0));
        app.receive_node_heartbeat(node_id, make_timepoint(1), healthy_firmware);
        run(app, mock, make_timepoint(1));
        app.receive_node_get_info_response(node_id, known_node_name);
        run(app, mock, make_timepoint(2));

        const auto initial_request = app.request();
        REQUIRE(initial_request.hash_request == cyphal::FirmwareFile::firmware_ac_controller);

        WHEN("motherboard reports the file as unavailable") {
            (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, initial_request.hash_salt, xbuddy_extension::DigestStatus::unavailable, std::array<std::byte, 32> {});
            const auto mock_before = run(app, mock, make_timepoint(3));

            THEN("hash request is cleared") {
                CHECK(app.request().hash_request == cyphal::FirmwareFile::none);
            }

            AND_WHEN("more time passes") {
                const auto commands_before_wait = mock.node_execute_command_request.size();
                const auto info_requests_before_wait = mock.node_get_info_request.size();
                run(app, mock, make_timepoint(5000));

                THEN("no further commands are sent and no new hash is requested") {
                    CHECK(mock.node_execute_command_request.size() == commands_before_wait);
                    CHECK(mock.node_get_info_request.size() == info_requests_before_wait);
                    CHECK(app.request().hash_request == cyphal::FirmwareFile::none);
                }
            }
        }

        WHEN("an unavailable response arrives with a mismatching file/salt") {
            (void)app.receive_digest(cyphal::FirmwareFile::firmware_anfc, initial_request.hash_salt, xbuddy_extension::DigestStatus::unavailable, std::array<std::byte, 32> {});
            run(app, mock, make_timepoint(3));

            THEN("the hash request is still pending") {
                CHECK(app.request().hash_request == cyphal::FirmwareFile::firmware_ac_controller);
            }

            AND_WHEN("a matching digest later arrives and the node responds") {
                const auto digest = generate_random_digest();
                (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, initial_request.hash_salt, xbuddy_extension::DigestStatus::ok, digest);
                app.receive_node_execute_command_response(node_id, 0, digest);
                run(app, mock, make_timepoint(4));

                THEN("verification completes normally (request is cleared)") {
                    CHECK(app.request().hash_request == cyphal::FirmwareFile::none);
                }
            }
        }

        WHEN("motherboard reports retry") {
            const auto garbage = generate_random_digest();
            (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, initial_request.hash_salt, xbuddy_extension::DigestStatus::retry, garbage);
            run(app, mock, make_timepoint(3));

            THEN("the hash request stays pending") {
                CHECK(app.request().hash_request == cyphal::FirmwareFile::firmware_ac_controller);
            }

            AND_WHEN("motherboard eventually responds with ok and the node matches") {
                const auto digest = generate_random_digest();
                (void)app.receive_digest(cyphal::FirmwareFile::firmware_ac_controller, initial_request.hash_salt, xbuddy_extension::DigestStatus::ok, digest);
                app.receive_node_execute_command_response(node_id, 0, digest);
                run(app, mock, make_timepoint(4));

                THEN("verification completes (request is cleared)") {
                    CHECK(app.request().hash_request == cyphal::FirmwareFile::none);
                }
            }
        }
    }
}

SCENARIO("NFC node allocation") {
    const auto node_id = NodeId { 1 };

    GIVEN("ready NFC node") {
        MockPresentation mock;
        Application app;
        uint32_t time = 0;
        setup_nfc_node(app, mock, node_id, 0x11111111, time);

        WHEN("request is queued on the virtual device") {
            anfc::modbus::Request request = {};
            request.size = 3;
            request.data[0] = 0xAA;
            request.data[1] = 0xBB;
            auto &nfc0 = app.get_nfc(anfc::Device::anfc0);
            CHECK(nfc0.queue(request));
            const auto mock_before = run(app, mock, make_timepoint(4));

            THEN("the same request is transmitted to the node") {
                REQUIRE(mock.nfc_requests.size() == mock_before.nfc_requests.size() + 1);
                CHECK(mock.nfc_requests.back().node_id == node_id);
                CHECK(mock.nfc_requests.back().data == as_bytes(request));
            }
        }

        WHEN("event is received from the node") {
            const auto event_data = std::array { std::byte { 0xDE }, std::byte { 0xAD } };
            app.receive_nfc_event(node_id, event_data);
            anfc::modbus::Event event;
            auto &nfc0 = app.get_nfc(anfc::Device::anfc0);
            nfc0.consume(event);

            THEN("the same event can be consumed on the virtual device") {
                CHECK(as_bytes(event) == as_bytes(event_data));
            }
        }
    }
}

SCENARIO("multiple NFC node allocation") {
    const auto node_id1 = NodeId { 1 };
    const auto node_id2 = NodeId { 2 };

    GIVEN("two ready NFC nodes") {
        MockPresentation mock;
        Application app;
        uint32_t time = 0;
        setup_nfc_node(app, mock, node_id1, 0x11111111, time);
        setup_nfc_node(app, mock, node_id2, 0x22222222, time);

        THEN("both nodes are allocated to different devices") {
            auto &nfc0 = app.get_nfc(anfc::Device::anfc0);
            auto &nfc1 = app.get_nfc(anfc::Device::anfc1);

            anfc::modbus::Request request0 = {};
            request0.size = 1;
            request0.data[0] = 0x01;
            CHECK(nfc0.queue(request0));

            anfc::modbus::Request request1 = {};
            request1.size = 1;
            request1.data[0] = 0x02;
            CHECK(nfc1.queue(request1));

            run(app, mock, make_timepoint(time));
            REQUIRE(mock.nfc_requests.size() == 2);

            const auto expected0 = MockPresentation::NfcCommand { node_id1, as_bytes(request0) };
            const auto expected1 = MockPresentation::NfcCommand { node_id2, as_bytes(request1) };
            CHECK(std::ranges::find(mock.nfc_requests, expected0) != mock.nfc_requests.end());
            CHECK(std::ranges::find(mock.nfc_requests, expected1) != mock.nfc_requests.end());
        }

        WHEN("a third NFC node tries to allocate") {
            const auto node_id3 = NodeId { 3 };
            setup_nfc_node(app, mock, node_id3, 0x33333333, time);

            THEN("third node goes to Inert state (no NFC device available)") {
                // The node should not cause any NFC communication
                // We can test this by verifying events sent to node3 are ignored
                const auto event_data = std::array { std::byte { 0xFF } };
                app.receive_nfc_event(node_id3, event_data);

                // Neither anfc0 nor anfc1 should have this event
                anfc::modbus::Event event0, event1;
                app.get_nfc(anfc::Device::anfc0).consume(event0);
                app.get_nfc(anfc::Device::anfc1).consume(event1);
                CHECK(event0.size == 0);
                CHECK(event1.size == 0);
            }
        }
    }
}

SCENARIO("NFC node event routing") {
    const auto node_id1 = NodeId { 1 };
    const auto node_id2 = NodeId { 2 };

    GIVEN("two active NFC nodes") {
        MockPresentation mock;
        Application app;
        uint32_t time = 0;
        setup_nfc_node(app, mock, node_id1, 0x11111111, time);
        setup_nfc_node(app, mock, node_id2, 0x22222222, time);

        WHEN("different events are received from both nodes") {
            const auto event1_data = std::array { std::byte { 0x11 }, std::byte { 0x22 } };
            const auto event2_data = std::array { std::byte { 0x33 }, std::byte { 0x44 }, std::byte { 0x55 } };

            app.receive_nfc_event(node_id1, event1_data);
            app.receive_nfc_event(node_id2, event2_data);

            THEN("events are routed to correct devices") {
                anfc::modbus::Event event0;
                anfc::modbus::Event event1;
                app.get_nfc(anfc::Device::anfc0).consume(event0);
                app.get_nfc(anfc::Device::anfc1).consume(event1);

                CHECK(as_bytes(event0) == as_bytes(event1_data));
                CHECK(as_bytes(event1) == as_bytes(event2_data));
            }
        }

        WHEN("event is received from unallocated node") {
            const auto event_data = std::array { std::byte { 0xAA } };
            app.receive_nfc_event(NodeId { 10 }, event_data);

            THEN("event is ignored by both nodes") {
                anfc::modbus::Event event0;
                anfc::modbus::Event event1;
                app.get_nfc(anfc::Device::anfc0).consume(event0);
                app.get_nfc(anfc::Device::anfc1).consume(event1);

                CHECK(event0.size == 0);
                CHECK(event1.size == 0);
            }
        }
    }
}

SCENARIO("tool offset sensor verifies its firmware on first boot") {
    const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };
    const auto node_id = NodeId { 1 };

    GIVEN("a freshly allocated tool offset sensor reporting from its firmware") {
        MockPresentation mock;
        Application app;
        uint32_t time = 0;

        app.receive_pnp_allocation(make_unique_id(0x705));
        run(app, mock, make_timepoint(time++));
        app.receive_node_heartbeat(node_id, make_timepoint(time), healthy_firmware);
        run(app, mock, make_timepoint(time++));

        WHEN("it identifies itself as a tool offset sensor") {
            app.receive_node_get_info_response(node_id, tool_offset_sensor_node_name);
            const auto mock_before = run(app, mock, make_timepoint(time++));

            THEN("its firmware hash is requested from both the node and the motherboard") {
                // A node that has never been verified must go through the full
                // hash verification. The fast reset-recovery path must NOT be
                // taken on first boot - this guards against the verification
                // ever being skipped for a node we haven't checked yet.
                REQUIRE(mock.node_execute_command_request.size() == mock_before.node_execute_command_request.size() + 1);
                CHECK(mock.node_execute_command_request.back().command == Command::get_app_salted_hash);
                CHECK(app.request().hash_request == cyphal::FirmwareFile::firmware_tool_offset_sensor);
            }
        }
    }
}

TEST_CASE("tool offset sensor recovers from a board reset") {
    const auto healthy_firmware = Heartbeat { Health::nominal, Mode::operational, 0 };
    const auto bootloader = Heartbeat { Health::advisory, Mode::software_update, 0 };
    const auto node_id = NodeId { 1 };

    // The non-default configuration we will apply to the node before it resets, and expect to be re-sent after it comes back up.
    const auto enabled_config = tool_offset_sensor::Config { .ch0_enabled = true, .ch1_enabled = false };

    MockPresentation mock;
    Application app;
    uint32_t time = 0;
    setup_tool_offset_sensor_node(app, mock, node_id, 0xdeadbeef, time);

    app.receive(enabled_config);
    run(app, mock, make_timepoint(time++));
    REQUIRE(mock.tool_offset_sensor_config_requests.back().config == enabled_config);

    const auto commands_before = mock.node_execute_command_request.size();
    const auto info_requests_before = mock.node_get_info_request.size();
    const auto config_requests_before = mock.tool_offset_sensor_config_requests.size();

    // The reset is observed as a software_update-mode heartbeat
    app.receive_node_heartbeat(node_id, make_timepoint(time), bootloader);
    run(app, mock, make_timepoint(time++));

    // The second heartbeat triggers step function again and performs start app command
    app.receive_node_heartbeat(node_id, make_timepoint(time), bootloader);
    run(app, mock, make_timepoint(time++));

    // The XBE issues start_app once the bootloader reports BootCancelled
    const auto start_app = ExecuteCommandRequest { node_id, Command::start_app };
    REQUIRE(mock.node_execute_command_request.size() == commands_before + 1);
    CHECK(mock.node_execute_command_request.back() == start_app);

    // The node acknowledges start_app and boots its application.
    app.receive_node_execute_command_response(node_id, 0, {});

    // Now the app boots and reports heartbeat from healthy firmware
    app.receive_node_heartbeat(node_id, make_timepoint(time), healthy_firmware);
    run(app, mock, make_timepoint(time++));

    // The firmware is fully re-verified after the reset: the XBE requests a
    // fresh salted hash both from the node and from the motherboard. No fresh
    // GetInfo is issued though - the node identity is remembered across the
    // reset, so start_app and get_app_salted_hash are the only two commands.
    CHECK(mock.node_get_info_request.size() == info_requests_before);
    REQUIRE(mock.node_execute_command_request.size() == commands_before + 2);
    CHECK(mock.node_execute_command_request.back().command == Command::get_app_salted_hash);
    REQUIRE(app.request().hash_request == cyphal::FirmwareFile::firmware_tool_offset_sensor);

    const auto digest = generate_random_digest();
    const auto received = app.receive_digest(cyphal::FirmwareFile::firmware_tool_offset_sensor, app.request().hash_salt, xbuddy_extension::DigestStatus::ok, digest);
    REQUIRE(received);
    app.receive_node_execute_command_response(node_id, 0, digest);
    run(app, mock, make_timepoint(time++));

    // Verification succeeded, the request slot is released again.
    CHECK(app.request().hash_request == cyphal::FirmwareFile::none);

    // Once the node is alive again its configuration is re-applied.
    REQUIRE(mock.tool_offset_sensor_config_requests.size() == config_requests_before + 1);
    CHECK(mock.tool_offset_sensor_config_requests.back().node_id == node_id);
    CHECK(mock.tool_offset_sensor_config_requests.back().config == enabled_config);
}
