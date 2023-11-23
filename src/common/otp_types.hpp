#pragma once

#include <cstdint>
#include <array>

struct __attribute__((packed)) STM32_UUID {
    uint8_t uuid[12];
};

struct __attribute__((packed)) MAC_addr {
    uint8_t mac[6];
};

// OTP structure pre-v3, does not contain version
// at the place of version is board_revision[0] ant it always contains 0 or 1
// so it is read as v0 or v1
struct __attribute__((packed)) OTP_v0 {
    uint8_t board_revision[3]; // Board revision as MAJOR.MINOR.PATCH (all in uin8_t)
    uint8_t reserve; // Reserved for 4byte write only access when writing.
    uint32_t timestamp; // UNIX Timestamp from 1970 (uint32_t little endian)
    uint8_t serialnumber[18]; // Serial number (18 bytes)
    MAC_addr mac_address; // MAC address (6 bytes)
};

// OTP_v1 is same as OTP_v0 read OTP_v0 description
using OTP_v1 = OTP_v0;

// v2 is used in loveboard
// it is the same as v5
struct __attribute__((packed)) OTP_v2 {
    uint8_t version; // Data structure version (1 bytes)
    uint16_t size; // Data structure size (uint16_t little endian)
    uint8_t bomID; // BOM ID (1 bytes)
    uint32_t timestamp; // UNIX Timestamp from 1970 (uint32_t little endian)
    uint8_t datamatrix[24]; // DataMatrix ID 1 (24 bytes)
};
// OTP_v2 is stored in RAM exchange area, it is better if its size is N*4
static_assert(sizeof(OTP_v2) % 4 == 0, "wrong size of OTP_v2");

struct __attribute__((packed)) OTP_v3 {
    uint8_t version; // Data structure version (1 bytes)
    uint16_t size; // Data structure size (uint16_t little endian)
    uint8_t bomID; // BOM ID (1 bytes)
    uint32_t timestamp; // UNIX Timestamp from 1970 (uint32_t little endian)
    uint8_t datamatrix[24]; // DataMatrix ID 1 (24 bytes)
    uint8_t datamatrix1[24]; // DataMatrix ID 2 (24 bytes)
    uint8_t serialnumber[14]; // unknown item
    MAC_addr mac_address; // MAC address (6 bytes)
};

struct __attribute__((packed)) OTP_v4 {
    uint8_t version; // Data structure version (1 bytes)
    uint16_t size; // Data structure size (uint16_t little endian)
    uint8_t bomID; // BOM ID (1 bytes)
    uint32_t timestamp; // UNIX Timestamp from 1970 (uint32_t little endian)
    uint8_t datamatrix[24]; // DataMatrix ID 1 (24 bytes)
    MAC_addr mac_address; // MAC address (6 bytes)
};

// v5 is used in puppies, it is the same as v2
using OTP_v5 = OTP_v2;

using LoveBoardEeprom = OTP_v2;

using XlcdEeprom = OTP_v2;

using serial_nr_t = std::array<char, 25>;

struct datamatrix_t {
    uint32_t date_serial_number;
    uint32_t supplier_id;
    uint16_t product_id;
    uint16_t production_year;
    uint8_t production_month;
    uint8_t production_day;
    uint8_t revision;
};

typedef uint16_t board_revision_t;

struct OtpStatus {
    uint16_t single_read_error_counter = 0;
    uint16_t repeated_read_error_counter = 0;
    uint16_t cyclic_read_error_counter = 0;
    uint8_t retried = 0;
    bool data_valid = false;
};
static_assert(sizeof(OtpStatus) % 4 == 0, "wrong size of OtpStatus");
// OtpStatus is stored in RAM exchange area, it is better if its size is N*4
