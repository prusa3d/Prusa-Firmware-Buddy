#include <stdio.h>

#include "otp.h"

const char *otp_get_mac_address_str() {
    static char mac_address_str[18] = { 0 };
    if (mac_address_str[0] == 0) {
        volatile uint8_t macaddr_bytes[OTP_MAC_ADDRESS_SIZE];
        for (uint8_t i = 0; i < OTP_MAC_ADDRESS_SIZE; i++)
            macaddr_bytes[i] = *((volatile uint8_t *)(OTP_MAC_ADDRESS_ADDR + i));
        snprintf(mac_address_str, sizeof(mac_address_str),
            "%x:%x:%x:%x:%x:%x", macaddr_bytes[0], macaddr_bytes[1], macaddr_bytes[2], macaddr_bytes[3], macaddr_bytes[4], macaddr_bytes[5]);
    }
    return mac_address_str;
}

uint32_t otp_get_timestamp() {
    // timestamp (seconds since 1970, little-endian)
    const uint32_t *timestamp = (const uint32_t *)OTP_BOARD_TIME_STAMP_ADDR;
    return *timestamp; // we are safe returning a DWORD as it is 4-bytes aligned
}

const STM32_UUID *otp_get_STM32_UUID() {
    return (const STM32_UUID *)OTP_STM32_UUID_ADDR;
}
const char *otp_get_serial_number() {
    return (const char *)(OTP_SERIAL_NUMBER_ADDR);
}
