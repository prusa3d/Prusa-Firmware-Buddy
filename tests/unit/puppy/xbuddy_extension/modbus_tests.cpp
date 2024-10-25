#include <puppy/xbuddy_extension/modbus.hpp>

#include <catch2/catch.hpp>

#include <cstring>

using namespace modbus;

namespace {

class MockCallbacks final : public Callbacks {
public:
    static constexpr size_t reg_count = 4;
    std::array<uint16_t, reg_count> registers = { 0, 1, 2, 3 };
    virtual Status read_register(uint8_t device, uint16_t address, uint16_t &out) override {
        if (device != 1) {
            return Status::Ignore;
        }

        if (address >= reg_count) {
            return Status::IllegalAddress;
        }

        out = registers[address];

        return Status::Ok;
    }
    virtual Status write_register(uint8_t device, uint16_t address, uint16_t value) override {
        if (device != 1) {
            return Status::Ignore;
        }

        if (address >= reg_count) {
            return Status::IllegalAddress;
        }

        registers[address] = value;

        return Status::Ok;
    }
};

std::span<const std::byte> s2b(const char *s, size_t len) {
    return std::span(reinterpret_cast<const std::byte *>(s), len);
}

std::span<const std::byte> s2b(const char *s) {
    const size_t len = strlen(s);
    return s2b(s, len);
}

std::span<const std::byte> trans_with_crc(Callbacks &callbacks, const char *s, size_t len, std::span<std::byte> out) {
    auto *b = reinterpret_cast<const std::byte *>(s);
    std::vector<std::byte> in(b, b + len);
    uint16_t crc = compute_crc(in);
    in.push_back(static_cast<std::byte>(crc & 0xFF));
    in.push_back(static_cast<std::byte>(crc >> 8));

    return handle_transaction(callbacks, in, out);
}

} // namespace

TEST_CASE("Modbus transaction - refused inputs") {
    MockCallbacks callbacks;
    std::array<std::byte, 40> out_buffer;

    SECTION("Too short") {
        REQUIRE(handle_transaction(callbacks, {}, out_buffer).empty());
    }

    SECTION("Garbage") {
        // Complete nonsense
        REQUIRE(handle_transaction(callbacks, s2b("hello world"), out_buffer).empty());
    }

    SECTION("Wrong CRC") {
        // This would be a valid message, but CRC is wrong.
        // device: 1
        // register: 4
        // address: 1
        // count: 1
        // crc: XX
        REQUIRE(handle_transaction(callbacks, s2b("\1\4\0\1\0\1XX", 8), out_buffer).empty());
    }

    SECTION("Other device") {
        REQUIRE(trans_with_crc(callbacks, "\2\4\0\1\0\1", 6, out_buffer).empty());
    }

    SECTION("Low bytes") {
        trans_with_crc(callbacks, "\1\x10\0\1\0\2\3\0AA\0", 11, out_buffer);
    }

    SECTION("High bytes") {
        trans_with_crc(callbacks, "\1\x10\0\1\0\2\5\0AA\0", 11, out_buffer);
    }

    SECTION("Short message") {
        trans_with_crc(callbacks, "\1\x10\0\1\0\2\4\0AA", 10, out_buffer);
    }
}

TEST_CASE("Invalid function") {
    MockCallbacks callbacks;
    std::array<std::byte, 40> out_buffer;

    auto response = trans_with_crc(callbacks, "\1\x22\0\1\0\2\4\0AA\0", 11, out_buffer);
    REQUIRE(response.size() == 5);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1);
    // Error + function 22
    REQUIRE(resp[1] == 0x22 + 0x80);
    REQUIRE(resp[2] == 1);
}

TEST_CASE("Invalid address") {
    MockCallbacks callbacks;
    std::array<std::byte, 40> out_buffer;

    // 3 is in-range, 4 is out of range
    auto response = trans_with_crc(callbacks, "\1\x10\0\3\0\2\4\0AA\0", 11, out_buffer);
    REQUIRE(response.size() == 5);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1);
    // Error + function 10
    REQUIRE(resp[1] == 0x10 + 0x80);
    REQUIRE(resp[2] == 2);
}

TEST_CASE("Success write") {
    MockCallbacks callbacks;
    std::array<std::byte, 40> out_buffer;

    auto response = trans_with_crc(callbacks, "\1\x10\0\1\0\2\4\0AA\0", 11, out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1);
    // Error + function 10
    REQUIRE(resp[1] == 0x10);

    REQUIRE(callbacks.registers[0] == 0);
    REQUIRE(callbacks.registers[1] == 65);
    REQUIRE(callbacks.registers[2] == 65 << 8);
    REQUIRE(callbacks.registers[3] == 3);
}

TEST_CASE("Success read") {
    MockCallbacks callbacks;
    std::array<std::byte, 40> out_buffer;

    auto response = trans_with_crc(callbacks, "\1\3\0\1\0\2", 6, out_buffer);
    REQUIRE(response.size() == 9);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(memcmp("\1\3\4\0\1\0\2", resp, 7) == 0);
}
