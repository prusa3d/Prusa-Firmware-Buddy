// otp.h - OTP memory mapping
#pragma once

#include <inttypes.h>

enum {
    // board revision (three bytes maj.min.sub)
    OTP_BOARD_REVISION_ADDR = 0x1FFF7800,
    OTP_BOARD_REVISION_SIZE = 3,

    // timestamp (seconds since 1970, little-endian)
    OTP_BOARD_TIME_STAMP_ADDR = 0x1FFF7804,
    OTP_BOARD_TIME_STAMP_SIZE = 4,

    // serial number without first four characters "CZPX" (total 15 chars, zero terminated)
    OTP_SERIAL_NUMBER_ADDR = 0x1FFF7808,
    OTP_SERIAL_NUMBER_SIZE = 16,

    // mac address - xx:xx:xx:xx:xx:xx, six byte array
    OTP_MAC_ADDRESS_ADDR = 0x1FFF781A,
    OTP_MAC_ADDRESS_SIZE = 6,

    // lock block - 16 bytes (byte at 0x1FFF7A00 locks 0x1FFF7800-0x1FFF781f)
    OTP_LOCK_BLOCK_ADDR = 0x1FFF7A00,
    OTP_LOCK_BLOCK_SIZE = 16,

    // unique identifier 96bits
    OTP_STM32_UUID_ADDR = 0x1FFF7A10,
    OTP_STM32_UUID_SIZE = 12,
};

#define otp_lock_sector0 (*((uint8_t *)OTP_LOCK_BLOCK_ADDR))

/// Returns MAC address formatted as "XX:XX:XX:XX:XX:XX" string
const char *otp_get_mac_address_str();
const char *otp_get_serial_number();
