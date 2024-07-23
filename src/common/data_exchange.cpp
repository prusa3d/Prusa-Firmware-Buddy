#include "data_exchange.hpp"
#include "otp_types.hpp"
#include <cstdint>
#include <cstring>
#include <option/bootloader.h>

#include "at21csxx_otp.hpp"
#include <device/hal.h>

// pin PA13 state
static constexpr uint8_t APPENDIX_FLAG_MASK = 0x01;

struct __attribute__((packed)) DataExchange {
    FwAutoUpdate fw_update_flag;
    uint8_t appendix_status;
    uint8_t fw_signature;
    uint8_t bootloader_valid; //< Initialized by preboot to false; bootloader sets it to true
    char bbf_sfn[13];
    uint8_t reserved__[3]; // aligned to N*4B (20B), set to 0 - so value is defined, in case we need ot gor versioning etc.
    OtpStatus xlcd_status; // 8B
    XlcdEeprom xlcd_eeprom; // 32B
    OtpStatus loveboard_status; // 8B
    LoveBoardEeprom loveboard_eeprom; // 32B
}; // 100B in total

DataExchange ram_data_exchange __attribute__((section(".boot_fw_data_exchange")));

#if !BOOTLOADER()
    #if HAS_XLCD()
static std::pair<XlcdEeprom, OtpStatus> read_xlcd() {
    // LCD reset
    __HAL_RCC_GPIOG_CLK_ENABLE(); // enable lcd reset pin port clock
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    __HAL_RCC_GPIOC_CLK_ENABLE(); // enable lcd eeprom pin port clock
    OtpFromEeprom XlcdEeprom = OtpFromEeprom(GPIOC, GPIO_PIN_8);
    return { XlcdEeprom.calib_data, XlcdEeprom.get_status() };
}
    #endif

    #if HAS_LOVE_BOARD() || PRINTER_IS_PRUSA_MK3_5()
static std::pair<LoveBoardEeprom, OtpStatus> read_loveboard() {
    __HAL_RCC_GPIOF_CLK_ENABLE(); // enable loveboard eeprom pin port clock
    OtpFromEeprom LoveBoard = OtpFromEeprom(GPIOF, GPIO_PIN_13);
    return { LoveBoard.calib_data, LoveBoard.get_status() };
}
    #endif

void data_exchange_init() {
    ram_data_exchange.fw_update_flag = FwAutoUpdate::off;
    ram_data_exchange.appendix_status = 0; // the state is actually unknown, this represents no appendix
    ram_data_exchange.fw_signature = 0;
    ram_data_exchange.bootloader_valid = 0;
    ram_data_exchange.bbf_sfn[0] = '\0';

    ram_data_exchange.xlcd_status = {};
    ram_data_exchange.xlcd_eeprom = {};
    ram_data_exchange.loveboard_status = {};
    ram_data_exchange.loveboard_eeprom = {};

    #if HAS_XLCD()
    auto xlcd = read_xlcd();
    ram_data_exchange.xlcd_eeprom = xlcd.first;
    ram_data_exchange.xlcd_status = xlcd.second;
    #endif

    // MK3.5 doesn't have a loveboard, but it needs the detection to complain if it's running on an MK4
    #if HAS_LOVE_BOARD() || PRINTER_IS_PRUSA_MK3_5()
    auto loveboard = read_loveboard();
    ram_data_exchange.loveboard_eeprom = loveboard.first;
    ram_data_exchange.loveboard_status = loveboard.second;
    #endif
}
#else
void data_exchange_init() {}
#endif

FwAutoUpdate get_auto_update_flag(void) {
    // EEPROM flag is temporarly removed (for new bootloader downgrade testing)
    uint8_t RAM_flag = (FwAutoUpdate::on == ram_data_exchange.fw_update_flag) ? 1 : 0;

    if (ram_data_exchange.fw_update_flag == FwAutoUpdate::specified) {
        return FwAutoUpdate::specified; // highest priority
    } else if (RAM_flag) {
        return FwAutoUpdate::on; // not from RAM but from eeprom, second highest priority
    } else {
        switch (ram_data_exchange.fw_update_flag) {
        case FwAutoUpdate::on:
        case FwAutoUpdate::off:
        case FwAutoUpdate::older:
        case FwAutoUpdate::specified:
        case FwAutoUpdate::tester_mode:
            return ram_data_exchange.fw_update_flag;
        }
    }
    return FwAutoUpdate::off; // somehow corrupted data in shared RAM, no update
}

void get_specified_SFN(char *out_buff) {
    strlcpy(out_buff, ram_data_exchange.bbf_sfn, sizeof(DataExchange::bbf_sfn));
}

void set_fw_update_flag(FwAutoUpdate flag) {
    ram_data_exchange.fw_update_flag = flag;
}

void set_fw_signature(uint8_t fw_signature) {
    ram_data_exchange.fw_signature = fw_signature;
}

void set_bootloader_valid() {
    ram_data_exchange.bootloader_valid = 1;
}

void set_bootloader_invalid() {
    ram_data_exchange.bootloader_valid = 0;
}

void clr_bbf_sfn() {
    ram_data_exchange.bbf_sfn[0] = 0;
}

namespace data_exchange {

#if HAS_XLCD()
OtpStatus get_xlcd_status() {
    return ram_data_exchange.xlcd_status;
}

XlcdEeprom get_xlcd_eeprom() {
    return ram_data_exchange.xlcd_eeprom;
}
#endif

#if HAS_LOVE_BOARD() || PRINTER_IS_PRUSA_MK3_5()
OtpStatus get_loveboard_status() {
    return ram_data_exchange.loveboard_status;
}

LoveBoardEeprom get_loveboard_eeprom() {
    return ram_data_exchange.loveboard_eeprom;
}
#endif

bool has_fw_signature() {
    return ram_data_exchange.fw_signature != 0;
}

bool is_fw_update_on_restart() {
    return ram_data_exchange.fw_update_flag == FwAutoUpdate::on;
}

void fw_update_on_restart_enable() {
    ram_data_exchange.fw_update_flag = FwAutoUpdate::on;
}

void fw_update_older_on_restart_enable() {
    ram_data_exchange.fw_update_flag = FwAutoUpdate::older;
}

void fw_update_on_restart_disable() {
    ram_data_exchange.fw_update_flag = FwAutoUpdate::off;
}

bool is_bootloader_valid() {
    return ram_data_exchange.bootloader_valid ? 1 : 0;
}

void set_reflash_bbf_sfn(const char *sfn) {
    ram_data_exchange.fw_update_flag = FwAutoUpdate::specified;
    strlcpy((char *)ram_data_exchange.bbf_sfn, sfn, sizeof(DataExchange::bbf_sfn));
}

bool has_apendix() {
    return ram_data_exchange.appendix_status & APPENDIX_FLAG_MASK;
}
} // namespace data_exchange

bool running_in_tester_mode() {
    return get_auto_update_flag() == FwAutoUpdate::tester_mode;
}
