#include <cyphal_nfc_node.hpp>

#include "cyphal_presentation.hpp"
#include <tool_offset_sensor/types.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <modbus/traits.hpp>
#include <utils/byte_utils.hpp>
#include <vector>

using cyphal::NfcNode;
using cyphal::NodeId;
using cyphal::Presentation;
using cyphal::Severity;
using cyphal::TransferId;

class MockPresentation final : public Presentation {
public:
    bool fail = false;

    std::vector<std::vector<std::byte>> accept_event;
    std::vector<std::vector<std::byte>> request;

    void transmit_heartbeat(uint32_t, bool) override {}
    void transmit_pnp_allocation(const cyphal::UniqueId &, NodeId) override {}
    void transmit_diagnostic_record(Severity, const char *) override {}
    void transmit_node_get_info_request(NodeId) override {}
    void transmit_node_execute_command_request(NodeId, cyphal::Command, Bytes) override {}
    void transmit_file_read_response(NodeId, TransferId, WritableBytes) override {}
    void transmit_ac_controller_config_request(NodeId, const ac_controller::Config &) override {}
    void transmit_ac_controller_leds_config_request([[maybe_unused]] NodeId, [[maybe_unused]] const ac_controller::LedConfig &) override {}
    void transmit_tool_offset_sensor_config_request(NodeId, const tool_offset_sensor::Config &) override {}

    bool transmit_nfc_command_request(NodeId, Bytes data) override {
        if (fail) {
            return false;
        } else {
            request.emplace_back(data.begin(), data.end());
            return true;
        }
    }

    bool transmit_nfc_command_accept_event(NodeId, Bytes data) override {
        if (fail) {
            return false;
        } else {
            accept_event.emplace_back(data.begin(), data.end());
            return true;
        }
    }
};

anfc::modbus::Request make_request(Bytes bytes) {
    anfc::modbus::Request request = {};
    request.size = static_cast<uint16_t>(bytes.size());
    std::memcpy(request.data.data(), bytes.data(), bytes.size());
    return request;
}

anfc::modbus::AcceptEvent make_accept_event(Bytes bytes) {
    anfc::modbus::AcceptEvent accept_event = {};
    accept_event.size = static_cast<uint16_t>(bytes.size());
    std::memcpy(accept_event.data.data(), bytes.data(), bytes.size());
    return accept_event;
}

std::vector<std::byte> as_bytes(const anfc::modbus::Event &event) {
    auto span = modbus::payload(event);
    return { span.begin(), span.end() };
}

template <size_t N>
std::vector<std::byte> as_bytes(const std::array<std::byte, N> &arr) {
    return { arr.begin(), arr.end() };
}

constexpr auto node_id = NodeId { 1 };

SCENARIO("initialization") {
    GIVEN("fresh NfcNode") {
        NfcNode node;
        MockPresentation mock;

        THEN("nothing is transmitted") {
            const auto mock_before = mock;
            const bool progress = node.step(mock, node_id);

            CHECK(!progress);
            CHECK(mock == mock_before);
        }

        THEN("nothing is received") {
            anfc::modbus::Event event;
            node.consume(event);

            CHECK(event.size == 0);
        }
    }
}

SCENARIO("basic AcceptEvent handling") {
    GIVEN("fresh NfcNode") {
        NfcNode node;
        MockPresentation mock;

        WHEN("AcceptEvent is queued") {
            const auto payload = std::vector { std::byte { 0x42 } };
            const auto accept_event = make_accept_event(payload);
            const bool queued = node.queue(accept_event);

            THEN("it succeeds") {
                CHECK(queued);
            }

            AND_WHEN("step() is called") {
                const auto mock_before = mock;
                const bool progress = node.step(mock, node_id);

                THEN("the AcceptEvent is transmitted") {
                    CHECK(progress);
                    REQUIRE(mock.accept_event.size() == mock_before.accept_event.size() + 1);
                    CHECK(mock.accept_event.back() == payload);
                    CHECK(mock.request.size() == mock_before.request.size());
                }

                AND_WHEN("step() is called again") {
                    const auto mock_before = mock;
                    const bool progress = node.step(mock, node_id);

                    THEN("nothing is transmitted") {
                        CHECK(!progress);
                        CHECK(mock == mock_before);
                    }
                }

                AND_WHEN("another AcceptEvent is queued") {
                    const auto payload = std::vector { std::byte { 0x43 } };
                    const auto accept_event = make_accept_event(payload);
                    const bool queued = node.queue(accept_event);

                    THEN("it succeeds") {
                        CHECK(queued);
                    }
                }
            }

            AND_WHEN("another AcceptEvent is queued") {
                const auto payload = std::vector { std::byte { 0x43 } };
                const auto accept_event = make_accept_event(payload);
                const bool queued = node.queue(accept_event);

                THEN("it fails") {
                    CHECK(!queued);
                }
            }
        }
    }
}

SCENARIO("basic Request handling") {
    GIVEN("fresh NfcNode") {
        NfcNode node;
        MockPresentation mock;

        WHEN("Request is queued") {
            const auto payload = std::vector { std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x03 } };
            const auto request = make_request(payload);
            const bool queued = node.queue(request);

            THEN("it succeeds") {
                CHECK(queued);
            }

            AND_WHEN("step() is called") {
                const auto mock_before = mock;
                const bool progress = node.step(mock, node_id);

                THEN("the Request is transmitted") {
                    CHECK(progress);
                    CHECK(mock.accept_event.size() == mock_before.accept_event.size());
                    REQUIRE(mock.request.size() == mock_before.request.size() + 1);
                    CHECK(mock.request.back() == payload);
                }

                AND_WHEN("step() is called again") {
                    const auto mock_before = mock;
                    const bool progress = node.step(mock, node_id);

                    THEN("nothing is transmitted") {
                        CHECK(!progress);
                        CHECK(mock == mock_before);
                    }
                }

                AND_WHEN("another Request is queued") {
                    const auto payload = std::vector { std::byte { 0x04 } };
                    const auto request = make_request(payload);
                    const bool queued = node.queue(request);

                    THEN("it succeeds") {
                        CHECK(queued);
                    }
                }
            }

            AND_WHEN("another Request is queued") {
                const auto payload = std::vector { std::byte { 0x04 } };
                const auto request = make_request(payload);
                const bool queued = node.queue(request);

                THEN("it fails") {
                    CHECK(!queued);
                }
            }
        }
    }
}

SCENARIO("AcceptEvent has priority over Request") {
    GIVEN("NfcNode with both pending AcceptEvent and Request") {
        NfcNode node;
        MockPresentation mock;

        const auto accept_event_payload = std::vector { std::byte { 0x01 } };
        const auto accept_event = make_accept_event(accept_event_payload);
        const auto request_payload = std::vector { std::byte { 0x02 }, std::byte { 0x03 } };
        const auto request = make_request(request_payload);
        CHECK(node.queue(accept_event));
        CHECK(node.queue(request));

        WHEN("step() is called") {
            const auto mock_before = mock;
            const bool progress = node.step(mock, node_id);

            THEN("only AcceptEvent is transmitted") {
                CHECK(progress);
                REQUIRE(mock.accept_event.size() == mock_before.accept_event.size() + 1);
                CHECK(mock.accept_event.back() == accept_event_payload);
                CHECK(mock.request.size() == mock_before.request.size());
            }

            AND_WHEN("step() is called again") {
                const auto mock_before = mock;
                const bool progress = node.step(mock, node_id);

                THEN("Request is transmitted") {
                    CHECK(progress);
                    CHECK(mock.accept_event.size() == mock_before.accept_event.size());
                    REQUIRE(mock.request.size() == mock_before.request.size() + 1);
                    CHECK(mock.request.back() == request_payload);
                }
            }
        }
    }
}

SCENARIO("presentation layer failure") {
    GIVEN("NfcNode with pending AcceptEvent") {
        NfcNode node;
        MockPresentation mock;

        const auto payload = std::vector { std::byte { 0xCC }, std::byte { 0xDD } };
        const auto accept_event = make_accept_event(payload);
        CHECK(node.queue(accept_event));

        WHEN("step() is called and presentation fails") {
            mock.fail = true;
            const auto mock_before = mock;
            const bool progress = node.step(mock, node_id);

            THEN("nothing is transmitted") {
                CHECK(!progress);
                CHECK(mock == mock_before);
            }

            AND_WHEN("step() is called again and presentation succeeds") {
                mock.fail = false;
                const auto mock_before = mock;
                const bool progress = node.step(mock, node_id);

                THEN("AcceptEvent is transmitted") {
                    CHECK(progress);
                    REQUIRE(mock.accept_event.size() == mock_before.accept_event.size() + 1);
                    CHECK(mock.accept_event.back() == payload);
                    CHECK(mock.request.size() == mock_before.request.size());
                }
            }
        }
    }

    GIVEN("NfcNode with pending Request") {
        NfcNode node;
        MockPresentation mock;

        const auto payload = std::vector { std::byte { 0xAA }, std::byte { 0xBB } };
        const auto request = make_request(payload);
        CHECK(node.queue(request));

        WHEN("step() is called and presentation fails") {
            mock.fail = true;
            const auto mock_before = mock;
            const bool progress = node.step(mock, node_id);

            THEN("nothing is transmitted") {
                CHECK(!progress);
                CHECK(mock == mock_before);
            }

            AND_WHEN("step() is called again and presentation succeeds") {
                mock.fail = false;
                const auto mock_before = mock;
                const bool progress = node.step(mock, node_id);

                THEN("Request is transmitted") {
                    CHECK(progress);
                    REQUIRE(mock.request.size() == mock_before.request.size() + 1);
                    CHECK(mock.request.back() == payload);
                    CHECK(mock.accept_event.size() == mock_before.accept_event.size());
                }
            }
        }
    }
}

SCENARIO("basic Event handling") {
    GIVEN("fresh NfcNode") {
        NfcNode node;

        WHEN("Event is received") {
            const auto data = std::array { std::byte { 0x10 }, std::byte { 0x20 }, std::byte { 0x30 }, std::byte { 0x40 } };
            node.receive_event(data);

            AND_WHEN("consume() is called") {
                anfc::modbus::Event event;
                node.consume(event);

                THEN("Event is returned") {
                    CHECK(as_bytes(event) == as_bytes(data));
                }

                AND_WHEN("consume() is called again") {
                    anfc::modbus::Event event;
                    node.consume(event);

                    THEN("empty Event is returned") {
                        CHECK(event.size == 0);
                    }
                }
            }
        }

        WHEN("two Events are received") {
            const auto data1 = std::array { std::byte { 0xAA }, std::byte { 0xBB } };
            const auto data2 = std::array { std::byte { 0xCC }, std::byte { 0xDD }, std::byte { 0xEE } };
            node.receive_event(data1);
            node.receive_event(data2);

            AND_WHEN("consume() is called") {
                anfc::modbus::Event event;
                node.consume(event);

                THEN("the latest Event is returned") {
                    CHECK(as_bytes(event) == as_bytes(data2));
                }
            }
        }
    }
}

SCENARIO("graceful large data handling") {
    GIVEN("fresh NfcNode") {
        NfcNode node;

        WHEN("large data are received") {
            constexpr size_t buffer_size = sizeof(anfc::modbus::Event::data);
            constexpr size_t large_data_size = 2 * buffer_size;
            const std::vector<std::byte> large_data { large_data_size, std::byte { 0xFF } };
            node.receive_event(large_data);

            AND_WHEN("consume() is called") {
                anfc::modbus::Event event;
                node.consume(event);

                THEN("data are truncated") {
                    CHECK(as_bytes(event) == std::vector(buffer_size, std::byte { 0xFF }));
                }
            }
        }
    }
}
