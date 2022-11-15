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
