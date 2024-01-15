#include "data_exchange.hpp"
#include "otp_types.hpp"
#include <cstdint>
#include <cstring>

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

void set_xlcd_status(OtpStatus status) {
    ram_data_exchange.xlcd_status = status;
}

void set_xlcd_eeprom(XlcdEeprom eeprom) {
    ram_data_exchange.xlcd_eeprom = eeprom;
}

void set_loveboard_status(OtpStatus status) {
    ram_data_exchange.loveboard_status = status;
}

void set_loveboard_eeprom(LoveBoardEeprom eeprom) {
    ram_data_exchange.loveboard_eeprom = eeprom;
}

namespace data_exchange {

OtpStatus get_xlcd_status() {
    return ram_data_exchange.xlcd_status;
}

XlcdEeprom get_xlcd_eeprom() {
    return ram_data_exchange.xlcd_eeprom;
}

OtpStatus get_loveboard_status() {
    return ram_data_exchange.loveboard_status;
}

LoveBoardEeprom get_loveboard_eeprom() {
    return ram_data_exchange.loveboard_eeprom;
}

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
