#pragma once

#include "shared_config.h"
#include "otp_types.hpp"

FwAutoUpdate get_auto_update_flag(void);

/*!****************************************************************************
 * \brief Copy specified BBF's SFN from RAM
 *
 * \param [out] out_buff pointer to destination buffer
 *
 * \return	void
 *
 *****************************************************************************/
void get_specified_SFN(char *out_buff);

// This is important: we need to disable auto flashing of newer FW - in case auto-update-specified was a downgrade
void set_fw_update_flag(FwAutoUpdate flag);

// eeprom_write_byte() is temporarly removed (for new bootloader downgrade testing)
void set_fw_signature(uint8_t fw_signature);

void set_bootloader_valid();

void set_bootloader_invalid();

void clr_bbf_sfn();

void set_xlcd_status(OtpStatus status);

void set_xlcd_eeprom(XlcdEeprom eeprom);

void set_loveboard_status(OtpStatus status);

void set_loveboard_eeprom(LoveBoardEeprom eeprom);

namespace data_exchange {

OtpStatus get_xlcd_status();

XlcdEeprom get_xlcd_eeprom();

OtpStatus get_loveboard_status();

LoveBoardEeprom get_loveboard_eeprom();

bool is_fw_update_on_restart();

void fw_update_on_restart_enable();

void fw_update_older_on_restart_enable();

void fw_update_on_restart_disable();

/// Return true if the preboot set the bootloader_valid flag to true
///
/// Warning: requires bootloader version 2.0.0 (which includes preboot)
/// or newer
bool is_bootloader_valid();

void set_reflash_bbf_sfn(const char *sfn);

bool has_apendix();

bool has_fw_signature();
} // namespace data_exchange
