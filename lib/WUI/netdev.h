/*
 * This file is part of the Prusa Firmware Buddy distribution (https://github.com/prusa3d/Prusa-Firmware-Buddy ).
 * Copyright (c) 2021 Marek Mosna.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////
/// @brief Possible states of the network device
///
typedef enum {
    NETDEV_UNLINKED,
    NETDEV_NETIF_DOWN,
    NETDEV_NETIF_UP
} netdev_status_t;

////////////////////////////////////////////////////////////////////////////
/// @brief Possible ip obtaining methods
///
typedef enum {
    NETDEV_DHCP,
    NETDEV_STATIC
} netdev_ip_obtained_t;

#define NETDEV_ETH_ID   0UL
#define NETDEV_ESP_ID   1UL
#define NETDEV_NODEV_ID 2UL
#define NETDEV_COUNT    NETDEV_NODEV_ID

#define create_evt_eth(dev, flag, value) (((dev) << 28 | (flag) << 16) | (value))

#define EVT_TCPIP_INIT_FINISHED              (0xF0000000UL)
#define EVT_LWESP_INIT_FINISHED              (0xE0000000UL)
#define EVT_NETDEV_INIT_FINISHED(dev, value) create_evt_eth((dev), 0x0FFF, (value))

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////
/// @brief Initialize network layer
///
/// @return 0 on success; error otherwise
uint32_t netdev_init();

////////////////////////////////////////////////////////////////////////////
/// @brief Initialize ESP network layer
///
/// @return 0 on success; error otherwise
uint32_t netdev_init_esp();

////////////////////////////////////////////////////////////////////////////
/// @brief Turn up given network device
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_up(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Turn down given network device
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_down(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Obtaining ip from DHCP server
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_dhcp(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Obtaining static ip
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_static(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Return device id which is currently active for communication
///
/// @return network device id
uint32_t netdev_get_active_id();

/**
 * \brief Get the currently used (primary) printer IP address.
 *
 * This provides the IPv4 address the printer currently uses, if any. In case
 * the printer has multiple IPs (once we have support for having multiple
 * interfaces up or having multiple IPs on a single interface), this shall
 * provide the „default“ one ‒ the one the printer would use for outgoing
 * requests.
 *
 * This may fail in case there's no IPv4 address used by the printer. Such
 * thing can (or will be able to) happen for reasons including:
 * - No interface is up.
 * - An interface is up, but hasn't yet receved an IP address from DHCP.
 * - We currently live in an IPv6-only network.
 *
 * Note: Yes, we are currently missing the IPv6 equivalent. Sorry, we are not
 * there yet.
 *
 * \param [out] dest - an 4-element array to put the IP into, in network order.
 * \return If any address was filled in (false in case it failed).
 */
bool get_current_ipv4(uint8_t *dest);

////////////////////////////////////////////////////////////////////////////
/// @brief Set network device for communication
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_active_id(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Set the the given network device state if is plugged or unplugged
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return 0
uint32_t netdev_check_link(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrive status of the given network device
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return device status
netdev_status_t netdev_get_status(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrive method used to obtain ip address on active network device
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return ip obtaining method type
netdev_ip_obtained_t netdev_get_ip_obtained_type(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrive hostname of active device
///
/// @param[in] dev_id device ID. One of
///             - #NETDEV_ETH_ID
///             - #NETDEV_ESP_ID
/// @return hostname
const char *netdev_get_hostname(uint32_t);

/// Load ini file to both runtime and eeprom configuration.
///
/// @return If it was successful.
bool netdev_load_ini_to_eeprom();

#ifdef __cplusplus
}
#endif // __cplusplus
