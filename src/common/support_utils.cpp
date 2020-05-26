#include "config.h"
#include "otp.h"
#include "sys.h"
#include "shared_config.h"
#include "support_utils.h"
#include "display.h"
#include "string.h"
#include "lang.h"
#include "../../gui/wizard/selftest.h"

#include "tm_stm32f4_crc.h"
#include "qrcodegen.h"

char *eofstr(char *str) {
    return (str + strlen(str));
}

void block2hex(char *str, uint32_t str_size, uint8_t *pdata, size_t length) {
    for (; length > 0; length--)
        snprintf(eofstr(str), str_size - strlen(str), "%02X", *(pdata++));
}

void append_crc(char *str, uint32_t str_size) {
    uint32_t crc;

    TM_CRC_Init(); // !!! should be somewhere else (not sure where yet)
    crc = TM_CRC_Calculate8((uint8_t *)(str + sizeof(ER_URL) - 1), strlen(str) - sizeof(ER_URL) + 1, 1);
    snprintf(eofstr(str), str_size - strlen(str), "/%08lX", crc);
}

void create_path_info_4error(char *str, uint32_t str_size, int error_code) {
    // FIXME use std::array instead

    strlcpy(str, ER_URL, str_size);
    snprintf(eofstr(str), str_size - strlen(str), "%d/", error_code);
    snprintf(eofstr(str), str_size - strlen(str), "%d/", PRINTER_TYPE);
    snprintf(eofstr(str), str_size - strlen(str), "%08lX%08lX%08lX/", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    //!//     snprintf(eofstr(str), str_size - strlen(str), "%d/", FW_VERSION);
    snprintf(eofstr(str), str_size - strlen(str), "%s", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
    append_crc(str, str_size);
}

void create_path_info_4service(char *str, uint32_t str_size) {
    // FIXME use std::array instead

    strlcpy(str, IR_URL, str_size);
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
