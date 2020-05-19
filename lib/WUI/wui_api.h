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
#include "netif_settings.h"

#define FW_VER_STR_LEN    32  // length of full Firmware version string
#define MAC_ADDR_STR_LEN  18  // length of mac address string ("MM:MM:MM:SS:SS:SS" + 0)
#define SER_NUM_STR_LEN   16  // length of serial number string
#define UUID_STR_LEN      32  // length of unique identifier string
#define PRI_STATE_STR_LEN 10  // length of printer state string
#define IP4_ADDR_STR_SIZE 16  // length of ip4 address string ((0-255).(0-255).(0-255).(0-255))
#define MAX_INI_SIZE      200 // length of ini file string
#define LAN_DESCP_SIZE    150 // length of lan description string with screen format
#define MAX_TIME_STR_SIZE 12  // length of time string hh:mm:ss (12 for warning-free compilation)
#define MAX_DATE_STR_SIZE 14  // length of date string dd:mm:yyyy (13 for warning-free compilation)

#define ETHVAR_MSK(n_id) ((uint32_t)1 << (n_id))
#define ETHVAR_STATIC_LAN_ADDRS \
    (ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4) | ETHVAR_MSK(ETHVAR_LAN_MSK_IP4) | ETHVAR_MSK(ETHVAR_LAN_GW_IP4))

#define ETHVAR_EEPROM_CONFIG \
    (ETHVAR_STATIC_LAN_ADDRS | ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_MSK(ETHVAR_HOSTNAME))

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

typedef struct {
    char time[MAX_TIME_STR_SIZE]; // string representation of system time hh:mm:ss
    char date[MAX_DATE_STR_SIZE]; // string representation of system date dd.mm.yyyy
} time_str_t;

typedef struct {
    uint8_t h; // system hours
    uint8_t m; // system minutes
    uint8_t s; // system seconds
} time_of_day_t;

typedef struct {
    uint8_t d;  // system days
    uint8_t m;  // system months
    uint16_t y; // system years
} date_t;

typedef struct {
    time_of_day_t time;  // system time storage
    date_t date;         // system date storage
    uint32_t epoch_secs; // system time and date in seconds since 1.1.1900
} timestamp_t;

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
uint32_t load_ini_params(ETH_config_t *config);

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

/*!****************************************************************************
* \brief Sets up network interface according to loaded configuration values
*
* \param    [in] config - storage for loaded ethernet configurations
*
* \return   uint32_t    error value
*
* \retval   1 if successful
*****************************************************************************/
uint32_t set_loaded_eth_params(ETH_config_t *config);
/*!*******************************************************************************************
* \brief Updates ethernet addresses and their static strings according to ethconfig structure
*
* \param    [out] config - storage for loaded ethernet configurations
*********************************************************************************************/
void update_eth_addrs(ETH_config_t *config);

/*!***************************************************************************
* \brief Returns addresses aquired/or not (0.0.0.0) from DHCP server
*
* \param [in] config - structure that stores currnet ethernet configurations
*****************************************************************************/
void get_addrs_from_dhcp(ETH_config_t *config);
/*!**********************************************************************************
* \brief Returns ethernet status
*
* \param [in] config - structure that stores currnet ethernet configurations
*
* \return ETH_STATUS_t status enum of possible cases (unlinked, netif down, netif up)
************************************************************************************/
ETH_STATUS_t eth_status(ETH_config_t *config);

/*!****************************************************************************
* \brief Turns software switch of ETH netif to OFF
*
* \param [out] config - structure that stores currnet ethernet configurations
******************************************************************************/
void turn_off_LAN(ETH_config_t *config);

/*!****************************************************************************
* \brief Turns software switch of ETH netif to ON
*
* \param [out] config - structure that stores currnet ethernet configurations
******************************************************************************/
void turn_on_LAN(ETH_config_t *config);

/*!****************************************************************************
* \brief Switches ETH netif to use user-defined STATIC addresses
*
* \param [in] config - structure that stores currnet ethernet configurations
******************************************************************************/
void set_LAN_to_static(ETH_config_t *config);

/*!****************************************************************************
* \brief Switches ETH netif to use addresses from DHCP server
*
* \param [out] config - structure that stores currnet ethernet configurations
******************************************************************************/
void set_LAN_to_dhcp(ETH_config_t *config);

/*!****************************************************************************
* \brief Determines whether DHCP server already supplied ip addresses
*
* \retval Returns 1 if DHCP server supplied ip addresses, 0 otherwise
******************************************************************************/
uint8_t dhcp_addrs_are_supplied(void);

/*!*********************************************************************************************************************
* \brief Parses time from device's time storage in dest string in format hh:mm:ss
*
* \param [out] system_time - destination structure for parsed time
*
* \retval 1 if time is initialized by sntp, else 0
***********************************************************************************************************************/
uint32_t sntp_get_system_time(timestamp_t *system_time);

/*!****************************************************************************
* \brief Sets time and date in device's RTC on some other time storage
*
* \param [in] sec - number of seconds from 1.1.1900
******************************************************************************/
void sntp_set_system_time(uint32_t sec);

/*!****************************************************************************
* \brief Parses system time info into a destination string
*
* \param [out] dest - destination structure with strings for time and date
*
* \param [in] timestamp - system time aquired from device's time storage/clock
*
* \retval 1 if time is initialized by sntp, else 0
*****************************************************************************/
uint32_t stringify_timestamp(time_str_t *dest, timestamp_t *timestamp);

/*!********************************************************************************
* \brief Updates timestamp from its epoch_secs value
*
* \param [in,out] timestamp - system time aquired from device's time storage/clock
**********************************************************************************/
void update_timestamp_from_epoch_secs(timestamp_t *timestamp);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
