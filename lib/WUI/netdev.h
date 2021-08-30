#pragma once

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
/// @param[in] dev_id device ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_up(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Turn down given network device
///
/// @param[in] dev_id device ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_down(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Obtaining ip from DHCP server
///
/// @param[in] dev_id device ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_dhcp(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Obtaining static ip
///
/// @param[in] dev_id device ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_static(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Return device id which is currently active for communication
///
/// @return network device id
uint32_t netdev_get_active_id();

////////////////////////////////////////////////////////////////////////////
/// @brief Set network device for communication
///
/// @param[in] dev_id device ID
/// @return 0 on success; error otherwise
uint32_t netdev_set_active_id(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Set the the given network device state if is plugged or unplugged
///
/// @param[in] dev_id device ID
/// @return 0
uint32_t netdev_check_link(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrive status of the given network device
///
/// @param[in] dev_id device ID
/// @return device status
netdev_status_t netdev_get_status(uint32_t);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrive method used to obtain ip address on active network device
///
/// @return ip obtaining method type
netdev_ip_obtained_t netdev_get_ip_obtained_type();

#ifdef __cplusplus
}
#endif // __cplusplus
