#include <array>

#include "config.h"
#include "otp.h"
#include "sys.h"
#include "shared_config.h"
#include "support_utils.h"
#include "display.h"
#include "string.h"
#include "version.h"
#include "language_eeprom.hpp"
#include "mbedtls/sha256.h"
#include "crc32.h"
#include "stm32f4xx_hal_gpio.h"

#include "qrcodegen.h"
#include "support_utils_lib.hpp"

static constexpr char INFO_URL_LONG_PREFIX[] = "HTTPS://HELP.PRUSA3D.COM";
static constexpr char ERROR_URL_LONG_PREFIX[] = "HTTPS://HELP.PRUSA3D.COM";
static constexpr char ERROR_URL_SHORT_PREFIX[] = "help.prusa3d.com";

/// FIXME same code in support_utils_lib
/// but linker cannot find it
char *eofstr(char *str) {
    return (str + strlen(str));
}

void append_crc(char *str, const uint32_t str_size) {
    uint32_t crc = crc32_calc((uint8_t *)(str + sizeof(ERROR_URL_LONG_PREFIX) - 1), strlen(str) - sizeof(ERROR_URL_LONG_PREFIX) + 1);
    snprintf(eofstr(str), str_size - strlen(str), "/%08lX", crc);
}

/// \returns 40 bit encoded to 8 chars (32 symbol alphabet: 0-9,A-V)
/// Make sure there's a space for 9 chars
void printerCode(char *str) {
    constexpr uint8_t SNSize = 4 + OTP_SERIAL_NUMBER_SIZE - 1; // + fixed header, - trailing 0
    constexpr uint8_t bufferSize = OTP_STM32_UUID_SIZE + OTP_MAC_ADDRESS_SIZE + SNSize;
    uint8_t toHash[bufferSize];
    /// CPU ID
    memcpy(toHash, (char *)OTP_STM32_UUID_ADDR, OTP_STM32_UUID_SIZE);
    //snprintf((char *)toHash, buffer, "/%08lX%08lX%08lX", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    /// MAC
    memcpy(&toHash[OTP_STM32_UUID_SIZE], (char *)OTP_MAC_ADDRESS_ADDR, OTP_MAC_ADDRESS_SIZE);
    /// SN
    memcpy(&toHash[OTP_STM32_UUID_SIZE + OTP_MAC_ADDRESS_SIZE], (char *)OTP_SERIAL_NUMBER_ADDR, SNSize);

    uint32_t hash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; /// 256 bits
    /// get hash;
    mbedtls_sha256_ret(toHash, sizeof(toHash), (unsigned char *)hash, false);

    /// shift hash by 2 bits
    hash[7] >>= 2;
    for (int i = 6; i >= 0; --i)
        rShift2Bits(hash[i], hash[i + 1]);

    /// convert number to 38 bits (32 symbol alphabet)
    for (uint8_t i = 0; i < PRINTER_CODE_SIZE; ++i) {
        str[i] = to32((uint8_t *)hash, i * 5);
    }

    /// set signature state
    if (signature_exist()) {
        setBit((uint8_t *)hash, 7);
        // setBit(str[0], 7);
    }

    /// appendix state
    if (appendix_exist()) {
        setBit((uint8_t *)hash, 6);
        //setBit(str[0], 6);
    }

    str[PRINTER_CODE_SIZE] = '\0';
}

/// Adds "/en" or other language abbreviation
void addLanguage(char *str, const uint32_t str_size) {
    char lang[3];
    const uint16_t langNum = LangEEPROM::getInstance().getLanguage();
    uint16_t *langP = (uint16_t *)lang;
    *langP = langNum;
    //uint16_t *(lang) = langNum;
    //lang[0] = langNum / 256;
    //lang[1] = langNum % 256;
    lang[2] = '\0';
    snprintf(eofstr(str), str_size - strlen(str), "/%s", lang);
}

void error_url_long(char *str, const uint32_t str_size, const int error_code) {
    /// FIXME remove eofstr & strlen

    /// fixed prefix
    strlcpy(str, ERROR_URL_LONG_PREFIX, str_size);

    addLanguage(str, str_size);

    /// error code
    snprintf(eofstr(str), str_size - strlen(str), "/%d", error_code);

    /// printer code
    snprintf(eofstr(str), str_size - strlen(str), "/");
    if (str_size - strlen(str) > 8)
        printerCode(eofstr(str));

    /// FW version
    snprintf(eofstr(str), str_size - strlen(str), "/%d", eeprom_get_ui16(EEVAR_FW_VERSION));

    //snprintf(eofstr(str), str_size - strlen(str), "/%08lX%08lX%08lX", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    //snprintf(eofstr(str), str_size - strlen(str), "/%s", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
    //append_crc(str, str_size);
}

void error_url_short(char *str, const uint32_t str_size, const int error_code) {
    /// help....com/
    strlcpy(str, ERROR_URL_SHORT_PREFIX, str_size);

    addLanguage(str, str_size);

    /// /12201
    snprintf(eofstr(str), str_size - strlen(str), "/%d", error_code);
}

bool appendix_exist() {
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

    if (bootloader->major >= 1 && bootloader->minor >= 1) {
        return !(ram_data_exchange.model_specific_flags & APPENDIX_FLAG_MASK);
    } else {
        GPIO_PinState pinState = GPIO_PIN_SET;
#ifndef _DEBUG //Secure backward compatibility
        GPIO_InitTypeDef GPIO_InitStruct = { 0 };
        GPIO_InitStruct.Pin = GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_Delay(50);
        pinState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_13);
#else //In debug version the appendix status has to be tested by bootloader version greater or equal than 1.1
        pinState = ram_data_exchange.model_specific_flags & APPENDIX_FLAG_MASK
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET;
#endif
        return pinState == GPIO_PIN_RESET;
    }
}

bool signature_exist() {
    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;
    if (bootloader->major >= 1 && bootloader->minor >= 2)
        return ram_data_exchange.fw_signature;
    return false;
}
