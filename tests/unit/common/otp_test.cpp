// #define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

using Catch::Matchers::Equals;

#include "otp.hpp"
#include <cstring>

// This particular size of OTP on STM32F4 broke size check while parsing datamatrix
#define OTP_SIZE 527

TEST_CASE("OTP test v4, DataMatrixID v5, date with offset", "[otp_v4]") {

    // example OTP v4
    uint8_t otp[OTP_SIZE] = {
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

    auto br = otp_parse_board_revision(otp, OTP_SIZE);
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(otp, OTP_SIZE);
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(datamatrix->supplier_id == 19);
    REQUIRE(datamatrix->production_year == 2025);
    REQUIRE(datamatrix->production_month == 5); // Thursday in 22th week of 2025
    REQUIRE(datamatrix->production_day == 29);
    REQUIRE(datamatrix->date_serial_number == 99999);

    uint32_t timestamp;
    REQUIRE(otp_parse_timestamp(&timestamp, otp, OTP_SIZE));
    REQUIRE(timestamp == 1672810819);

    const MAC_addr *mac = otp_parse_mac_address(otp, OTP_SIZE);
    REQUIRE(mac != nullptr);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, otp, OTP_SIZE) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200001952299999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(otp, OTP_SIZE);
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v4, DataMatrixID v5", "[otp_v4]") {

    // example OTP v4
    uint8_t otp[OTP_SIZE] = {
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

    auto br = otp_parse_board_revision(otp, OTP_SIZE);
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(otp, OTP_SIZE);
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 14558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(datamatrix->supplier_id == 19);
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5); // Thursday in 22th week of 2020
    REQUIRE(datamatrix->production_day == 28);
    REQUIRE(datamatrix->date_serial_number == 99999);

    uint32_t timestamp;
    REQUIRE(otp_parse_timestamp(&timestamp, otp, OTP_SIZE));
    REQUIRE(timestamp == 1672810819);

    const MAC_addr *mac = otp_parse_mac_address(otp, OTP_SIZE);
    REQUIRE(mac != nullptr);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, otp, OTP_SIZE) == 25);
    REQUIRE(strncmp(sn.begin(), "14558-0200001902299999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(otp, OTP_SIZE);
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v4", "[otp_v4]") {

    // example OTP v4
    uint8_t otp[OTP_SIZE] = {
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

    auto br = otp_parse_board_revision(otp, OTP_SIZE);
    REQUIRE(br);
    REQUIRE(*br == 2);

    auto datamatrix = otp_parse_datamatrix(otp, OTP_SIZE);
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 4558);
    REQUIRE(datamatrix->revision == 2);
    REQUIRE(datamatrix->supplier_id == 19);
    REQUIRE(datamatrix->production_year == 2020);
    REQUIRE(datamatrix->production_month == 5);
    REQUIRE(datamatrix->production_day == 25);
    REQUIRE(datamatrix->date_serial_number == 9999);

    uint32_t timestamp;
    REQUIRE(otp_parse_timestamp(&timestamp, otp, OTP_SIZE));
    REQUIRE(timestamp == 1672810819);

    const MAC_addr *mac = otp_parse_mac_address(otp, OTP_SIZE);
    REQUIRE(mac != nullptr);
    REQUIRE(mac->mac[0] == 0x01);
    REQUIRE(mac->mac[1] == 0x23);
    REQUIRE(mac->mac[2] == 0x45);
    REQUIRE(mac->mac[3] == 0x67);
    REQUIRE(mac->mac[4] == 0x89);
    REQUIRE(mac->mac[5] == 0xab);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, otp, OTP_SIZE) == 25);
    REQUIRE(strncmp(sn.begin(), "4558-02000019005259999", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(otp, OTP_SIZE);
    REQUIRE(bomid);
    REQUIRE(*bomid == 15);
}

TEST_CASE("OTP test v3", "[otp_v3]") {

    // example OTP v3, taken from production MK4
    uint8_t otp[OTP_SIZE] = {
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

    auto br = otp_parse_board_revision(otp, OTP_SIZE);
    REQUIRE(br);
    REQUIRE(*br == 27);

    auto datamatrix = otp_parse_datamatrix(otp, OTP_SIZE);
    REQUIRE(datamatrix);
    REQUIRE(datamatrix->product_id == 4914);
    REQUIRE(datamatrix->revision == 27);
    REQUIRE(datamatrix->supplier_id == 145608);
    REQUIRE(datamatrix->production_year == 2021);
    REQUIRE(datamatrix->production_month == 10);
    REQUIRE(datamatrix->production_day == 21);
    REQUIRE(datamatrix->date_serial_number == 383);

    uint32_t timestamp;
    REQUIRE(otp_parse_timestamp(&timestamp, otp, OTP_SIZE));
    REQUIRE(timestamp == 1645437492);

    const MAC_addr *mac = otp_parse_mac_address(otp, OTP_SIZE);
    REQUIRE(mac != nullptr);
    REQUIRE(mac->mac[0] == 0x10);
    REQUIRE(mac->mac[1] == 0x9C);
    REQUIRE(mac->mac[2] == 0x70);
    REQUIRE(mac->mac[3] == 0x28);
    REQUIRE(mac->mac[4] == 0x2E);
    REQUIRE(mac->mac[5] == 0x33);

    serial_nr_t sn;
    REQUIRE(otp_parse_serial_nr(sn, otp, OTP_SIZE) == 25);
    REQUIRE(strncmp(sn.begin(), "4914-27145608110210383", sn.size()) == 0);

    auto bomid = otp_parse_bom_id(otp, OTP_SIZE);
    REQUIRE(bomid);
    REQUIRE(*bomid == 12);
}
