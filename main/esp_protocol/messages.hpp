#pragma once

#include "checksum.hpp"
#include <array>

namespace esp {

constexpr uint8_t REQUIRED_PROTOCOL_VERSION = 13;
constexpr size_t INTRON_SIZE = 8;
constexpr size_t SSID_LEN = 32;
constexpr size_t MAC_SIZE = 6;
using Intron = std::array<uint8_t, INTRON_SIZE>;
constexpr Intron DEFAULT_INTRON = { 'U', 'N', 0, 1, 2, 3, 4, 5 };

// note: Values 1-5 are deprecated and shouldn't be used
enum class MessageType : uint8_t {
    DEVICE_INFO_V2 = 0,
    CLIENTCONFIG_V2 = 6,
    PACKET_V2 = 7,
    SCAN_START = 8,
    SCAN_STOP = 9,
    SCAN_AP_CNT = 10,
    SCAN_AP_GET = 11,
};

struct __attribute__((packed)) Header {
    MessageType type;
    union {
        uint8_t variable_byte; // generic name when setting the value from generic function
        uint8_t version; // when type == DEVICE_INFO_V2
        uint8_t up; // when type == PACKET_V2
        uint8_t ap_count; // when type == SCAN_AP_CNT
        uint8_t ap_index; // when type == SCAN_AP_GET
    };
    uint16_t size;
};

struct __attribute__((packed)) MessagePrelude {
    Intron intron;
    Header header;
    uint32_t data_checksum;
};

namespace data {
    struct APInfo {
        APInfo() = default;
        APInfo(APInfo &&) = default;
        APInfo(const APInfo &) = delete;
        APInfo &operator=(APInfo &&) = default;
        APInfo &operator=(const APInfo &) = delete;

        std::array<uint8_t, SSID_LEN> ssid;
        bool requires_password;
    };

    using MacAddress = std::array<uint8_t, MAC_SIZE>;
} // namespace data
} // namespace esp
