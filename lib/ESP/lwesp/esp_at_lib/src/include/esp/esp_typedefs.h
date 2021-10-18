/**
 * \file            esp_typedefs.h
 * \brief           List of structures and enumerations for public usage
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#ifndef ESP_HDR_DEFS_H
#define ESP_HDR_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * \ingroup         ESP
 * \defgroup        ESP_TYPEDEFS Structures and enumerations
 * \brief           List of core structures and enumerations
 * \{
 */

/**
 * \}
 */

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           Result enumeration used across application functions
 */
typedef enum {
    espOK = 0,                                  /*!< Function succeeded */
    espOKIGNOREMORE,                            /*!< Function succedded, should continue as espOK but ignore sending more data. This result is possible on connection data receive callback */
    espERR,
    espPARERR,                                  /*!< Wrong parameters on function call */
    espERRMEM,                                  /*!< Memory error occurred */
    espTIMEOUT,                                 /*!< Timeout occurred on command */
    espCONT,                                    /*!< There is still some command to be processed in current command */
    espCLOSED,                                  /*!< Connection just closed */
    espINPROG,                                  /*!< Operation is in progress */

    espERRNOIP,                                 /*!< Station does not have IP address */
    espERRNOFREECONN,                           /*!< There is no free connection available to start */
    espERRCONNTIMEOUT,                          /*!< Timeout received when connection to access point */
    espERRPASS,                                 /*!< Invalid password for access point */
    espERRNOAP,                                 /*!< No access point found with specific SSID and MAC address */
    espERRCONNFAIL,                             /*!< Connection failed to access point */
    espERRWIFINOTCONNECTED,                     /*!< Wifi not connected to access point */
    espERRNODEVICE,                             /*!< Device is not present */
    espERRBLOCKING,                             /*!< Blocking mode command is not allowed */
} espr_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           List of support ESP devices by firmware
 */
typedef enum {
#if ESP_CFG_ESP8266 || __DOXYGEN__
    ESP_DEVICE_ESP8266,                         /*!< Device is ESP8266 */
#endif /* ESP_CFG_ESP8266 || __DOXYGEN__ */
#if ESP_CFG_ESP32 || __DOXYGEN__
    ESP_DEVICE_ESP32,                           /*!< Device is ESP32 */
#endif /* ESP_CFG_ESP32 || __DOXYGEN__ */
    ESP_DEVICE_UNKNOWN,                         /*!< Unknown device */
} esp_device_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           List of encryptions of access point
 */
typedef enum {
    ESP_ECN_OPEN = 0x00,                        /*!< No encryption on access point */
    ESP_ECN_WEP,                                /*!< WEP (Wired Equivalent Privacy) encryption */
    ESP_ECN_WPA_PSK,                            /*!< WPA (Wifi Protected Access) encryption */
    ESP_ECN_WPA2_PSK,                           /*!< WPA2 (Wifi Protected Access 2) encryption */
    ESP_ECN_WPA_WPA2_PSK,                       /*!< WPA/2 (Wifi Protected Access 1/2) encryption */
    ESP_ECN_WPA2_Enterprise                     /*!< Enterprise encryption. \note ESP is currently not able to connect to access point of this encryption type */
} esp_ecn_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           IP structure
 */
typedef struct {
    uint8_t ip[4];                              /*!< IPv4 address */
} esp_ip_t;

/**
 * \ingroup         ESP_UTILITIES
 * \brief           Set IP address to \ref esp_ip_t variable
 * \param[in]       ip_str: Pointer to IP structure
 * \param[in]       ip1,ip2,ip3,ip4: IPv4 parts
 */
#define ESP_SET_IP(ip_str, ip1, ip2, ip3, ip4)      do { (ip_str)->ip[0] = (ip1); (ip_str)->ip[1] = (ip2); (ip_str)->ip[2] = (ip3); (ip_str)->ip[3] = (ip4); } while (0)

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           Port variable
 */
typedef uint16_t    esp_port_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           MAC address
 */
typedef struct {
    uint8_t mac[6];                             /*!< MAC address */
} esp_mac_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           SW version in semantic versioning format
 */
typedef struct {
    uint8_t major;                              /*!< Major version */
    uint8_t minor;                              /*!< Minor version */
    uint8_t patch;                              /*!< Patch version */
} esp_sw_version_t;

/**
 * \ingroup         ESP_AP
 * \brief           Access point data structure
 */
typedef struct {
    esp_ecn_t ecn;                              /*!< Encryption mode */
    char ssid[ESP_CFG_MAX_SSID_LENGTH];         /*!< Access point name */
    int16_t rssi;                               /*!< Received signal strength indicator */
    esp_mac_t mac;                              /*!< MAC physical address */
    uint8_t ch;                                 /*!< WiFi channel used on access point */

    /* Not support for now */
    //int8_t offset;                              /*!< Access point offset */
    //uint8_t cal;                                /*!< Calibration value */
    uint8_t bgn;                                /*!< Information about 802.11[b|g|n] support */
    //uint8_t wps;                                /*!< Status if WPS function is supported */
} esp_ap_t;

/**
 * \ingroup         ESP_AP
 * \brief           Access point information on which station is connected to
 */
typedef struct {
    char ssid[ESP_CFG_MAX_SSID_LENGTH];         /*!< Access point name */
    int16_t rssi;                               /*!< RSSI */
    esp_mac_t mac;                              /*!< MAC address */
    uint8_t ch;                                 /*!< Channel information */
} esp_sta_info_ap_t;

/**
 * \ingroup         ESP_STA
 * \brief           Station data structure
 */
typedef struct {
    esp_ip_t ip;                                /*!< IP address of connected station */
    esp_mac_t mac;                              /*!< MAC address of connected station */
} esp_sta_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           Date and time structure
 */
typedef struct {
    uint8_t date;                               /*!< Day in a month, from 1 to up to 31 */
    uint8_t month;                              /*!< Month in a year, from 1 to 12 */
    uint16_t year;                              /*!< Year */
    uint8_t day;                                /*!< Day in a week, from 1 to 7 */
    uint8_t hours;                              /*!< Hours in a day, from 0 to 23 */
    uint8_t minutes;                            /*!< Minutes in a hour, from 0 to 59 */
    uint8_t seconds;                            /*!< Seconds in a minute, from 0 to 59 */
} esp_datetime_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           List of possible WiFi modes
 */
typedef enum {
#if ESP_CFG_MODE_STATION || __DOXYGEN__
    ESP_MODE_STA = 1,                           /*!< Set WiFi mode to station only */
#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_MODE_AP = 2,                            /*!< Set WiFi mode to access point only */
#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if ESP_CFG_MODE_STATION_ACCESS_POINT || __DOXYGEN__
    ESP_MODE_STA_AP = 3,                        /*!< Set WiFi mode to station and access point */
#endif /* (ESP_CFG_MODE_STATION_ACCESS_POINT) || __DOXYGEN__ */
} esp_mode_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           List of possible HTTP methods
 */
typedef enum {
    ESP_HTTP_METHOD_GET,                        /*!< HTTP method GET */
    ESP_HTTP_METHOD_HEAD,                       /*!< HTTP method HEAD */
    ESP_HTTP_METHOD_POST,                       /*!< HTTP method POST */
    ESP_HTTP_METHOD_PUT,                        /*!< HTTP method PUT */
    ESP_HTTP_METHOD_DELETE,                     /*!< HTTP method DELETE */
    ESP_HTTP_METHOD_CONNECT,                    /*!< HTTP method CONNECT */
    ESP_HTTP_METHOD_OPTIONS,                    /*!< HTTP method OPTIONS */
    ESP_HTTP_METHOD_TRACE,                      /*!< HTTP method TRACE */
    ESP_HTTP_METHOD_PATCH,                      /*!< HTTP method PATCH */
} esp_http_method_t;

/**
 * \ingroup         ESP_CONN
 * \brief           List of possible connection types
 */
typedef enum {
    ESP_CONN_TYPE_TCP,                          /*!< Connection type is TCP */
    ESP_CONN_TYPE_UDP,                          /*!< Connection type is UDP */
    ESP_CONN_TYPE_SSL,                          /*!< Connection type is SSL */
} esp_conn_type_t;

/* Forward declarations */
struct esp_evt;
struct esp_conn;
struct esp_pbuf;

/**
 * \ingroup         ESP_CONN
 * \brief           Pointer to \ref esp_conn_t structure
 */
typedef struct esp_conn* esp_conn_p;

/**
 * \ingroup         ESP_PBUF
 * \brief           Pointer to \ref esp_pbuf_t structure
 */
typedef struct esp_pbuf* esp_pbuf_p;

/**
 * \ingroup         ESP_EVT
 * \brief           Event function prototype
 * \param[in]       evt: Callback event data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
typedef espr_t  (*esp_evt_fn)(struct esp_evt* evt);

/**
 * \ingroup         ESP_EVT
 * \brief           List of possible callback types received to user
 */
typedef enum esp_evt_type_t {
    ESP_EVT_INIT_FINISH,                        /*!< Initialization has been finished at this point */

    ESP_EVT_RESET_DETECTED,                     /*!< Device reset detected */
    ESP_EVT_RESET,                              /*!< Device reset operation finished */
    ESP_EVT_RESTORE,                            /*!< Device restore operation finished */

    ESP_EVT_CMD_TIMEOUT,                        /*!< Timeout on command.
                                                        When application receives this event,
                                                        it may reset system as there was (maybe) a problem in device */

    ESP_EVT_DEVICE_PRESENT,                     /*!< Notification when device present status changes */

    ESP_EVT_AT_VERSION_NOT_SUPPORTED,           /*!< Library does not support firmware version on ESP device. */

    ESP_EVT_CONN_RECV,                          /*!< Connection data received */
    ESP_EVT_CONN_SEND,                          /*!< Connection data send */
    ESP_EVT_CONN_ACTIVE,                        /*!< Connection just became active */
    ESP_EVT_CONN_ERROR,                         /*!< Client connection start was not successful */
    ESP_EVT_CONN_CLOSE,                         /*!< Connection close event. Check status if successful */
    ESP_EVT_CONN_POLL,                          /*!< Poll for connection if there are any changes */

    ESP_EVT_SERVER,                             /*!< Server status changed */

#if ESP_CFG_MODE_STATION || __DOXYGEN__
    ESP_EVT_WIFI_CONNECTED,                     /*!< Station just connected to AP */
    ESP_EVT_WIFI_GOT_IP,                        /*!< Station has valid IP.
                                                    When this event is received to application, no IP has been read from device.
                                                    Stack will proceed with IP read from device and will later send \ref ESP_EVT_WIFI_IP_ACQUIRED event */
    ESP_EVT_WIFI_DISCONNECTED,                  /*!< Station just disconnected from AP */
    ESP_EVT_WIFI_IP_ACQUIRED,                   /*!< Station IP address acquired.
                                                    At this point, valid IP address has been received from device.
                                                    Application may use \ref esp_sta_copy_ip function to read it */

    ESP_EVT_STA_LIST_AP,                        /*!< Station listed APs event */
    ESP_EVT_STA_JOIN_AP,                        /*!< Join to access point */
    ESP_EVT_STA_INFO_AP,                        /*!< Station AP info (name, mac, channel, rssi) */
#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_EVT_AP_CONNECTED_STA,                   /*!< New station just connected to ESP's access point */
    ESP_EVT_AP_DISCONNECTED_STA,                /*!< New station just disconnected from ESP's access point */
    ESP_EVT_AP_IP_STA,                          /*!< New station just received IP from ESP's access point */
#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if ESP_CFG_DNS || __DOXYGEN__
    ESP_EVT_DNS_HOSTBYNAME,                     /*!< DNS domain service finished */
#endif /* ESP_CFG_DNS || __DOXYGEN__ */
#if ESP_CFG_PING || __DOXYGEN__
    ESP_EVT_PING,                               /*!< PING service finished */
#endif /* ESP_CFG_PING || __DOXYGEN__ */
} esp_evt_type_t;

/**
 * \ingroup         ESP_EVT
 * \brief           Global callback structure to pass as parameter to callback function
 */
typedef struct esp_evt {
    esp_evt_type_t type;                        /*!< Callback type */
    union {
        struct {
            uint8_t forced;                     /*!< Set to `1` if reset forced by user */
        } reset_detected;                       /*!< Reset occurred. Use with \ref ESP_EVT_RESET_DETECTED event */

        struct {
            espr_t res;                         /*!< Reset operation result */
        } reset;                                /*!< Reset sequence finish. Use with \ref ESP_EVT_RESET event */
        struct {
            espr_t res;                         /*!< Restore operation result */
        } restore;                              /*!< Restore sequence finish. Use with \ref ESP_EVT_RESTORE event */

        struct {
            esp_conn_p conn;                    /*!< Connection where data were received */
            esp_pbuf_p buff;                    /*!< Pointer to received data */
        } conn_data_recv;                       /*!< Network data received. Use with \ref ESP_EVT_CONN_RECV event */
        struct {
            esp_conn_p conn;                    /*!< Connection where data were sent */
            size_t sent;                        /*!< Number of bytes sent on connection */
            espr_t res;                         /*!< Send data result */
        } conn_data_send;                       /*!< Data send. Use with \ref ESP_EVT_CONN_SEND event */
        struct {
            const char* host;                   /*!< Host to use for connection */
            esp_port_t port;                    /*!< Remote port used for connection */
            esp_conn_type_t type;               /*!< Connection type */
            void* arg;                          /*!< Connection user argument */
            espr_t err;                         /*!< Error value */
        } conn_error;                           /*!< Client connection start error. Use with \ref ESP_EVT_CONN_ERROR event */
        struct {
            esp_conn_p conn;                    /*!< Pointer to connection */
            uint8_t client;                     /*!< Set to 1 if connection is/was client mode */
            uint8_t forced;                     /*!< Set to 1 if connection action was forced (when active: 1 = CLIENT, 0 = SERVER: when closed, 1 = CMD, 0 = REMOTE) */
            espr_t res;                         /*!< Result of close event. Set to \ref espOK on success */
        } conn_active_close;                    /*!< Process active and closed statuses at the same time. Use with \ref ESP_EVT_CONN_ACTIVE or \ref ESP_EVT_CONN_CLOSE events */
        struct {
            esp_conn_p conn;                    /*!< Set connection pointer */
        } conn_poll;                            /*!< Polling active connection to check for timeouts. Use with \ref ESP_EVT_CONN_POLL event */

        struct {
            espr_t res;                         /*!< Status of command */
            uint8_t en;                         /*!< Status to enable/disable server */
            esp_port_t port;                    /*!< Server port number */
        } server;                               /*!< Server change event. Use with \ref ESP_EVT_SERVER event */
#if ESP_CFG_MODE_STATION || __DOXYGEN__
        struct {
            espr_t res;                         /*!< Result of command */
            esp_ap_t* aps;                      /*!< Pointer to access points */
            size_t len;                         /*!< Number of access points found */
        } sta_list_ap;                          /*!< Station list access points. Use with \ref ESP_EVT_STA_LIST_AP event */
        struct {
            espr_t res;                         /*!< Result of command */
        } sta_join_ap;                          /*!< Join to access point. Use with \ref ESP_EVT_STA_JOIN_AP event */
        struct {
            esp_sta_info_ap_t* info;            /*!< AP info of current station */
            espr_t res;                         /*!< Result of command */
        } sta_info_ap;                          /*!< Current AP informations. Use with \ref ESP_EVT_STA_INFO_AP event */
#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
        struct {
            esp_mac_t* mac;                     /*!< Station MAC address */
        } ap_conn_disconn_sta;                  /*!< A new station connected or disconnected to ESP's access point. Use with \ref ESP_EVT_AP_CONNECTED_STA or \ref ESP_EVT_AP_DISCONNECTED_STA events */
        struct {
            esp_mac_t* mac;                     /*!< Station MAC address */
            esp_ip_t* ip;                       /*!< Station IP address */
        } ap_ip_sta;                            /*!< Station got IP address from ESP's access point. Use with \ref ESP_EVT_AP_IP_STA event */
#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if ESP_CFG_DNS || __DOXYGEN__
        struct {
            espr_t res;                         /*!< Result of command */
            const char* host;                   /*!< Host name for DNS lookup */
            esp_ip_t* ip;                       /*!< Pointer to IP result */
        } dns_hostbyname;                       /*!< DNS domain service finished. Use with \ref ESP_EVT_DNS_HOSTBYNAME event */
#endif /* ESP_CFG_DNS || __DOXYGEN__ */
#if ESP_CFG_PING || __DOXYGEN__
        struct {
            espr_t res;                         /*!< Result of command */
            const char* host;                   /*!< Host name for ping */
            uint32_t time;                      /*!< Time required for ping. Valid only if operation succedded */
        } ping;                                 /*!< Ping finished. Use with \ref ESP_EVT_PING event */
#endif /* ESP_CFG_PING || __DOXYGEN__ */
    } evt;                                      /*!< Callback event union */
} esp_evt_t;

#define ESP_SIZET_MAX                           ((size_t)(-1))  /*!< Maximal value of size_t variable type */

/**
 * \ingroup         ESP_LL
 * \brief           Function prototype for AT output data
 * \param[in]       data: Pointer to data to send. This parameter can be set to `NULL`
 * \param[in]       len: Number of bytes to send. This parameter can be set to `0`
 *                      to indicate that internal buffer can be flushed to stream.
 *                      This is implementation defined and feature might be ignored
 * \return          Number of bytes sent
 */
typedef size_t (*esp_ll_send_fn)(const void* data, size_t len);

/**
 * \ingroup         ESP_LL
 * \brief           Function prototype for hardware reset of ESP device
 * \param[in]       state: State indicating reset. When set to `1`, reset must be active (usually pin active low),
 *                      or set to `0` when reset is cleared
 * \return          `1` on successful action, `0` otherwise
 */
typedef uint8_t (*esp_ll_reset_fn)(uint8_t state);

/**
 * \ingroup         ESP_LL
 * \brief           Low level user specific functions
 */
typedef struct {
    esp_ll_send_fn send_fn;                     /*!< Callback function to transmit data */
    esp_ll_reset_fn reset_fn;                   /*!< Reset callback function */
    struct {
        uint32_t baudrate;                      /*!< UART baudrate value */
    } uart;                                     /*!< UART communication parameters */
} esp_ll_t;

/**
 * \ingroup         ESP_TIMEOUT
 * \brief           Timeout callback function prototype
 * \param[in]       arg: Custom user argument
 */
typedef void (*esp_timeout_fn)(void* arg);

/**
 * \ingroup         ESP_TIMEOUT
 * \brief           Timeout structure
 */
typedef struct esp_timeout {
    struct esp_timeout* next;                   /*!< Pointer to next timeout entry */
    uint32_t time;                              /*!< Time difference from previous entry */
    void* arg;                                  /*!< Argument to pass to callback function */
    esp_timeout_fn fn;                          /*!< Callback function for timeout */
} esp_timeout_t;

/**
 * \ingroup         ESP_BUFF
 * \brief           Buffer structure
 */
typedef struct {
    uint8_t* buff;                              /*!< Pointer to buffer data.
                                                    Buffer is considered initialized when `buff != NULL` */
    size_t size;                                /*!< Size of buffer data. Size of actual buffer is `1` byte less than this value */
    size_t r;                                   /*!< Next read pointer. Buffer is considered empty when `r == w` and full when `w == r - 1` */
    size_t w;                                   /*!< Next write pointer. Buffer is considered empty when `r == w` and full when `w == r - 1` */
} esp_buff_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           Linear buffer structure
 */
typedef struct {
    uint8_t* buff;                              /*!< Pointer to buffer data array */
    size_t len;                                 /*!< Length of buffer array */
    size_t ptr;                                 /*!< Current buffer pointer */
} esp_linbuff_t;

/**
 * \ingroup         ESP_TYPEDEFS
 * \brief           Function declaration for API function command event callback function
 * \param[in]       res: Operation result, member of \ref espr_t enumeration
 * \param[in]       arg: Custom user argument
 */
typedef void (*esp_api_cmd_evt_fn) (espr_t res, void* arg);

/**
 * \ingroup         ESP_CONN
 * \brief           Connection start structure, used to start the connection in extended mode
 */
typedef struct {
    esp_conn_type_t type;                       /*!< Connection type */
    const char* remote_host;                    /*!< Host name or IP address in string format */
    esp_port_t remote_port;                     /*!< Remote server port */
    const char* local_ip;                       /*!< Local IP. Optional parameter, set to NULL if not used (most cases) */
    union {
        struct {
            uint16_t keep_alive;                /*!< Keep alive parameter for TCP/SSL connection in units of seconds.
                                                    Value can be between `0 - 7200` where `0` means no keep alive */
        } tcp_ssl;                              /*!< TCP/SSL specific features */
        struct {
            esp_port_t local_port;              /*!< Custom local port for UDP */
            uint8_t mode;                       /*!< UDP mode. Set to `0` by default. Check ESP AT commands instruction set for more info when needed */
        } udp;
    } ext;                                      /*!< Extended support union */
} esp_conn_start_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESP_HDR_DEFS_H */
