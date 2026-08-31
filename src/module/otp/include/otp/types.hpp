/// @file
#pragma once

#include <array>
#include <cstdint>
#include <span>

struct __attribute__((packed)) STM32_UUID {
    uint8_t uuid[12] = { 0 };
};

struct __attribute__((packed)) MAC_addr {
    uint8_t mac[6] = { 0 };
};

/// MAC address string representation
///
/// Format: "xx:xx:xx:xx:xx:xx" (17 characters + null terminator)
using MAC_str = std::array<char, sizeof(MAC_addr::mac) * 3>;

using DatamatrixRaw = std::array<uint8_t, 24>;
using DatamatrixRawView = std::span<const uint8_t, std::tuple_size_v<DatamatrixRaw>>;

// OTP structure pre-v3, does not contain version
// at the place of version is board_revision[0] ant it always contains 0 or 1
// so it is read as v0 or v1
struct __attribute__((packed)) OTP_v0 {
    uint8_t board_revision[3] = { 0 }; // Board revision as MAJOR.MINOR.PATCH (all in uin8_t)
    uint8_t reserve = 0; // Reserved for 4byte write only access when writing.
    uint32_t timestamp = 0; // UNIX Timestamp from 1970 (uint32_t little endian)
    std::array<uint8_t, 18> serialnumber = { 0 }; // Serial number (18 bytes)
    MAC_addr mac_address; // MAC address (6 bytes)
    static constexpr uint8_t version_number = 0;
};

// OTP_v1 is same as OTP_v0 read OTP_v0 description
struct OTP_v1 : public OTP_v0 {
    static constexpr uint8_t version_number = 1;
};

// v2 is used in loveboard
// it is the same as v5
struct __attribute__((packed)) OTP_v2 {
    uint8_t version = 0; // Data structure version (1 bytes)
    uint16_t size = 0; // Data structure size (uint16_t little endian)
    uint8_t bomID = 0; // BOM ID (1 bytes)
    uint32_t timestamp = 0; // UNIX Timestamp from 1970 (uint32_t little endian)
    DatamatrixRaw datamatrix; // DataMatrix ID 1 (24 bytes)
    static constexpr uint8_t version_number = 2;
};
// OTP_v2 is stored in RAM exchange area, it is better if its size is N*4
static_assert(sizeof(OTP_v2) % 4 == 0, "wrong size of OTP_v2");

struct __attribute__((packed)) OTP_v3 {
    uint8_t version = 0; // Data structure version (1 bytes)
    uint16_t size = 0; // Data structure size (uint16_t little endian)
    uint8_t bomID = 0; // BOM ID (1 bytes)
    uint32_t timestamp = 0; // UNIX Timestamp from 1970 (uint32_t little endian)
    DatamatrixRaw datamatrix; // DataMatrix ID 1 (24 bytes)
    DatamatrixRaw datamatrix1; // DataMatrix ID 2 (24 bytes)
    std::array<uint8_t, 14> serialnumber = { 0 }; // unknown item
    MAC_addr mac_address; // MAC address (6 bytes)
    static constexpr uint8_t version_number = 3;
};

struct __attribute__((packed)) OTP_v4 {
    uint8_t version = 0; // Data structure version (1 bytes)
    uint16_t size = 0; // Data structure size (uint16_t little endian)
    uint8_t bomID = 0; // BOM ID (1 bytes)
    uint32_t timestamp = 0; // UNIX Timestamp from 1970 (uint32_t little endian)
    DatamatrixRaw datamatrix; // DataMatrix ID 1 (24 bytes)
    MAC_addr mac_address; // MAC address (6 bytes)
    static constexpr uint8_t version_number = 4;
};

// v5 is used in puppies, it is the same as v2
struct OTP_v5 : public OTP_v2 {
    static constexpr uint8_t version_number = 5;
};

using LoveBoardEeprom = OTP_v2;

using XlcdEeprom = OTP_v2;

using serial_nr_t = std::array<char, 25>;

struct datamatrix_t {
    uint32_t date_serial_number;
    // NOTE: This used to be a int, but the production run out of ids, so instead now we allow letters as well. Technically we shifted the number to hexa, but since we can't tell if the number was originally hex or not we can't just simply parse it. So we store the numbers and letters as a "string" instead.
    std::array<char, 6> supplier_id;
    uint16_t product_id;
    uint16_t production_year;
    uint8_t production_month;
    uint8_t production_day;
    uint8_t revision;
};

using board_revision_t = uint16_t;

struct __attribute__((packed)) OtpStatus {
    uint16_t single_read_error_counter = 0;
    uint16_t repeated_read_error_counter = 0;
    uint16_t cyclic_read_error_counter = 0;
    uint8_t retried = 0;
    bool data_valid = false;
};
static_assert(sizeof(OtpStatus) % 4 == 0, "wrong size of OtpStatus");
// OtpStatus is stored in RAM exchange area, it is better if its size is N*4
