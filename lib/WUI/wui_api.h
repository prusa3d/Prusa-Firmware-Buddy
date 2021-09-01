/*
 * wui_api.h
 * \brief   interface functions for Web User Interface library
 *
 *  Created on: April 22, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#ifndef _WUI_API_H_
#define _WUI_API_H_

#include <stdint.h>
#include <stdbool.h>
#include "netif_settings.h"

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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ETHVAR_LAN_FLAGS,    // uint8_t, lan.flag
    ETHVAR_HOSTNAME,     // char[20 + 1], hostname
    ETHVAR_LAN_ADDR_IP4, // ip4_addr_t, lan.addr_ip4
    ETHVAR_LAN_MSK_IP4,  // ip4_addr_t, lan.msk_ip4
    ETHVAR_LAN_GW_IP4,   // ip4_addr_t, lan.gw_ip4
    ETHVAR_TIMEZONE,     // int8_t, timezone
    ETHVAR_DNS1_IP4,     // ip_addr_t, dns1_ip4
    ETHVAR_DNS2_IP4,     // ip_addr_t, dns2_ip4
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
* \brief saves the Ethernet specific parameters to non-volatile memory
*
* \param [in] ETH_config storage for parameters to set from static ethconfig to non-volatile memory
*
* \return   uint32_t    error value
*
* \retval   0 if successful
***************************************************************************************************/
uint32_t save_eth_params(ETH_config_t *ethconfig);

/*!**********************************************************************************************
* \brief loads the Ethernet specific parameters from non-volatile memory
*
* \param [out] ETH_config storage for parameters to get from memory to static ethconfig structure
*
* \return   uint32_t    error value
*
* \retval   0 if successful
************************************************************************************************/
uint32_t load_eth_params(ETH_config_t *ethconfig);

/*!****************************************************************************
* \brief load from ini file Ethernet specific parameters
*
* \param    [out] config - storage for loaded ethernet configurations
*
* \return   uint32_t    error value
*
* \retval   1 if successful
*****************************************************************************/
uint32_t load_ini_file(ETH_config_t *config);

/*!****************************************************************************
* \brief access user defined addresses in memory and aquire vital printer info
*
* \param [out] printer_info* pointer to struct with storage for printer info
*
* \retval   0 if successful
*****************************************************************************/
void get_printer_info(printer_info_t *printer_info);

/*!****************************************************************************
* \brief parses MAC address from device's memory to static string
*
* \param [out] dest - static MAC address null-terminated string
******************************************************************************/
void parse_MAC_address(mac_address_t *dest);

/*!*****************************************************************************************
* \brief Parses all vital eth information in destination string according to ini file format
*
* \param [out] destination null-terminated string
* \param [in] config - storage for ethernet configurations
*******************************************************************************************/
void stringify_eth_for_ini(ini_file_str_t *dest, ETH_config_t *config);
/*!*****************************************************************************************
* \brief Parses all vital eth information in destination string according to screen format
*
* \param [out] destination null-terminated string
* \param [in] config - storage for ethernet configurations
*******************************************************************************************/
void stringify_eth_for_screen(lan_descp_str_t *dest, ETH_config_t *config);

/*!***************************************************************************
* \brief Returns the ethernet addresses
*
* \param [in] config - structure that stores currnet ethernet configurations
*****************************************************************************/
void get_eth_address(uint32_t, ETH_config_t *);

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
/// @brief Prepare file descriptor to upload the file
///
/// @param[in] filename Name of the file to start uploading
/// @return Return 0 if success otherwise the error code
uint32_t wui_upload_begin(const char *);

////////////////////////////////////////////////////////////////////////////
/// @brief Writting received data on already prepared file descriptor
///
/// @param[in] buffer Received file data
/// @param[in] length Size of the buffer
/// @return Return 0 if success otherwise the error code
uint32_t wui_upload_data(const char *, uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Finalize upload of the file
///
/// @param[in] oldFilename Temporary file name
/// @param[in] newFilename Regular file name
/// @param[in] startPrint 1 if print should start after upload, 0 otherwise
/// @return Return status code for http response
///                 200 OK
///                 409 Conflict
///                 415 Unsupported Media Type
uint32_t wui_upload_finish(const char *, const char *, uint32_t);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
