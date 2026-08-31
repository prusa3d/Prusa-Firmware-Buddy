// #define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_.hpp>

// using Catch::Matchers::Equals;

#include "otp.hpp"
#include <array>
#include <cstring>
#include <string_view>

// This particular size of OTP on STM32F4 broke size check while parsing datamatrix
#define OTP_SIZE 527

TEST_CASE("OTP test v4, DataMatrixID v5, date with offset", "[otp_v4]") {

    // example OTP v4
    // clang-format off
    std::array<uint8_t, OTP_SIZE> otp = {
        4, // Data structure version (1 bytes)
        38, 0, // Data structure size (uint16_t little endian)
        15, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '1', '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', '0', '0', '1', '9', // supplier ID
        '5', '2', '2', // calendar week of production (YWW)
        '9', '9', '9', '9', '9', // daily serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, // MAC address (6 bytes)       // weekly serial number
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "000019");
    REQUIRE(datamatrix->production_year == 2025);
    REQUIRE(datamatrix->production_month == 5); // Thursday in 22th week of 2025
    REQUIRE(datamatrix->production_day == 29);
    REQUIRE(datamatrix->date_serial_number == 99999);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);

    auto mac = otp_parse_mac_address(trivial_as_bytes(otp));
    REQUIRE(mac);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200001952299999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v4, DataMatrixID v5", "[otp_v4]") {

    // example OTP v4
    // clang-format off
    std::array<uint8_t, OTP_SIZE> otp = {
        4, // Data structure version (1 bytes)
        38, 0, // Data structure size (uint16_t little endian)
        15, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '1', '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', '0', '0', '1', '9', // supplier ID
        '0', '2', '2', // calendar week of production (YWW)
        '9', '9', '9', '9', '9', // daily serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, // MAC address (6 bytes)
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "000019");
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5); // Thursday in 22th week of 2020
    REQUIRE(datamatrix->production_day == 28);
    REQUIRE(datamatrix->date_serial_number == 99999);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);

    auto mac = otp_parse_mac_address(trivial_as_bytes(otp));
    REQUIRE(mac);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200001902299999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v4, DataMatrixID v5, supplier ID with letters", "[otp_v4]") {

    // Supplier IDs ran out of plain numbers, so they may contain letters as well
    // clang-format off
    std::array<uint8_t, OTP_SIZE> otp = {
        4, // Data structure version (1 bytes)
        38, 0, // Data structure size (uint16_t little endian)
        15, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '1', '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', 'A', 'B', '1', '9', // supplier ID
        '0', '2', '2', // calendar week of production (YWW)
        '9', '9', '9', '9', '9', // daily serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, // MAC address (6 bytes)
    };
    // clang-format on

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "00AB19");

    // the letters must not shift the fields that follow
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5);
    REQUIRE(datamatrix->production_day == 28);
    REQUIRE(datamatrix->date_serial_number == 99999);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200AB1902299999", sn.size()) == 0);
}

TEST_CASE("OTP test v4", "[otp_v4]") {

    // example OTP v4
    // clang-format off
    std::array<uint8_t, OTP_SIZE> otp = {
        4, // Data structure version (1 bytes)
        38, 0, // Data structure size (uint16_t little endian)
        15, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', '0', '0', '1', '9', // supplier ID
        '0', '0', '5', '2', '5', // daily date production (YMMDD)
        '9', '9', '9', '9', // daily serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, // MAC address (6 bytes)
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 4558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "000019");
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5);
    REQUIRE(datamatrix->production_day == 25);
    REQUIRE(datamatrix->date_serial_number == 9999);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);

    auto mac = otp_parse_mac_address(trivial_as_bytes(otp));
    REQUIRE(mac);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "4558-02000019005259999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v3", "[otp_v3]") {

    // example OTP v3, taken from production MK4
    // clang-format off
    std::array<uint8_t, OTP_SIZE> otp = {
        3, // Data structure version (1 bytes)
        0x4C, 0x00, // Data structure size (uint16_t little endian)
        0x0c, // BOM ID (1 bytes)
        0x34, 0x62, 0x13, 0x62, // UNIX Timestamp from 1970 (uint32_t little endian).
        '4', '9', '1', '4', // factorify ID
        '-', // -
        '2', '7', // revision
        '1', '4', '5', '6', '0', '8', // supplier
        '1', '1', '0', '2', '1', // daily date production (YMMDD) ID
        '0', '3', '8', '3', // daily serial number
        0x00, 0xff, // \--> DataMatrix ID 1 (24 bytes)
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x9C, 0x70, 0x28, 0x2E, 0x33, // MAC address (6 bytes)
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 27);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 4914);
    REQUIRE(datamatrix->revision == 27);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "145608");
    REQUIRE(datamatrix->production_year == 2021);
    REQUIRE(datamatrix->production_month == 10);
    REQUIRE(datamatrix->production_day == 21);
    REQUIRE(datamatrix->date_serial_number == 383);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1645437492);

    auto mac = otp_parse_mac_address(trivial_as_bytes(otp));
    REQUIRE(mac);
    REQUIRE(mac->mac[0] == 0x10);
    REQUIRE(mac->mac[1] == 0x9C);
    REQUIRE(mac->mac[2] == 0x70);
    REQUIRE(mac->mac[3] == 0x28);
    REQUIRE(mac->mac[4] == 0x2E);
    REQUIRE(mac->mac[5] == 0x33);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "4914-27145608110210383", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 12);
}

TEST_CASE("OTP test v2, loveboard, exact-size span", "[otp_v2]") {

    // OTP_v2 has no MAC address and no serial number field, and the span is exactly the
    // struct size, so nothing beyond the structure is available to the parser
    // clang-format off
    std::array<uint8_t, sizeof(OTP_v2)> otp = {
        2, // Data structure version (1 bytes)
        32, 0, // Data structure size (uint16_t little endian)
        7, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', '0', '0', '1', '9', // supplier ID
        '0', '0', '5', '2', '5', // daily date production (YMMDD)
        '9', '9', '9', '9', // daily serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 4558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "000019");
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5);
    REQUIRE(datamatrix->production_day == 25);
    REQUIRE(datamatrix->date_serial_number == 9999);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);

    // OTP_v2 carries no MAC address
    REQUIRE_FALSE(otp_parse_mac_address(trivial_as_bytes(otp)).has_value());

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "4558-02000019005259999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 7);
}

TEST_CASE("OTP test v5, puppy, calendar week datamatrix", "[otp_v5]") {

    // v5 is the same structure as v2 and is parsed by the same branch
    // clang-format off
    std::array<uint8_t, sizeof(OTP_v5)> otp = {
        5, // Data structure version (1 bytes)
        32, 0, // Data structure size (uint16_t little endian)
        9, // BOM ID (1 bytes)
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '1', '4', '5', '5', '8', // factorify ID
        '-', // -
        '0', '2', // revision
        '0', '0', '0', '0', '1', '9', // supplier ID
        '0', // year of production (Y)
        '2', '2', // calendar week of production (WW)
        '9', '9', '9', '9', '9', // weekly serial number
        0, 0, // \--> DataMatrix ID 1 (24 bytes)
    };
    // clang-format on

    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(trivial_as_bytes(otp));
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(std::string_view(datamatrix->supplier_id) == "000019");
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5); // Thursday in 22th week of 2020
    REQUIRE(datamatrix->production_day == 28);
    REQUIRE(datamatrix->date_serial_number == 99999);

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);

    // OTP_v5 carries no MAC address
    REQUIRE_FALSE(otp_parse_mac_address(trivial_as_bytes(otp)).has_value());

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200001902299999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(trivial_as_bytes(otp));
    REQUIRE(bomid);
    REQUIRE(*bomid == 9);
}

TEST_CASE("OTP test v1, old board without a datamatrix", "[otp_v1]") {

    // Old boards carry a plain serial number, prefixed with CZPX and padded with 0xff
    // clang-format off
    std::array<uint8_t, sizeof(OTP_v1)> otp = {
        1, 2, 0, // board revision, the first byte doubles as the structure version
        0, // reserve
        0x43, 0x11, 0xb5, 0x63, // UNIX Timestamp from 1970 (uint32_t little endian)
        '0', '5', '1', '9', '2', '2', '3', '4', // serial number (18 bytes)
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, // MAC address (6 bytes)
    };
    // clang-format on

    // old boards keep the revision in the first three bytes, not in a datamatrix
    auto br = otp_parse_board_revision(trivial_as_bytes(otp));
    REQUIRE(br);
    REQUIRE(*br == 120);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 12);
    REQUIRE(strcmp(sn.data(), "CZPX05192234") == 0);

    // there is no datamatrix to decode
    REQUIRE_FALSE(otp_parse_datamatrix(trivial_as_bytes(otp)).has_value());

    auto timestamp = otp_parse_timestamp(trivial_as_bytes(otp));
    REQUIRE(timestamp);
    REQUIRE(*timestamp == 1672810819);
}

TEST_CASE("OTP virgin memory is rejected", "[otp_v2]") {

    // An unburned OTP reads as 0xFF, which is not a valid structure version
    std::array<uint8_t, sizeof(OTP_v2)> otp;
    otp.fill(0xff);

    REQUIRE_FALSE(otp_parse_board_revision(trivial_as_bytes(otp)).has_value());
    REQUIRE_FALSE(otp_parse_datamatrix(trivial_as_bytes(otp)).has_value());
    REQUIRE_FALSE(otp_parse_timestamp(trivial_as_bytes(otp)).has_value());
    REQUIRE_FALSE(otp_parse_mac_address(trivial_as_bytes(otp)).has_value());
    REQUIRE_FALSE(otp_parse_bom_id(trivial_as_bytes(otp)).has_value());

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, trivial_as_bytes(otp)) == 0);
}
