#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <test_utils/formatters.hpp>

#include <vector>
#include <ranges>

#include <onewire_master/onewire_master.hpp>
#include <utils/byte_utils.hpp>

namespace {

constexpr OneWireMaster::Timing timing {
    .reset_drive = 1,
    .reset_settle_before_presence_sample = 2,
    .reset_settle_after_presence_sample = 3,
    .write_bit_1_drive = 4,
    .write_bit_1_settle = 5,
    .write_bit_0_drive = 6,
    .write_bit_0_settle = 7,
    .read_bit_drive = 8,
    .read_bit_settle_before_sample = 9,
    .read_bit_settle_after_sample = 10,
};

using StepArgs = OneWireMaster::StepArgs;
using StepResult = OneWireMaster::StepResult;
using DeviceAddress = OneWireMaster::DeviceAddress;

struct SequenceStep {
    StepArgs args {};
    StepResult expected_result;
};

using Sequence = std::vector<SequenceStep>;

Sequence &operator+=(Sequence &a, const Sequence &b) {
    for (const auto &i : b) {
        a.push_back(i);
    }
    return a;
}

Sequence operator+(const Sequence &a, const Sequence &b) {
    Sequence result = a;
    result += b;
    return result;
}

Sequence init_sequence(bool presence) {
    return Sequence {
        SequenceStep {
            .expected_result { .next_step_delay_us = timing.reset_drive, .drive_bus_low = true },
        },
        SequenceStep {
            .expected_result { .next_step_delay_us = timing.reset_settle_before_presence_sample, .drive_bus_low = false },
        },
        SequenceStep {
            .args { .bus_is_low = presence },
            .expected_result { .next_step_delay_us = timing.reset_settle_after_presence_sample, .drive_bus_low = false },
        },
    };
};

Sequence bit_tx_sequence(bool bit) {
    return Sequence {
        SequenceStep {
            .args {},
            .expected_result {
                .next_step_delay_us = bit ? timing.write_bit_1_drive : timing.write_bit_0_drive,
                .drive_bus_low = true,
            },
        },
        SequenceStep {
            .args {},
            .expected_result {
                .next_step_delay_us = bit ? timing.write_bit_1_settle : timing.write_bit_0_settle,
            },
        },
    };
}

Sequence tx_sequence(Bytes tx) {
    Sequence sequence;

    for (const auto byte : tx) {
        for (uint8_t i = 0; i < 8; i++) {
            const bool bit = bool((byte >> i) & std::byte { 1 });
            sequence += bit_tx_sequence(bit);
        }
    }

    return sequence;
}

Sequence bit_rx_sequence(bool bit) {
    return Sequence {
        SequenceStep {
            .args {},
            .expected_result {
                .next_step_delay_us = timing.read_bit_drive,
                .drive_bus_low = true,
            },
        },
        SequenceStep {
            .args {},
            .expected_result {
                .next_step_delay_us = timing.read_bit_settle_before_sample,
            },
        },
        SequenceStep {
            .args { .bus_is_low = !bit },
            .expected_result {
                .next_step_delay_us = timing.read_bit_settle_after_sample,
            },
        },
    };
}

Sequence rx_sequence(Bytes rx) {
    Sequence sequence;

    for (const auto byte : rx) {
        for (uint8_t i = 0; i < 8; i++) {
            const bool bit = bool((byte >> i) & std::byte { 1 });
            sequence += bit_rx_sequence(bit);
        }
    }

    return sequence;
}

const Sequence finish_sequence {
    SequenceStep {
        .expected_result { .finished = true },
    },
    // Test one more time
    SequenceStep {
        .expected_result { .finished = true },
    },
};

void test_sequence(OneWireMaster &m, const Sequence &sequence) {
    for (const auto &step : sequence) {
        const auto r = m.step(step.args);
        REQUIRE(r.finished == step.expected_result.finished);
        REQUIRE(r.drive_bus_low == step.expected_result.drive_bus_low);
        REQUIRE(r.next_step_delay_us == step.expected_result.next_step_delay_us);
    }
}

} // namespace

TEST_CASE("onewire_master presence_detection", "[onewire_master]") {
    OneWireMaster m { timing };

    // Do not transfer anything, we just wanna test the presence detection
    m.start_transfer({}, {});

    const bool presence = GENERATE(false, true);
    CAPTURE(presence);

    Sequence sequence = init_sequence(presence) + finish_sequence;

    test_sequence(m, sequence);

    CHECK(m.presence_detected() == presence);
    CHECK(!m.is_active());
}

TEST_CASE("onewire_master tx_rx", "[onewire_master]") {
    OneWireMaster m { timing };

    // Do not transfer anything, we just wanna test the presence detection
    constexpr std::array tx { std::byte { 0b01010101 }, std::byte { 0b10011011 } };
    constexpr std::array expected_rx { std::byte { 0b01010101 }, std::byte { 0b10011011 }, std::byte { 0b10111011 } };

    std::array<std::byte, std::size(expected_rx)> rx;
    m.start_transfer(tx, rx);

    Sequence sequence = init_sequence(true) + tx_sequence(tx) + rx_sequence(expected_rx) + finish_sequence;

    test_sequence(m, sequence);

    for (size_t i = 0; i < rx.size(); i++) {
        CAPTURE(i);
        CHECK(rx[i] == expected_rx[i]);
    }

    CHECK(m.presence_detected());
    CHECK(!m.is_active());
}

TEST_CASE("onewire_master scan_1", "[onewire_master]") {
    struct Scenario {
        std::vector<DeviceAddress> devices;
        std::optional<OneWireMaster::SearchData> data;
        DeviceAddress expected_directions;
        std::optional<OneWireMaster::SearchData> expected_result;
    };
    const std::array scenarios {
        // Empty devices - should fail
        Scenario {
            .devices {},
            .data {},
            .expected_directions = 0,
            .expected_result = std::nullopt,
        },

        // Simple scenario with two devices
        Scenario {
            .devices { 0b101, 0b100 },
            .data {},
            .expected_directions = 0b100,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b100,
                .last_discrepancy = 0,
            },
        },
        Scenario {
            .devices { 0b101, 0b100 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b100,
                .last_discrepancy = 0,
            },
            .expected_directions = 0b101,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b101,
                .last_discrepancy = OneWireMaster::no_discrepancy,
            },
        },
        // Device 0b101 got removed mid search
        Scenario {
            .devices { 0b100 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b100,
                .last_discrepancy = 0,
            },
            .expected_directions = 0b100,
            .expected_result = std::nullopt,
        },
        Scenario {
            .devices { 0b101, 0b100 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b101,
                .last_discrepancy = OneWireMaster::no_discrepancy,
            },
            .expected_directions = 0b101,
            .expected_result = std::nullopt,
        },

        // Complex scenario with four devices
        Scenario {
            .devices { 0b0000, 0b1000, 0b1010, 0b0010 },
            .data {},
            .expected_directions = 0b0000,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b0000,
                .last_discrepancy = 3,
            },
        },
        Scenario {
            .devices { 0b0000, 0b1000, 0b1010, 0b0010 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b0000,
                .last_discrepancy = 3,
            },
            .expected_directions = 0b1000,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b1000,
                .last_discrepancy = 1,
            },
        },
        Scenario {
            .devices { 0b0000, 0b1000, 0b1010, 0b0010 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b1000,
                .last_discrepancy = 1,
            },
            .expected_directions = 0b0010,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b0010,
                .last_discrepancy = 3,
            },
        },
        Scenario {
            .devices { 0b0000, 0b1000, 0b1010, 0b0010 },
            .data = OneWireMaster::SearchData {
                .device_address = 0b0010,
                .last_discrepancy = 3,
            },
            .expected_directions = 0b1010,
            .expected_result = OneWireMaster::SearchData {
                .device_address = 0b1010,
                .last_discrepancy = OneWireMaster::no_discrepancy,
            },
        },
    };

    for (const Scenario &scenario : scenarios) {
        const auto data = scenario.data.value_or(OneWireMaster::SearchData {});
        CAPTURE(scenario.data.has_value(), data.device_address, int(data.last_discrepancy));

        const auto expected_result = scenario.expected_result.value_or(OneWireMaster::SearchData {});
        CAPTURE(scenario.expected_result.has_value(), expected_result.device_address, int(expected_result.last_discrepancy));

        CAPTURE(scenario.devices);

        OneWireMaster m { timing };
        m.start_search(scenario.data);

        test_sequence(m, init_sequence(true));

        constexpr std::array tx { std::byte { 0xF0 } }; // rom search command
        test_sequence(m, tx_sequence(tx));

        std::vector<DeviceAddress> candidates = scenario.devices;

        // The search algo has bit order flipped for things to make sense
        for (int i = 0; i < 64; i++) {
            bool all_high = true;
            bool all_low = true;
            for (const DeviceAddress device : candidates) {
                const bool bit = (device >> i) & 1;
                all_high &= bit;
                all_low &= !bit;
            }

            const auto expected_direction = (scenario.expected_directions >> i) & 1;

            CAPTURE(i, all_high, all_low, expected_direction);

            test_sequence(m, bit_rx_sequence(all_high));
            test_sequence(m, bit_rx_sequence(all_low));

            // If candidates run out, the algo is expected to finish here
            if (candidates.empty()) {
                break;
            }

            test_sequence(m, bit_tx_sequence(expected_direction));

            std::erase_if(candidates, [&](DeviceAddress d) {
                return bool((d >> i) & 1) != expected_direction;
            });
        }
        test_sequence(m, finish_sequence);

        REQUIRE(!m.is_active());

        const auto result = m.search_result();
        REQUIRE(result.has_value() == scenario.expected_result.has_value());
        if (result.has_value()) {
            CHECK(result->device_address == scenario.expected_result->device_address);
            CHECK(int(result->last_discrepancy) == int(scenario.expected_result->last_discrepancy));
        }
    }
}
