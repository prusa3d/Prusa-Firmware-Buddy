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

#define PRUSA_LINK_USERNAME "maker"

#define MAC_ADDR_STR_LEN 18 // length of mac address string ("MM:MM:MM:SS:SS:SS" + 0)
#define MAX_INI_SIZE     320 // length of ini file string

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
    ETHVAR_LAN_FLAGS, // uint8_t, lan.flag
    ETHVAR_HOSTNAME, // char[20 + 1], hostname
    ETHVAR_LAN_ADDR_IP4, // ip4_addr_t, lan.addr_ip4
    ETHVAR_LAN_MSK_IP4, // ip4_addr_t, lan.msk_ip4
    ETHVAR_LAN_GW_IP4, // ip4_addr_t, lan.gw_ip4
    ETHVAR_TIMEZONE, // int8_t, timezone
    ETHVAR_DNS1_IP4, // ip_addr_t, dns1_ip4
    ETHVAR_DNS2_IP4, // ip_addr_t, dns2_ip4
    ETHVAR_MAC_ADDRESS, // is not included in ethconfig (used in stringifying for screen)

    APVAR_SSID, // char[32 + 1], ap_entry_t::ssid
    APVAR_PASS, // char[64 + 1], ap_entry_t::pass

    ETHVAR_NTP_ADDRESS, // char[60+1], hostname or ip
} ETHVAR_t;

typedef char mac_address_t[MAC_ADDR_STR_LEN];
typedef char ini_file_str_t[MAX_INI_SIZE];

/*!*************************************************************************************************
 * \brief saves the network parameters to non-volatile memory
 *
 * \param [in] ethconfig storage for parameters to set from static ethconfig to non-volatile memory
 * \param [in] ap_config storage for AP parameters. May be NULL. Non-null is valid only with NETDEV_ESP_ID.
 * \param [in] netdev_id which slots to use in the eeprom. Either NETDEV_ETH_ID or NETDEV_ESP_ID.
 ***************************************************************************************************/
void save_net_params(netif_config_t *ethconfig, ap_entry_t *ap_config, uint32_t netdev_id);

/*!**********************************************************************************************
 * \brief loads the network parameters from non-volatile memory
 *
 * \param [out] ethconfig storage for parameters to get from memory to static ethconfig structure
 * \param [out] ap_config storage for parameters about connecting to a WIFI AP. May be NULL. Non-null is valid only with NETDEV_ESP_ID.
 * \param [in] netdev_id which slots in the eeprom to use. Either NETDEV_ETH_ID or NETDEV_ESP_ID.
 ************************************************************************************************/
void load_net_params(netif_config_t *ethconfig, ap_entry_t *ap_config, uint32_t netdev_id);

/*!****************************************************************************
 * \brief load from ini file Ethernet specific parameters
 *
 * \param    [out] config - storage for loaded ethernet configurations
 *
 * \return   uint32_t    error value
 *
 * \retval   1 if successful
 *****************************************************************************/
uint32_t load_ini_file_eth(netif_config_t *config);

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
uint32_t load_ini_file_wifi(netif_config_t *config, ap_entry_t *ap);

/*!****************************************************************************
 * \brief Retrieves the MAC address of the requested device.
 *
 * \param [out] dest - static MAC address null-terminated string. May be an
 *   empty string when the requested device is NETDEV_NODEV_ID or when ESP is not
 *   available and its MAC is requested.
 * \param [in] netdev_id - which device's address to get.
 ******************************************************************************/
void get_MAC_address(mac_address_t *dest, uint32_t netdev_id);

/*!**********************************************************************************************************
 * \brief Sets time and date in device's RTC on some other time storage
 *
 * \param [in] sec - number of seconds from 1.1.1900
 ************************************************************************************************************/
void sntp_set_system_time(uint32_t sec);

/*!********************************************************************************
 * \brief Adds time in seconds to given timestamp
 *
 * \param [in] secs_to_add - seconds that have to be added to given timestamp (+ or -)
 *
 * \param [in,out] timestamp - system time aquired from device's time storage/clock
 **********************************************************************************/
void add_time_to_timestamp(int32_t secs_to_add, struct tm *timestamp);

////////////////////////////////////////////////////////////////////////////
/// @brief Authorization password for PrusaLink
///
/// @return Return a password
const char *wui_get_password();

////////////////////////////////////////////////////////////////////////////
/// @brief Generate authorization password for PrusaLink
///
/// @param[out] buffer password buffer
/// @param[in] length Size of the buffer
void wui_generate_password(char *, uint32_t);

void wui_store_password(char *, uint32_t);

#ifdef __cplusplus
enum class StartPrintResult {
    Failed, /// uploading file failed
    Uploaded, /// uploading succeeded, able to print
    PrintStarted, /// uploading succeeded and print started immediately
};

/// Start a print of a given filename.
///
/// @param[in] autostart_if_able true  - printer will start print without asking (in case filament, printer type and other checks are satisfied)
///                              false - printer will not start print without asking, but it will show one click print if able to
StartPrintResult wui_start_print(char *filename, bool autostart_if_able);
#endif /* __cplusplus */

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
/// @brief initialize marlin client for tcpip thread
///
void wui_marlin_client_init(void);

////////////////////////////////////////////////////////////////////////////
/// @brief get system time
///
time_t sntp_get_system_time(void);

bool wui_is_file_being_printed(const char *filename);

bool wui_media_inserted();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
