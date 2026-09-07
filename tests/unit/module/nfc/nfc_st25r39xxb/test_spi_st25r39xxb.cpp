#include <catch2/catch_test_macros.hpp>
#include <st25r39xxb/hw_interface.hpp>

#include <vector>
#include <array>
#include <variant>
#include <cstdlib>
#include <cstdio>
#include <ranges>
#include <algorithm>
#include <ostream>

#include <test_utils/formatters.hpp>
#include <utils/byte_utils.hpp>

struct WriteEvent {
    std::vector<std::byte> data;
    bool operator==(const WriteEvent &other) const { return data == other.data; };
    friend std::ostream &operator<<(std::ostream &out, const WriteEvent &e) {
        out << "WriteEvent {.data = " << std::span { e.data } << " }";
        return out;
    }
};

struct ReadEvent {
    size_t read_count;
    bool operator==(const ReadEvent &other) const { return read_count == other.read_count; };
    friend std::ostream &operator<<(std::ostream &out, const ReadEvent &e) {
        out << "ReadEvent {.read_count = " << e.read_count << " }";
        return out;
    }
};

struct ChipSelectChange {
    bool state;
    bool operator==(const ChipSelectChange &other) const { return state == other.state; };
    friend std::ostream &operator<<(std::ostream &out, const ChipSelectChange &e) {
        out << "ChipSelectChange {.state = " << (e.state ? "true" : "false") << " }";
        return out;
    }
};

using Event = std::variant<WriteEvent, ReadEvent, ChipSelectChange>;
std::ostream &operator<<(std::ostream &out, const Event &event) {
    std::visit([&](const auto &e) { out << e; }, event);
    return out;
}

class EventListener : public st25r39xxb::SpiInterface {
public:
    std::vector<Event> events {};
    std::vector<std::byte> to_read {};

protected:
    void unsafe_transmit(const Bytes &tx) final {
        events.emplace_back(WriteEvent { .data = std::vector(tx.cbegin(), tx.cend()) });
    }

    void unsafe_receive(const WritableBytes &rx) final {
        events.emplace_back(ReadEvent { .read_count = rx.size() });
        std::copy_n(to_read.begin(), std::min(rx.size(), to_read.size()), rx.begin());
    }

    void chip_select() final {
        events.emplace_back(ChipSelectChange { .state = true });
    }

    void chip_deselect() final {
        events.emplace_back(ChipSelectChange { .state = false });
    }
};

TEST_CASE("Test ST25R39XXB Spi interface message creation", "[st25r39xxb]") {
    EventListener listener;

    SECTION("Test direct_command") {
        listener.direct_command(st25r39xxb::Command::transmit_without_crc);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = std::vector<std::byte> { static_cast<std::byte>(st25r39xxb::Command::transmit_without_crc) } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test write_fifo") {
        const auto data_view = std::views::iota(0, 5) | std::views::transform([](int a) { return static_cast<std::byte>(a); });
        std::vector<std::byte> data(data_view.begin(), data_view.end());
        listener.write_fifo(data);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = std::vector { std::byte { 0x80 } } }, WriteEvent { .data = data }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test read_fifo") {
        std::vector<std::byte> data { 100 };
        listener.read_fifo(data);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = std::vector { std::byte { 0x9F } } }, ReadEvent { .read_count = data.size() }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test write A register") {
        listener.write_register(st25r39xxb::RegisterA::io_configuration_2, std::byte { 0x1f });
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x01 }, std::byte { 0x1f } } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test write B register") {
        listener.write_register(st25r39xxb::RegisterB::subcarrier_start_timer, std::byte { 0xde });
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0xfb }, std::byte { 0x06 }, std::byte { 0xde } } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test write multiple registers") {
        std::vector<std::byte> data { std::byte { 0x42 }, std::byte { 0x69 }, std::byte { 0x15 }, std::byte { 0x79 } };
        listener.write_registers_continuous(st25r39xxb::RegisterB::overshoot_protection_configuration_1, data);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0xfb }, std::byte { 0x30 }, std::byte { 0x42 }, std::byte { 0x69 }, std::byte { 0x15 }, std::byte { 0x79 } } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test read A register") {
        [[maybe_unused]] const auto res = listener.read_register(st25r39xxb::RegisterA::io_configuration_2);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x41 } } }, ReadEvent { .read_count = 1 }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test read B register") {
        [[maybe_unused]] const auto res = listener.read_register(st25r39xxb::RegisterB::subcarrier_start_timer);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0xfb }, std::byte { 0x46 } } }, ReadEvent { .read_count = 1 }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test read multiple registers") {
        std::vector<std::byte> data { 3 };
        listener.read_registers_continuous(st25r39xxb::RegisterB::overshoot_protection_configuration_1, data);
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0xfb }, std::byte { 0x70 } } }, ReadEvent { .read_count = 3 }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test register bits manipulation - setting bits") {
        listener.to_read = { std::byte { 0x10 } };
        listener.register_set_bits(st25r39xxb::RegisterA::ISO14443A, std::byte { 0x01 });
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x45 } } }, ReadEvent { .read_count = 1 }, ChipSelectChange { .state = false }, ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x05 }, std::byte { 0x11 } } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test register bits manipulation - clearing bits") {
        listener.to_read = { std::byte { 0b1001'1100 } };
        listener.register_clear_bits(st25r39xxb::RegisterA::ISO14443A, std::byte { 0b0001'1001 });
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x45 } } }, ReadEvent { .read_count = 1 }, ChipSelectChange { .state = false }, ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x05 }, std::byte { 0b1000'0100 } } }, ChipSelectChange { .state = false } }));
    }

    SECTION("Test register bits manipulation - changing bits") {
        listener.to_read = { std::byte { 0b1001'1100 } };
        listener.change_register(st25r39xxb::RegisterA::ISO14443A, std::byte { 0b0001'1001 }, std::byte { 0b0100'1001 });
        CHECK(std::ranges::equal(listener.events, std::vector<Event> { ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x45 } } }, ReadEvent { .read_count = 1 }, ChipSelectChange { .state = false }, ChipSelectChange { .state = true }, WriteEvent { .data = { std::byte { 0x05 }, std::byte { 0b1000'1101 } } }, ChipSelectChange { .state = false } }));
    }
}
