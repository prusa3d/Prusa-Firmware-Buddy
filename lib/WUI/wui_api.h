/*
 * wui_api.h
 * \brief   interface functions for Web User Interface library
 *
 *  Created on: April 22, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
 */

#ifndef _WUI_API_H_
#define _WUI_API_H_

#include <stdbool.h>
#include <stdint.h>

#include "netif_settings.h"
#include "marlin_vars.h"

#define FW_VER_STR_LEN    32  // length of full Firmware version string
#define MAC_ADDR_STR_LEN  18  // length of mac address string ("MM:MM:MM:SS:SS:SS" + 0)
#define SER_NUM_STR_LEN   16  // length of serial number string
#define UUID_STR_LEN      32  // length of unique identifier string
#define PRI_STATE_STR_LEN 10  // length of printer state string
#define IP4_ADDR_STR_SIZE 16  // length of ip4 address string ((0-255).(0-255).(0-255).(0-255))
#define MAX_INI_SIZE      200 // length of ini file string
#define LAN_DESCP_SIZE    150 // length of lan description string with screen format

#define ETHVAR_MSK(n_id) ((uint32_t)1 << (n_id))
#define ETHVAR_STATIC_LAN_ADDRS \
    (ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4) | ETHVAR_MSK(ETHVAR_LAN_MSK_IP4) | ETHVAR_MSK(ETHVAR_LAN_GW_IP4))

#define ETHVAR_STATIC_DNS_ADDRS \
    (ETHVAR_MSK(ETHVAR_DNS1_IP4) | ETHVAR_MSK(ETHVAR_DNS2_IP4))

#define ETHVAR_EEPROM_CONFIG \
    (ETHVAR_STATIC_LAN_ADDRS | ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_MSK(ETHVAR_HOSTNAME) | ETHVAR_MSK(ETHVAR_DNS1_IP4) | ETHVAR_MSK(ETHVAR_DNS2_IP4))

#define APVAR_EEPROM_CONFIG \
    (ETHVAR_MSK(APVAR_SSID) | ETHVAR_MSK(APVAR_PASS) | ETHVAR_MSK(APVAR_SECURITY))

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ETHVAR_LAN_FLAGS,    // uint8_t, lan.flag
    ETHVAR_HOSTNAME,     // char[20 + 1], hostname
    ETHVAR_LAN_ADDR_IP4, // ip4_addr_t, lan.addr_ip4
    ETHVAR_LAN_MSK_IP4,  // ip4_addr_t, lan.msk_ip4
    ETHVAR_LAN_GW_IP4,   // ip4_addr_t, lan.gw_ip4
    ETHVAR_DNS1_IP4,     // ip_addr_t, dns1_ip4
    ETHVAR_DNS2_IP4,     // ip_addr_t, dns2_ip4

    // Is it too much abuse to include the flags for the AP in the var_mask of related ETH_config_t?
    APVAR_SECURITY, // ap_entry_t::security, saved together in the same byte as LAN_FLAGS
    APVAR_SSID,     // char[32 + 1], ap_entry_t::ssid
    APVAR_PASS,     // char[64 + 1], ap_entry_t::pass
} ETHVAR_t;

typedef char mac_address_t[MAC_ADDR_STR_LEN];
typedef char ini_file_str_t[MAX_INI_SIZE];
typedef char lan_descp_str_t[LAN_DESCP_SIZE];

typedef struct {
    uint8_t printer_type;                  // Printer type (defined in CMakeLists.txt)
    uint8_t printer_version;               // Printer varsion (Stored in FLASH)
    char firmware_version[FW_VER_STR_LEN]; // Full project's version (4.0.3-BETA+1035.PR111.B4)
    mac_address_t mac_address;             // MAC address string "MM:MM:MM:SS:SS:SS"
    char serial_number[SER_NUM_STR_LEN];   // serial number without first four characters "CZPX" (total 15 chars, zero terminated)
    char mcu_uuid[UUID_STR_LEN];           // Unique identifier (96bits) into string format "%08lx-%08lx-%08lx"
    char printer_state[PRI_STATE_STR_LEN]; // state of the printer, have to be set in wui
} printer_info_t;

/*!*************************************************************************************************
* \brief saves the network parameters to non-volatile memory
*
* \param [in] ethconfig storage for parameters to set from static ethconfig to non-volatile memory
* \param [in] ap_config storage for AP parameters. May be NULL. Non-null is valid only with NETDEV_ESP_ID.
* \param [in] netdev_id which slots to use in the eeprom. Either NETDEV_ETH_ID or NETDEV_ESP_ID.
***************************************************************************************************/
void save_net_params(ETH_config_t *ethconfig, ap_entry_t *ap_config, uint32_t netdev_id);

/*!**********************************************************************************************
* \brief loads the network parameters from non-volatile memory
*
* \param [out] ethconfig storage for parameters to get from memory to static ethconfig structure
* \param [out] ap_config storage for parameters about connecting to a WIFI AP. May be NULL. Non-null is valid only with NETDEV_ESP_ID.
* \param [in] netdev_id which slots in the eeprom to use. Either NETDEV_ETH_ID or NETDEV_ESP_ID.
************************************************************************************************/
void load_net_params(ETH_config_t *ethconfig, ap_entry_t *ap_config, uint32_t netdev_id);

/*!****************************************************************************
* \brief load from ini file Ethernet specific parameters
*
* \param    [out] config - storage for loaded ethernet configurations
*
* \return   uint32_t    error value
*
* \retval   1 if successful
*****************************************************************************/
uint32_t load_ini_file_eth(ETH_config_t *config);

/*!****************************************************************************
* \brief load from ini file Wifi specific parameters
*
* \param    [out] config - storage for loaded ethernet configurations
* \param    [out] ap - storage for loaded accesspoint parameters
*
* \return   uint32_t    error value
*
* \retval   1 if successful
*****************************************************************************/
uint32_t load_ini_file_wifi(ETH_config_t *config, ap_entry_t *ap);

/*!*****************************************************************************************
* \brief Formats all vital eth information in destination string according to ini file format
*
* \param [out] destination null-terminated string
* \param [in] config - storage for ethernet configurations
*******************************************************************************************/
void stringify_eth_for_ini(ini_file_str_t *dest, ETH_config_t *config);

/*!****************************************************************************
* \brief Retrieves the MAC address of the requested device.
*
* \param [out] dest - static MAC address null-terminated string. May be an
*   empty string when the requested device is NETDEV_NODEV_ID or when ESP is not
*   available and its MAC is requested.
* \param [in] netdev_id - which device's address to get.
******************************************************************************/
void get_MAC_address(mac_address_t *dest, uint32_t netdev_id);

/*!*********************************************************************************************************************
* \brief Parses time from device's time storage to seconds. MONTHS are from 0 and YEARS are from 1900
*
* \retval number of seconds since epoch start (1.0.1900), if time is initialized by sntp
*
* \retval 0 if RTC time have not been initialized
***********************************************************************************************************************/
time_t sntp_get_system_time(void);

/*!**********************************************************************************************************
* \brief Sets time and date in device's RTC on some other time storage
*
* \param [in] sec - number of seconds from 1.1.1900
*
* \param [in] last_timezone - to calculate difference between timezones we need to pass last saved timezone
************************************************************************************************************/
void sntp_set_system_time(uint32_t sec, int8_t last_timezone);

/*!********************************************************************************
* \brief Adds time in seconds to given timestamp
*
* \param [in] secs_to_add - seconds that have to be added to given timestamp (+ or -)
*
* \param [in,out] timestamp - system time aquired from device's time storage/clock
**********************************************************************************/
void add_time_to_timestamp(int32_t secs_to_add, struct tm *timestamp);

////////////////////////////////////////////////////////////////////////////
/// @brief Authorization key for PrusaLink
///
/// @return Return an x-api-key
const char *wui_get_api_key();

////////////////////////////////////////////////////////////////////////////
/// @brief Generate authorization key for PrusaLink
///
/// @param[out] buffer api key buffer
/// @param[in] length Size of the buffer
/// @return Return an x-api-key
const char *wui_generate_api_key(char *, uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Generate authorization key for PrusaLink
///
/// @param[out] api_key api key buffer
/// @param[in] length Size of the buffer
void wui_store_api_key(char *, uint32_t);

/// Start a print of a given filename.
///
/// Returns false if can't print right now. Note that this doesn't check the
/// existence of the file.
bool wui_start_print(char *filename);

////////////////////////////////////////////////////////////////////////////
/// @brief A new gcode was uploaded, take appropriate actions
///
/// @param[in] path Path to the gcode file
/// @param[in] start_print Should the print be started right away (if possible)?
///
/// @return True if everything went fine. False if start_print was enabled and
///   the print was not possible to start.
bool wui_uploaded_gcode(char *path, bool start_print);

////////////////////////////////////////////////////////////////////////////
/// @brief Return the number of gcodes uploaded since boot.
///
/// May be used to check if a file was uploaded since last check.
/// Guaranteed to start at 0, but may wrap around (unlikely).
///
/// Thread safe.
uint32_t wui_gcodes_uploaded();

////////////////////////////////////////////////////////////////////////////
/// @brief initialize marlin client for tcpip thread
///
void wui_marlin_client_init(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
