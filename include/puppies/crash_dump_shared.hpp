#pragma once
#include <cstdint>
/**
 * @brief This file should be exactly the same in all affected repositories, currently ModularBed, Puppy Bootloader and private
 */

namespace puppy_crash_dump {
constexpr uint32_t CRASH_DUMP_GUARD { 0x71439503 }; // Constant for indicating a crash dump is present
constexpr uint32_t APP_DESCRIPTOR_OFFSET { 0x0001ff80 - 0x2000 }; // Offset into FW application space for fw_descriptor location
constexpr uint8_t APP_DESCRIPTOR_LENGTH { 128 }; // Length of the fw_descriptor region

struct __attribute__((aligned(8))) FWDescriptor {
    enum class StoredType : uint32_t {
        fw = 12321,
        crash_dump = CRASH_DUMP_GUARD,
    };

    FWDescriptor() {} // allows uninitialized const instances

    StoredType stored_type;
    uint32_t dump_offset;
    uint8_t fingerprint[32]; // sha 256/8 == 32
    uint32_t dump_size;
};

static_assert(sizeof(FWDescriptor) % 8 == 0, "Config needs to be 8 byte aligned for flash write");
static_assert(sizeof(FWDescriptor) <= APP_DESCRIPTOR_LENGTH, "Config needs to fit into region");
}; // namespace puppy_crash_dump
