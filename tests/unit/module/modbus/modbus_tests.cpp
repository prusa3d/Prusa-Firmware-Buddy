#include <modbus/modbus.hpp>
#include <modbus/modbus_constants.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <bitset>
#include <utils/byte_utils.hpp>

using namespace modbus;

namespace {

class MockDevice1 final : public Callbacks {
public:
    static constexpr size_t reg_count = 4;
    std::array<uint16_t, reg_count> registers = { 0, 1, 2, 3 };
    static constexpr size_t coil_count = 20;
    std::bitset<coil_count> coils { 0xAAAAA };

    virtual ServerAddress server_address() const override { return static_cast<ServerAddress>(1); }

    virtual Status read_registers(uint16_t address, std::span<uint16_t> out) override {
        REQUIRE(reinterpret_cast<intptr_t>(out.data()) % alignof(uint16_t) == 0);
        if (address + out.size() >= reg_count) {
            return Status::IllegalAddress;
        }

        for (size_t i = 0; i < out.size(); i++) {
            out[i] = registers[address + i];
        }

        return Status::Ok;
    }

    virtual Status write_registers(uint16_t address, std::span<const uint16_t> in) override {
        REQUIRE(reinterpret_cast<intptr_t>(in.data()) % alignof(uint16_t) == 0);
        if (address + in.size() >= reg_count) {
            return Status::IllegalAddress;
        }

        for (size_t i = 0; i < in.size(); i++) {
            registers[address + i] = in[i];
        }

        return Status::Ok;
    }

    virtual Status read_coils(uint16_t first_address, uint16_t no_coils, WritableBytes out) override {
        if (first_address + no_coils >= coils.size()) {
            return Status::IllegalAddress;
        }

        for (int coil = 0; coil < no_coils; coil++) {
            if (coils[first_address + coil]) {
                out[(coil / 8)] |= std::byte((1 << (coil % 8)));
            } else {
                out[(coil / 8)] &= std::byte((~(1 << (coil % 8))));
            }
        }
        return Status::Ok;
    }

    virtual Status write_coils(uint16_t first_address, uint16_t no_coils, Bytes in) override {
        if (first_address + no_coils >= coils.size()) {
            return Status::IllegalAddress;
        }

        for (int coil = 0; coil < no_coils; coil++) {
            coils[first_address + coil] = (std::to_integer<uint8_t>(in[(coil / 8)]) & (1 << (coil % 8)));
        }

        return Status::Ok;
    }
};

Bytes safe_transaction_handle(Dispatch &dispatch, WritableBytes in, WritableBytes out) {

    // We assume this is true due to allocator that gives data to vector.
    // If not, we'll have to manually fix this in the tests (eg. the failure
    // would be problem of tests, not of the tested code).
    REQUIRE(reinterpret_cast<intptr_t>(in.data()) % alignof(uint16_t) == 0);

    return handle_transaction(dispatch, in, out);
}

struct ModbusMessageBuilder {
    std::vector<std::byte> data;

    ModbusMessageBuilder &device_id(uint8_t id) {
        data.push_back(std::byte { id });
        return *this;
    }

    ModbusMessageBuilder &function_code(std::byte fn_code) {
        data.push_back(fn_code);
        return *this;
    }

    ModbusMessageBuilder &address(uint16_t addr) {
        data.push_back(static_cast<std::byte>(addr >> 8)); // high byte
        data.push_back(static_cast<std::byte>(addr & 0xFF)); // low byte
        return *this;
    }

    ModbusMessageBuilder &count(uint16_t cnt) {
        data.push_back(static_cast<std::byte>(cnt >> 8)); // high byte
        data.push_back(static_cast<std::byte>(cnt & 0xFF)); // low byte
        return *this;
    }

    ModbusMessageBuilder &byte_count(uint8_t bc) {
        data.push_back(std::byte { bc });
        return *this;
    }

    ModbusMessageBuilder &word(uint16_t w) {
        data.push_back(static_cast<std::byte>(w >> 8)); // high byte
        data.push_back(static_cast<std::byte>(w & 0xFF)); // low byte
        return *this;
    }

    ModbusMessageBuilder &byte(uint8_t b) {
        data.push_back(std::byte { b });
        return *this;
    }

    ModbusMessageBuilder &bytes(const uint8_t *b, size_t len) {
        const auto *begin = reinterpret_cast<const std::byte *>(b);
        data.insert(data.end(), begin, begin + len);
        return *this;
    }

    ModbusMessageBuilder &bytes(const char *str) {
        return bytes(reinterpret_cast<const uint8_t *>(str), strlen(str));
    }

    ModbusMessageBuilder &crc() {
        uint16_t computed_crc = compute_crc(data);
        data.push_back(static_cast<std::byte>(computed_crc & 0xFF)); // Low byte
        data.push_back(static_cast<std::byte>(computed_crc >> 8)); // High byte
        return *this;
    }

    WritableBytes view() {
        return std::span(data);
    }

    size_t size() const {
        return data.size();
    }
};

} // namespace

TEST_CASE("Modbus transaction - refused inputs", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    SECTION("Too short") {
        REQUIRE(handle_transaction(dispatch, {}, out_buffer).empty());
    }

    SECTION("Garbage") {
        // Complete nonsense - not a valid modbus message at all
        auto msg = ModbusMessageBuilder()
                       .bytes("Hello World");
        REQUIRE(handle_transaction(dispatch, msg.view(), out_buffer).empty());
    }

    SECTION("Wrong CRC") {
        // This would be a valid message, but CRC is wrong.
        auto msg = ModbusMessageBuilder()
                       .device_id(1)
                       .function_code(fc::read_input_registers)
                       .address(1)
                       .count(1)
                       .byte('X')
                       .byte('X'); // Invalid CRC
        REQUIRE(handle_transaction(dispatch, msg.view(), out_buffer).empty());
    }

    SECTION("Other device") {
        auto msg = ModbusMessageBuilder()
                       .device_id(2) // Device 2 doesn't exist
                       .function_code(fc::read_input_registers)
                       .address(1)
                       .count(1)
                       .crc();
        REQUIRE(safe_transaction_handle(dispatch, msg.view(), out_buffer).empty());
    }

    SECTION("Other device with unknown function") {
        const std::byte unknown_function { 0x08 };
        auto msg = ModbusMessageBuilder()
                       .device_id(2)
                       .function_code(unknown_function)
                       .address(1)
                       .count(1)
                       .crc();
        REQUIRE(safe_transaction_handle(dispatch, msg.view(), out_buffer).empty());
    }

    SECTION("Low bytes") {
        // Write 2 registers starting at address 1, but byte count (3) is less than expected (4)
        auto msg = ModbusMessageBuilder()
                       .device_id(1)
                       .function_code(fc::write_multiple_registers)
                       .address(1)
                       .count(2)
                       .byte_count(3)
                       .byte(0)
                       .byte('A')
                       .byte('A')
                       .byte(0)
                       .crc();
        safe_transaction_handle(dispatch, msg.view(), out_buffer);
    }

    SECTION("High bytes") {
        // Write 2 registers starting at address 1, but byte count (5) is more than expected (4)
        auto msg = ModbusMessageBuilder()
                       .device_id(1)
                       .function_code(fc::write_multiple_registers)
                       .address(1)
                       .count(2)
                       .byte_count(5)
                       .byte(0)
                       .byte('A')
                       .byte('A')
                       .byte(0)
                       .crc();
        safe_transaction_handle(dispatch, msg.view(), out_buffer);
    }

    SECTION("Short message") {
        // Write 2 registers starting at address 1, message is truncated
        auto msg = ModbusMessageBuilder()
                       .device_id(1)
                       .function_code(fc::write_multiple_registers)
                       .address(1)
                       .count(2)
                       .byte_count(4)
                       .byte(0)
                       .byte('A')
                       .byte('A')
                       .crc();
        safe_transaction_handle(dispatch, msg.view(), out_buffer);
    }
}

TEST_CASE("Invalid function", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };
    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Request with invalid function code 0x22
    const std::byte invalid_fc { 0x22 };
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(invalid_fc)
                   .address(1)
                   .count(2)
                   .byte_count(4)
                   .byte(0)
                   .byte('A')
                   .byte('A')
                   .byte(0)
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 5);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1); // Device ID
    REQUIRE(resp[1] == 0x22 + 0x80); // Error flag | function code
    REQUIRE(resp[2] == 1); // Status: IllegalFunction
}

TEST_CASE("Invalid address", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Write 2 registers starting at address 3 (address 3 is in-range, 4 is out of range)
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::write_multiple_registers)
                   .address(3)
                   .count(2)
                   .byte_count(4)
                   .byte(0)
                   .byte('A')
                   .byte('A')
                   .byte(0)
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 5);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1); // Device ID
    REQUIRE(resp[1] == 0x10 + 0x80); // Error flag | function code
    REQUIRE(resp[2] == 2); // Status: IllegalAddress
}

TEST_CASE("Success write", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Write 2 registers starting at address 1: [0x0041, 0x4100]
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::write_multiple_registers)
                   .address(1)
                   .count(2)
                   .byte_count(4)
                   .byte(0)
                   .byte('A') // 0x41
                   .byte('A') // 0x41
                   .byte(0)
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(resp[0] == 1); // Device ID
    REQUIRE(resp[1] == 0x10); // Function code (no error)

    REQUIRE(md1.registers[0] == 0); // Register 0 unchanged
    REQUIRE(md1.registers[1] == 65); // Register 1 = 0x0041 (65)
    REQUIRE(md1.registers[2] == 65 << 8); // Register 2 = 0x4100 (16640)
    REQUIRE(md1.registers[3] == 3); // Register 3 unchanged
}

TEST_CASE("Success read", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Read 2 holding registers starting at address 1
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::read_holding_registers)
                   .address(1)
                   .count(2)
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 9);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    // Expected response: device_id=1, function=3, byte_count=4, data=[0x0001, 0x0002]
    REQUIRE(memcmp("\1\3\4\0\1\0\2", resp, 7) == 0);
}

TEST_CASE("Success coils read", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    // zero-initialized out buffer
    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Read 17 coils starting at address 1
    // MockDevice1 initializes coils to 0xAAAAA (binary: 1010 1010 1010 1010 1010)
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::read_coils)
                   .address(1)
                   .count(17)
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    // Expected: device_id=1, function=1, byte_count=3, data=[0x55, 0x55, 0x01]
    REQUIRE(memcmp("\1\1\3\x55\x55\1", resp, 6) == 0);
}

TEST_CASE("Sucess coil write", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Write single coil at address 1 to OFF (0x0000)
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::write_coil)
                   .address(1)
                   .word(0x0000) // OFF
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(memcmp("\1\5\0\1\0\0", resp, 6) == 0);
    REQUIRE(!md1.coils[1]);

    // Write single coil at address 1 to ON (0xFF00)
    msg = ModbusMessageBuilder()
              .device_id(1)
              .function_code(fc::write_coil)
              .address(1)
              .word(0xFF00) // ON
              .crc();
    response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(memcmp("\1\5\0\1\xFF\0", resp, 6) == 0);
    REQUIRE(md1.coils[1]);
}

TEST_CASE("Sucess coils write", "[modbus]") {
    MockDevice1 md1;
    std::array<modbus::Callbacks *, 1> devices { &md1 };
    modbus::Dispatch dispatch { devices };

    alignas(uint16_t) std::array<std::byte, 40> out_buffer {};

    // Write 16 coils starting at address 1
    // Data: 0x01 = 0000 0001, 0x0F = 0000 1111
    // This sets coils [1-16] to: 1000 0000 1111 0000
    auto msg = ModbusMessageBuilder()
                   .device_id(1)
                   .function_code(fc::write_coils)
                   .address(1)
                   .count(16)
                   .byte_count(2)
                   .byte(0x01) // Coils 1-8:  bit pattern 0000 0001
                   .byte(0x0F) // Coils 9-16: bit pattern 0000 1111
                   .crc();
    auto response = safe_transaction_handle(dispatch, msg.view(), out_buffer);
    REQUIRE(response.size() == 8);
    REQUIRE(compute_crc(response) == 0);
    const uint8_t *resp = reinterpret_cast<const uint8_t *>(response.data());

    REQUIRE(memcmp("\1\x0F\0\1\0\x10", resp, 6) == 0);

    // Verify the coil states (LSB first within each byte)
    REQUIRE(!md1.coils[0]); // Address 0 not written
    REQUIRE(md1.coils[1]); // Bit 0 of 0x01 = 1
    REQUIRE(!md1.coils[2]); // Bit 1 of 0x01 = 0
    REQUIRE(!md1.coils[3]); // Bit 2 of 0x01 = 0
    REQUIRE(!md1.coils[4]); // Bit 3 of 0x01 = 0
    REQUIRE(!md1.coils[5]); // Bit 4 of 0x01 = 0
    REQUIRE(!md1.coils[6]); // Bit 5 of 0x01 = 0
    REQUIRE(!md1.coils[7]); // Bit 6 of 0x01 = 0
    REQUIRE(!md1.coils[8]); // Bit 7 of 0x01 = 0
    REQUIRE(md1.coils[9]); // Bit 0 of 0x0F = 1
    REQUIRE(md1.coils[10]); // Bit 1 of 0x0F = 1
    REQUIRE(md1.coils[11]); // Bit 2 of 0x0F = 1
    REQUIRE(md1.coils[12]); // Bit 3 of 0x0F = 1
    REQUIRE(!md1.coils[13]); // Bit 4 of 0x0F = 0
    REQUIRE(!md1.coils[14]); // Bit 5 of 0x0F = 0
    REQUIRE(!md1.coils[15]); // Bit 6 of 0x0F = 0
    REQUIRE(!md1.coils[16]); // Bit 7 of 0x0F = 0
    REQUIRE(md1.coils[17]); // Address 17 unchanged (was 1 from 0xAAAAA)
    REQUIRE(!md1.coils[18]); // Address 18 unchanged (was 0 from 0xAAAAA)
    REQUIRE(md1.coils[19]); // Address 19 unchanged (was 1 from 0xAAAAA)
}
