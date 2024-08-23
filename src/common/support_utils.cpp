#include <array>
#include <cassert>

#include "config.h"
#include "otp.hpp"
#include "sys.h"
#include "shared_config.h"
#include "support_utils.h"
#include "string.h"
#include "version.h"
#include "language_eeprom.hpp"
#include "mbedtls/sha256.h"
#include "crc32.h"
#include "stm32f4xx_hal_gpio.h"

#include "qrcodegen.h"
#include "support_utils_lib.hpp"
#include "data_exchange.hpp"

#include <option/bootloader.h>

/// FIXME same code in support_utils_lib
/// but linker cannot find it
char *eofstr(char *str) {
    return (str + strlen(str));
}

void printerHash(char *str, size_t size, bool state_prefix) {
    serial_nr_t serial_nr;
    const uint8_t serial_nr_len = otp_get_serial_nr(serial_nr);

    constexpr uint8_t bufferSize = sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac) + sizeof(serial_nr);
    uint8_t toHash[bufferSize];
    /// CPU ID
    memcpy(toHash, otp_get_STM32_UUID(), sizeof(STM32_UUID));
    // snprintf((char *)toHash, buffer, "/%08lX%08lX%08lX", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    /// MAC
    memcpy(&toHash[sizeof(STM32_UUID)], otp_get_mac_address()->mac, sizeof(otp_get_mac_address()->mac));
    /// SN
    memcpy(&toHash[sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac)], serial_nr.begin(), serial_nr_len);

    uint32_t hash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; /// 256 bits
    /// get hash;
    const size_t hash_len = sizeof(STM32_UUID) + sizeof(otp_get_mac_address()->mac) + serial_nr_len;
    mbedtls_sha256_ret(toHash, hash_len, (unsigned char *)hash, false);

    if (state_prefix) {
        /// shift hash by 2 bits
        hash[7] >>= 2;
        for (int i = 6; i >= 0; --i) {
            rShift2Bits(hash[i], hash[i + 1]);
        }

        /// set signature state
        if (signature_exist()) {
            setBit((uint8_t *)hash, 7);
            // setBit(str[0], 7);
        }

        /// appendix state
        if (appendix_exist()) {
            setBit((uint8_t *)hash, 6);
            // setBit(str[0], 6);
        }
    }

    /// convert number by 5-bit chunks (32 symbol alphabet)
    assert(sizeof(hash) * 8 >= size * 5);
    for (uint8_t i = 0; i < size; ++i) {
        str[i] = to32((uint8_t *)hash, i * 5);
    }
}

/// \returns 40 bit encoded to 8 chars (32 symbol alphabet: 0-9,A-V)
/// Make sure there's a space for 9 chars
void printerCode(char *str) {
    printerHash(str, PRINTER_CODE_SIZE, true);

    str[PRINTER_CODE_SIZE] = '\0';
}

/// Adds "/en" or other language abbreviation
void addLanguage(char *str, const uint32_t str_size) {
    char lang[3];
    const uint16_t langNum = LangEEPROM::getInstance().getLanguage();
    uint16_t *langP = (uint16_t *)lang;
    *langP = langNum;
    // uint16_t *(lang) = langNum;
    // lang[0] = langNum / 256;
    // lang[1] = langNum % 256;
    lang[2] = '\0';
    snprintf(eofstr(str), str_size - strlen(str), "/%s", lang);
}

bool appendix_exist() {
    // With recent bootloader this can be done the easy way
#if BOOTLOADER()
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;
    if (bootloader->major >= 1 && bootloader->minor >= 1) {
        return !(data_exchange::has_apendix());
    }
#endif

    // If debugging session is active there is no appendix
    if (DBGMCU->CR) {
        return false;
    }

    // Check appendix state (temporary breaking the debugging)
    GPIO_InitTypeDef init = {
        .Pin = GPIO_PIN_13,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(GPIOA, &init);
    HAL_Delay(50);
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_13);

    // Reinitialize to allow attaching debugger later
    init.Alternate = GPIO_AF0_SWJ;
    init.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOA, &init);

    return pin_state == GPIO_PIN_RESET;
}

bool signature_exist() {
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;
    if (bootloader->major >= 1 && bootloader->minor >= 2) {
        return data_exchange::has_fw_signature();
    }
    return false;
}
