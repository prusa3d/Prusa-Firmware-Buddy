#include <array>

#include "config.h"
#include "otp.h"
#include "sys.h"
#include "shared_config.h"
#include "support_utils.h"
#include "display.h"
#include "string.h"
#include "lang.h"
#include "../../gui/wizard/selftest.h"
#include "version.h"
#include "eeprom.h"
#include "sha256.h"

#include "tm_stm32f4_crc.h"
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

void append_crc(char *str, uint32_t str_size) {
    uint32_t crc;

    TM_CRC_Init(); // !!! should be somewhere else (not sure where yet)
    crc = TM_CRC_Calculate8((uint8_t *)(str + sizeof(ERROR_URL_LONG_PREFIX) - 1), strlen(str) - sizeof(ERROR_URL_LONG_PREFIX) + 1, 1);
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
    mbedtls_sha256_ret_256(toHash, sizeof(toHash), (unsigned char *)hash);

    /// shift hash by 2 bits
    hash[7] >>= 2;
    for (int i = 6; i >= 0; --i)
        rShift2Bits(hash[i], hash[i + 1]);

    /// convert number to 38 bits (32 symbol alphabet)
    for (uint8_t i = 0; i < 8; ++i) {
        str[i] = to32((uint8_t *)hash, i * 5);
    }

    /// set first 2 bits (sign, appendix)
    /// TODO FW sign
    if (0) {
        setBit((uint8_t *)hash, 7);
        // setBit(str[0], 7);
    }

    /// appendix state
    if (ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) {
        setBit((uint8_t *)hash, 6);
        //setBit(str[0], 6);
    }

    str[8] = '\0';
}

void error_url_long(char *str, uint32_t str_size, int error_code) {
    /// FIXME remove eofstr & strlen

    /// fixed prefix
    strlcpy(str, ERROR_URL_LONG_PREFIX, str_size);

    /// language
    char lang[3];
    const uint16_t langNum = eeprom_get_var(EEVAR_LANGUAGE).ui16;
    uint16_t *langP = (uint16_t *)lang;
    *langP = langNum;
    //uint16_t *(lang) = langNum;
    //lang[0] = langNum / 256;
    //lang[1] = langNum % 256;
    lang[2] = '\0';
    snprintf(eofstr(str), str_size - strlen(str), "/%s", lang);

    /// error code
    snprintf(eofstr(str), str_size - strlen(str), "/%d", error_code);

    /// printer code
    snprintf(eofstr(str), str_size - strlen(str), "/");
    if (str_size - strlen(str) > 8)
        printerCode(eofstr(str));

    /// FW version
    snprintf(eofstr(str), str_size - strlen(str), "/%d", eeprom_get_var(EEVAR_FW_VERSION).ui16);

    //snprintf(eofstr(str), str_size - strlen(str), "/%08lX%08lX%08lX", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    //snprintf(eofstr(str), str_size - strlen(str), "/%s", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
    //append_crc(str, str_size);
}

void error_url_short(char *str, uint32_t str_size, int error_code) {
    strlcpy(str, ERROR_URL_SHORT_PREFIX, str_size);
    /// FIXME add language (/en, ...)
    snprintf(eofstr(str), str_size - strlen(str), "/%d", error_code);
}

void create_path_info_4service(char *str, uint32_t str_size) {

    strlcpy(str, INFO_URL_LONG_PREFIX, str_size);
    // PrinterType
    snprintf(eofstr(str), str_size - strlen(str), "%d/", PRINTER_TYPE);
    // UniqueID
    block2hex(str, str_size, (uint8_t *)OTP_STM32_UUID_ADDR, OTP_STM32_UUID_SIZE);
    strlcat(str, "/", str_size);
    // AppendixStatus
    snprintf(eofstr(str), str_size - strlen(str), "%s/", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
    // SerialNumber
    block2hex(str, str_size, (uint8_t *)OTP_SERIAL_NUMBER_ADDR, OTP_SERIAL_NUMBER_SIZE - 1); // "-1" ~ without "\x00"
    strlcat(str, "/", str_size);
    // BootloaderVersion
    snprintf(eofstr(str), str_size - strlen(str), "%02X%02X%02X/", boot_version.major, boot_version.minor, boot_version.patch);
    // MacAddress
    block2hex(str, str_size, (uint8_t *)OTP_MAC_ADDRESS_ADDR, OTP_MAC_ADDRESS_SIZE);
    strlcat(str, "/", str_size);
    // BoardVersion
    block2hex(str, str_size, (uint8_t *)OTP_BOARD_REVISION_ADDR, OTP_BOARD_REVISION_SIZE);
    strlcat(str, "/", str_size);
    // TimeStamp
    block2hex(str, str_size, (uint8_t *)OTP_BOARD_TIME_STAMP_ADDR, OTP_BOARD_TIME_STAMP_SIZE);
    strlcat(str, "/", str_size);
    // FWversion
    //!//     snprintf(eofstr(str), str_size - strlen(str), "%04X-", (uint16_t)(FW_VERSION));
    // BuildNumber
    //!//     snprintf(eofstr(str), str_size - strlen(str), "%d/",FW_BUILDNR);
    // LanguageInfo
    snprintf(eofstr(str), str_size - strlen(str), "%d/", get_actual_lang()->lang_code);
    // SelfTestResult
    if (last_selftest_time == 0)
        strlcat(str, "0", str_size);
    else
        snprintf(eofstr(str), str_size - strlen(str), "%lu-%lu", last_selftest_result, (HAL_GetTick() / 1000 - last_selftest_time));
    strlcat(str, "/", str_size);
    // LockBlock
    block2hex(str, str_size, (uint8_t *)OTP_LOCK_BLOCK_ADDR, OTP_LOCK_BLOCK_SIZE);
    append_crc(str, str_size);
}
