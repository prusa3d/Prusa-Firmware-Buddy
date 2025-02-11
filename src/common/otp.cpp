#include "otp.hpp"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef STM32F4
    #include <stm32f4xx.h>
#elif defined(STM32G0)
    #include <stm32g0xx.h>
#elif defined(UNITTEST)

#else /*MCU*/
    #error "Unknown MCU"
#endif /*MCU*/

// Private absolute addresses for items already provided by the public otp functions.
// These absolute addresses/constants shall not be used outside of otp.cpp anymore!
enum {
// OTP memory start address
#ifdef STM32F4
    OTP_START_ADDR = FLASH_OTP_BASE,
    OTP_SIZE = FLASH_OTP_END - FLASH_OTP_BASE,
#elif defined(STM32G0)
    // G0 doesn't have the OTP defines
    OTP_START_ADDR = 0x1FFF7000UL,
    OTP_SIZE = 1024,
#elif defined(UNITTEST) /*MCU*/
    UID_BASE = 0,
    OTP_START_ADDR = 0x12345678,
    OTP_SIZE = 1024,
#else
    #error "Unknown MCU"
#endif /*MCU*/

    // mac address size
    OTP_MAC_ADDRESS_SIZE = 6,

    // offset into OTP where timestamp is stored (seconds since 1970, little-endian)
    OTP_BOARD_TIME_STAMP_OFFS = 4,

    // unique identifier of the STM32 CPU - 96bits
    OTP_STM32_UUID_ADDR = UID_BASE,

    // offset into OTP where datamatrix is stored
    OTP_DATAMATRIX_OFFS = 8,
    OTP_DATAMATRIX_TEXT_SIZE = (24UL) // size of textual representation of Datamatrix (as burned in OTP)
};

static_assert(sizeof(serial_nr_t) == OTP_DATAMATRIX_TEXT_SIZE + 1, "serial number size inconsistent");
static_assert(sizeof(MAC_addr) == OTP_MAC_ADDRESS_SIZE, "MAC addr size inconsistent");
static_assert(sizeof(board_revision_t) == 2, "board_revision_t size inconsistent");
static_assert(sizeof(OTP_v0) == 32, "OTP_v0 size inconsistent");
static_assert(sizeof(OTP_v1) == 32, "OTP_v1 size inconsistent");
static_assert(sizeof(OTP_v2) == 32, "OTP_v2 size inconsistent");
static_assert(sizeof(OTP_v3) == 76, "OTP_v3 size inconsistent");
static_assert(sizeof(OTP_v4) == 38, "OTP_v4 size inconsistent");
static_assert(sizeof(OTP_v5) == 32, "OTP_v5 size inconsistent");

/**
 * @brief Get OTP structure version.
 * @param scr address of the OTP structure, use (uint8_t*)OTP_START_ADDR to read own OTP
 * @return version number or 0 if the version is not defined
 */
static std::optional<uint8_t> otp_get_structure_version(const uint8_t *src) {
    assert(src != nullptr); // Trying to parse OTP from nullptr
    if (src[0] == 0xFF) {
        return std::nullopt;
    }
    return src[0];
}

std::optional<board_revision_t> otp_get_board_revision() {
    return otp_parse_board_revision((uint8_t *)OTP_START_ADDR, OTP_SIZE);
}

std::optional<board_revision_t> otp_parse_board_revision(const uint8_t *memory, size_t len) {
    assert(memory != nullptr); // Trying to parse OTP from nullptr

    auto otp_version = otp_get_structure_version(memory);
    if (!otp_version) {
        return std::nullopt;
    }
    uint8_t structure_version = *otp_version;

    if (structure_version == 2 || structure_version == 3 || structure_version == 4 || structure_version == 5) {
        auto datamatrix = otp_parse_datamatrix(memory, len);
        if (!datamatrix) {
            return std::nullopt;
        }
        return datamatrix->revision;
    } else if (structure_version < 2) {
        // OTP_v0 does not have version
        // but it is ensured there will be 0 or 1 in the first byte
        // which is now stored in structure_version
        const uint8_t *board_revision = memory;
        return board_revision[0] * 100 + board_revision[1] * 10 + board_revision[2];
    } else {
        return std::nullopt;
    }
}

uint32_t scan_digit_group(const uint8_t *src, uint8_t digits) {
    uint32_t rv = 0;
    for (; digits; --digits) {
        rv *= 10;
        rv += (*src - '0');
        ++src;
    }
    return rv;
}

static std::optional<datamatrix_t> decode_datamatrix(const uint8_t *src, size_t size) {
    assert(src != nullptr); // Trying to parse OTP from nullptr
    if (size < 23) {
        return std::nullopt;
    }

    datamatrix_t dm;

    // product ID until a '-' is found
    dm.product_id = 0;
    while (isdigit(*src) && size) {
        dm.product_id *= 10;
        dm.product_id += *src - '0';
        ++src;
        --size;
    }
    if (size < 24 - 5 || *src != '-') {
        return std::nullopt; // couldn't find the '-' separator
    }

    // skip '-'
    ++src;

    // product revision
    dm.revision = scan_digit_group(src, 2);
    src += 2;

    // supplier ID (6 digits)
    dm.supplier_id = scan_digit_group(src, 6);
    src += 6;

    // year
    dm.production_year = (*src - '0') + 2020;
    ++src;

    // If product ID is 4 digit, or smaller than 10381 read date in format YMMDD
    // else use format YCW (calendar week)
    if (dm.product_id < 10381) {
        // month
        dm.production_month = scan_digit_group(src, 2);
        src += 2;

        // day
        dm.production_day = scan_digit_group(src, 2);
        src += 2;

        // daily serial
        dm.date_serial_number = scan_digit_group(src, 4);
        src += 4;
    } else {
        // Conversion from the ISO calendar week to approximate production day and month i.e. thursday of given week
        uint16_t production_week = (uint16_t)scan_digit_group(src, 2);
        src += 2;

        // Year offset - number of days gained from/lost to the last calendar week of the last year
        // Formula = weekday of 4th January - 3
        //(monday == 0, ... , sunday == 6)
        uint16_t year_offset = -3;
        if (dm.production_year == 2020) { // special case for 2020
            year_offset += 5;
        } else {
            // Gauss algorithm simplified for 2021-2030
            year_offset += (((5 * ((dm.production_year - 1) % 10))) / 4) % 7;
        }

        struct tm date = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = production_week * 7 - 3 /*thursday*/ - year_offset,
            .tm_mon = 0,
            .tm_year = dm.production_year - 1900,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = 0,
        };

        mktime(&date);

        // month
        dm.production_month = (uint8_t)(date.tm_mon) + 1; // tm_mon [0,11]

        // day
        dm.production_day = (uint8_t)(date.tm_mday);

        // weekly serial
        dm.date_serial_number = scan_digit_group(src, 5);
        src += 5;
    }

    return dm;
}

std::optional<datamatrix_t> otp_get_datamatrix() {
    return otp_parse_datamatrix((uint8_t *)OTP_START_ADDR, OTP_SIZE);
}

std::optional<datamatrix_t> otp_parse_datamatrix(const uint8_t *memory, size_t len) {
    auto version = otp_get_structure_version(memory);

    if (!version || version < 2) {
        return std::nullopt;
    }
    return decode_datamatrix(memory + OTP_DATAMATRIX_OFFS, len - OTP_DATAMATRIX_OFFS);
}

std::optional<datamatrix_t> otp_serial_nr_to_datamatrix(const serial_nr_t &sn) {
    return decode_datamatrix((const uint8_t *)sn.begin(), sn.size());
}

const MAC_addr *otp_get_mac_address() {
    return otp_parse_mac_address((uint8_t *)OTP_START_ADDR, OTP_SIZE);
}

const MAC_addr *otp_parse_mac_address(const uint8_t *memory, size_t len) {
    assert(memory != nullptr); // Trying to parse OTP from nullptr

    auto version = otp_get_structure_version(memory);
    if (!version) {
        return nullptr;
    }

    switch (*version) {
    case 0:
    case 1: { // old boards
        if (len < sizeof(OTP_v0)) {
            return nullptr;
        }
        const OTP_v0 *otp = (const OTP_v0 *)memory;
        return &otp->mac_address;
    } break;
    case 3: {
        if (len < sizeof(OTP_v3)) {
            return nullptr;
        }
        const OTP_v3 *otp = (const OTP_v3 *)memory;
        return &otp->mac_address;
    } break;
    case 2:
    case 5: {
        return nullptr; // v2 and v5 don't have MAC
    } break;
    default: {
        if (len < sizeof(OTP_v4)) {
            return nullptr;
        }
        const OTP_v4 *otp = (const OTP_v4 *)memory;
        return &otp->mac_address;
    } break;
    }
}

std::array<char, 18> otp_get_mac_address_str() {
    std::array<char, 18> mac_address_str = { 0 };

    const MAC_addr *mac = otp_get_mac_address();
    if (mac != nullptr) {
        snprintf(mac_address_str.begin(), mac_address_str.size(),
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }
    return mac_address_str;
}

uint32_t otp_get_timestamp() {
    uint32_t timestamp;
    if (otp_parse_timestamp(&timestamp, (uint8_t *)OTP_START_ADDR, OTP_SIZE)) {
        return timestamp;
    } else {
        return 0;
    }
}

bool otp_parse_timestamp(uint32_t *timestamp, const uint8_t *memory, size_t len) {
    if (len < (OTP_BOARD_TIME_STAMP_OFFS + sizeof(uint32_t))) {
        return false;
    }
    // timestamp (seconds since 1970, little-endian)
    *timestamp = *(const uint32_t *)(memory + OTP_BOARD_TIME_STAMP_OFFS); // we are safe reading a DWORD as it is 4-bytes aligned

    return true;
}

const STM32_UUID *otp_get_STM32_UUID() {
    return (const STM32_UUID *)OTP_STM32_UUID_ADDR;
}

uint8_t otp_get_serial_nr(serial_nr_t &sn) {
    return otp_parse_serial_nr(sn, (uint8_t *)OTP_START_ADDR, OTP_SIZE);
}

uint8_t otp_parse_serial_nr(serial_nr_t &sn, const uint8_t *memory, size_t len) {
    assert(memory != nullptr); // Trying to parse OTP from nullptr

    auto version = otp_get_structure_version(memory);
    if (!version) {
        return 0;
    }

    switch (*version) {
    case 0:
    case 1: { // old boards
        if (len < sizeof(OTP_v0)) {
            return 0;
        }
        const OTP_v0 *otp = (const OTP_v0 *)memory;
        // we want to copy those 15+\0 bytes + fill the rest of sn with zeros
        strcpy(sn.begin(), "CZPX");
        size_t i;
        for (i = 0; ((i + 4) < (sn.size() - 1)) && (i < sizeof(otp->serialnumber)); i++) {
            if (isprint(otp->serialnumber[i]) == 0) {
                break; // End on first unprintable character as it can be 0xff on virgin OTP
            }
            sn[i + 4] = otp->serialnumber[i];
        }
        for (; (i + 4) < sn.size(); i++) {
            sn[i + 4] = '\0';
        }
        return strlen(sn.begin());
    } break;
    case 3: { // OTP v3
        if (len < sizeof(OTP_v3)) {
            return 0;
        }
        const OTP_v3 *otp = (const OTP_v3 *)memory;
        memcpy(sn.begin(), otp->datamatrix, OTP_DATAMATRIX_TEXT_SIZE);
        sn[OTP_DATAMATRIX_TEXT_SIZE] = 0; // terminate with a \0
        return sn.size();
    } break;
    case 4: { // OTP v4
        if (len < sizeof(OTP_v4)) {
            return 0;
        }
        const OTP_v4 *otp = (const OTP_v4 *)memory;
        memcpy(sn.begin(), otp->datamatrix, OTP_DATAMATRIX_TEXT_SIZE);
        sn[OTP_DATAMATRIX_TEXT_SIZE] = 0; // terminate with a \0
        return sn.size();
    } break;
    case 2: // OTP v2 is the same as OTP v5
    case 5: { // OTP v5
        if (len < sizeof(OTP_v5)) {
            return 0;
        }
        const OTP_v5 *otp = (const OTP_v5 *)memory;
        memcpy(sn.begin(), otp->datamatrix, OTP_DATAMATRIX_TEXT_SIZE);
        sn[OTP_DATAMATRIX_TEXT_SIZE] = 0; // terminate with a \0
        return sn.size();
    } break;
    default:
        return 0;
    }
}

std::optional<uint8_t> otp_get_bom_id() {
    return otp_parse_bom_id((uint8_t *)OTP_START_ADDR, OTP_SIZE);
}

std::optional<uint8_t> otp_parse_bom_id(const uint8_t *memory, size_t len) {
    assert(memory != nullptr); // Trying to parse OTP from nullptr

    auto version = otp_get_structure_version(memory);
    if (!version) {
        return std::nullopt;
    }

    switch (*version) {
    case 3: { // OTP v3
        if (len < sizeof(OTP_v3)) {
            break;
        }
        const OTP_v3 *otp = (const OTP_v3 *)memory;
        return otp->bomID;
    } break;

    case 4: { // OTP v4
        if (len < sizeof(OTP_v4)) {
            break;
        }
        const OTP_v4 *otp = (const OTP_v4 *)memory;
        return otp->bomID;
    } break;
    case 2: // OTP v2 is the same as OTP v5
    case 5: { // OTP v5
        if (len < sizeof(OTP_v5)) {
            break;
        }
        const OTP_v5 *otp = (const OTP_v5 *)memory;
        return otp->bomID;
    } break;

    default:
        break;
    }

    return std::nullopt;
}
