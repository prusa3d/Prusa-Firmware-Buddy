/// @file
#pragma once

#include <option/has_xlcd.h>
#include <option/has_love_board.h>
#include <option/has_indx.h>
#include <option/has_ht_hotend.h>
#include <otp/types.hpp>

/**
 * Init the data_exchange struct in case of no-bootloader build.
 */
void data_exchange_init();

/// @return true if firmware is running in tester mode
bool running_in_tester_mode();

namespace data_exchange {

#if HAS_XLCD()
const XlcdEeprom &get_xlcd_eeprom();
#endif

// MK3.5 doesn't have a loveboard, but it needs the detection to complain if it's running on an MK4
#if HAS_LOVE_BOARD() || PRINTER_IS_PRUSA_MK3_5() || HAS_INDX()
const OtpStatus &get_loveboard_status();

const LoveBoardEeprom &get_loveboard_eeprom();
#endif

void fw_update_on_restart_enable();

void fw_update_older_on_restart_enable();

/// Return true if the preboot set the bootloader_valid flag to true
///
/// Warning: requires bootloader version 2.0.0 (which includes preboot)
/// or newer
bool is_bootloader_valid();

void set_reflash_bbf_sfn(const char *sfn);

#if HAS_HT_HOTEND()
/// Boot-loop guard of the hotend-detection heat pulse.
///
/// Kept in the RAM shared with the bootloader, which nothing initialises: it survives a warm reset,
/// bootloader included, but not a power cycle.
bool is_heat_pulse_guard_set();
void set_heat_pulse_guard();
void clear_heat_pulse_guard();
#endif

} // namespace data_exchange
