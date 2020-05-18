#include "config.h"
#include "otp.h"
#include "sys.h"
#include "shared_config.h"
#include "support_utils.h"
#include "version.h"
#include "display.h"
#include "string.h"
#include "lang.h"
#include "../../gui/wizard/selftest.h"
#include <usbd_desc.h>

//  #include "dbg.h"

#include "tm_stm32f4_crc.h"
#include "qrcodegen.h"

static char *eofstr(char *str) {
    return (str + strlen(str));
}

static void block2hex(char *str, uint8_t *pdata, size_t length) {
    for (; length > 0; length--)
        sprintf(eofstr(str), "%02X", *(pdata++));
}

static void append_crc(char *str) {
    uint32_t crc;

    TM_CRC_Init(); // !!! spravne patri uplne jinam (zatim neni jasne kam)
    crc = TM_CRC_Calculate8((uint8_t *)str, strlen(str), 1);
    sprintf(eofstr(str), "/%08lX", crc);
}

void create_path_info_4error(char *str, int err_code) {
    strcpy(str, get_actual_lang()->err_url);
    sprintf(eofstr(str), "%d%d/", USBD_PID_FS, err_code);
    sprintf(eofstr(str), "%08lX%08lX%08lX/", *(uint32_t *)(OTP_STM32_UUID_ADDR), *(uint32_t *)(OTP_STM32_UUID_ADDR + sizeof(uint32_t)), *(uint32_t *)(OTP_STM32_UUID_ADDR + 2 * sizeof(uint32_t)));
    sprintf(eofstr(str), "%d%d%d/", project_version_major, project_version_minor, project_version_patch);
    sprintf(eofstr(str), "%s", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
}

void create_path_info_4service(char *str) {
    char *substr4crc;
    const lang_t *plang;

    plang = get_actual_lang();
    strcpy(str, plang->service_url);
    substr4crc = eofstr(str);
    // PrinterType
    sprintf(eofstr(str), "%d/", PRINTER_TYPE);
    // UniqueID
    block2hex(str, (uint8_t *)OTP_STM32_UUID_ADDR, OTP_STM32_UUID_SIZE);
    strcat(str, "/");
    // AppendixStatus
    sprintf(eofstr(str), "%s/", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
    // SerialNumber
    block2hex(str, (uint8_t *)OTP_SERIAL_NUMBER_ADDR, OTP_SERIAL_NUMBER_SIZE - 1); // "-1" ~ without "\x00"
    strcat(str, "/");
    // BootloaderVersion
    sprintf(eofstr(str), "%02X%02X%02X/", boot_version.major, boot_version.minor, boot_version.patch);
    // MacAddress
    block2hex(str, (uint8_t *)OTP_MAC_ADDRESS_ADDR, OTP_MAC_ADDRESS_SIZE);
    strcat(str, "/");
    // BoardVersion
    block2hex(str, (uint8_t *)OTP_BOARD_REVISION_ADDR, OTP_BOARD_REVISION_SIZE);
    strcat(str, "/");
    // TimeStamp
    block2hex(str, (uint8_t *)OTP_BOARD_TIME_STAMP_ADDR, OTP_BOARD_TIME_STAMP_SIZE);
    strcat(str, "/");
    // FWversion
    sprintf(eofstr(str), "%d-%d-%d", project_version_major, project_version_minor, project_version_patch);
    // BuildNumber
    sprintf(eofstr(str), "-%d/", project_build_number);
    // LanguageInfo
    sprintf(eofstr(str), "%lu/", (uint32_t)plang->lang_code);
    // SelfTestResult
    if (last_selftest_time == 0)
        strcat(str, "0");
    else
        sprintf(eofstr(str), "%lu-%lu", last_selftest_result, (HAL_GetTick() / 1000 - last_selftest_time));
    strcat(str, "/");
    // LockBlock
    block2hex(str, (uint8_t *)OTP_LOCK_BLOCK_ADDR, OTP_LOCK_BLOCK_SIZE);
    append_crc(substr4crc);
}
