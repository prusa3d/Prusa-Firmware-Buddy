/**
 * \file            lwesp_typedefs.h
 * \brief           List of structures and enumerations for public usage
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#ifndef LWESP_HDR_DEFS_H
#define LWESP_HDR_DEFS_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP
 * \defgroup        LWESP_TYPEDEFS Structures and enumerations
 * \brief           List of core structures and enumerations
 * \{
 */

/**
 * \}
 */

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           Result enumeration used across application functions
 */
typedef enum {
    lwespOK = 0,                                /*!< Function succeeded */
    lwespOKIGNOREMORE,                          /*!< Function succedded, should continue as lwespOK but ignore sending more data. This result is possible on connection data receive callback */
    lwespERR,
    lwespPARERR,                                /*!< Wrong parameters on function call */
    lwespERRMEM,                                /*!< Memory error occurred */
    lwespTIMEOUT,                               /*!< Timeout occurred on command */
    lwespCONT,                                  /*!< There is still some command to be processed in current command */
    lwespCLOSED,                                /*!< Connection just closed */
    lwespINPROG,                                /*!< Operation is in progress */

    lwespERRNOIP,                               /*!< Station does not have IP address */
    lwespERRNOFREECONN,                         /*!< There is no free connection available to start */
    lwespERRCONNTIMEOUT,                        /*!< Timeout received when connection to access point */
    lwespERRPASS,                               /*!< Invalid password for access point */
    lwespERRNOAP,                               /*!< No access point found with specific SSID and MAC address */
    lwespERRCONNFAIL,                           /*!< Connection failed to access point */
    lwespERRWIFINOTCONNECTED,                   /*!< Wifi not connected to access point */
    lwespERRNODEVICE,                           /*!< Device is not present */
    lwespERRBLOCKING,                           /*!< Blocking mode command is not allowed */
} lwespr_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           List of support ESP devices by firmware
 */
typedef enum {
#if LWESP_CFG_ESP8266 || __DOXYGEN__
    LWESP_DEVICE_ESP8266,                       /*!< Device is ESP8266 */
#endif /* LWESP_CFG_ESP8266 || __DOXYGEN__ */
#if LWESP_CFG_ESP32 || __DOXYGEN__
    LWESP_DEVICE_ESP32,                         /*!< Device is ESP32 */
#endif /* LWESP_CFG_ESP32 || __DOXYGEN__ */
    LWESP_DEVICE_UNKNOWN,                       /*!< Unknown device */
} lwesp_device_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           List of encryptions of access point
 */
typedef enum {
    LWESP_ECN_OPEN = 0x00,                      /*!< No encryption on access point */
    LWESP_ECN_WEP,                              /*!< WEP (Wired Equivalent Privacy) encryption */
    LWESP_ECN_WPA_PSK,                          /*!< WPA (Wifi Protected Access) encryption */
    LWESP_ECN_WPA2_PSK,                         /*!< WPA2 (Wifi Protected Access 2) encryption */
    LWESP_ECN_WPA_WPA2_PSK,                     /*!< WPA/2 (Wifi Protected Access 1/2) encryption */
    LWESP_ECN_WPA2_Enterprise                   /*!< Enterprise encryption. \note ESP is currently not able to connect to access point of this encryption type */
} lwesp_ecn_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           IP structure
 */
typedef struct {
    uint8_t ip[4];                              /*!< IPv4 address */
} lwesp_ip_t;

/**
 * \ingroup         LWESP_UTILITIES
 * \brief           Set IP address to \ref lwesp_ip_t variable
 * \param[in]       ip_str: Pointer to IP structure
 * \param[in]       ip1,ip2,ip3,ip4: IPv4 parts
 */
#define LWESP_SET_IP(ip_str, ip1, ip2, ip3, ip4)      do { (ip_str)->ip[0] = (ip1); (ip_str)->ip[1] = (ip2); (ip_str)->ip[2] = (ip3); (ip_str)->ip[3] = (ip4); } while (0)

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           Port variable
 */
typedef uint16_t    lwesp_port_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           MAC address
 */
typedef struct {
    uint8_t mac[6];                             /*!< MAC address */
} lwesp_mac_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           SW version in semantic versioning format
 */
typedef struct {
    uint8_t major;                              /*!< Major version */
    uint8_t minor;                              /*!< Minor version */
    uint8_t patch;                              /*!< Patch version */
} lwesp_sw_version_t;

/**
 * \ingroup         LWESP_AP
 * \brief           Access point data structure
 */
typedef struct {
    lwesp_ecn_t ecn;                            /*!< Encryption mode */
    char ssid[LWESP_CFG_MAX_SSID_LENGTH];       /*!< Access point name */
    int16_t rssi;                               /*!< Received signal strength indicator */
    lwesp_mac_t mac;                            /*!< MAC physical address */
    uint8_t ch;                                 /*!< WiFi channel used on access point */

    /* Not support for now */
    //int8_t offset;                            /*!< Access point offset */
    //uint8_t cal;                              /*!< Calibration value */
    uint8_t bgn;                                /*!< Information about 802.11[b|g|n] support */
    //uint8_t wps;                              /*!< Status if WPS function is supported */
} lwesp_ap_t;

/**
 * \ingroup         LWESP_AP
 * \brief           Access point information on which station is connected to
 */
typedef struct {
    char ssid[LWESP_CFG_MAX_SSID_LENGTH];       /*!< Access point name */
    int16_t rssi;                               /*!< RSSI */
    lwesp_mac_t mac;                            /*!< MAC address */
    uint8_t ch;                                 /*!< Channel information */
} lwesp_sta_info_ap_t;

/**
 * \ingroup         LWESP_AP
 * \brief           Soft access point data structure
 */
typedef struct {
    char ssid[LWESP_CFG_MAX_SSID_LENGTH];       /*!< Access point name */
    char pwd[LWESP_CFG_MAX_PWD_LENGTH];         /*!< Access point password/passphrase */
    uint8_t ch;                                 /*!< WiFi channel used on access point */
    lwesp_ecn_t ecn;                            /*!< Encryption mode */
    uint8_t max_cons;                           /*!< Maximum number of stations allowed connected to this AP */
    uint8_t hidden;                             /*!< broadcast the SSID, 0 -- No, 1 -- Yes */
} lwesp_ap_conf_t;

/**
 * \ingroup         LWESP_STA
 * \brief           Station data structure
 */
typedef struct {
    lwesp_ip_t ip;                              /*!< IP address of connected station */
    lwesp_mac_t mac;                            /*!< MAC address of connected station */
} lwesp_sta_t;

/**
 * \ingroup         LWESP_TYPEDEFS
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
} lwesp_datetime_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           List of possible WiFi modes
 */
typedef enum {
#if LWESP_CFG_MODE_STATION || __DOXYGEN__
    LWESP_MODE_STA = 1,                         /*!< Set WiFi mode to station only */
#endif /* LWESP_CFG_MODE_STATION || __DOXYGEN__ */
#if LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    LWESP_MODE_AP = 2,                          /*!< Set WiFi mode to access point only */
#endif /* LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if LWESP_CFG_MODE_STATION_ACCESS_POINT || __DOXYGEN__
    LWESP_MODE_STA_AP = 3,                      /*!< Set WiFi mode to station and access point */
#endif /* (LWESP_CFG_MODE_STATION_ACCESS_POINT) || __DOXYGEN__ */
} lwesp_mode_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           List of possible HTTP methods
 */
typedef enum {
    LWESP_HTTP_METHOD_GET,                      /*!< HTTP method GET */
    LWESP_HTTP_METHOD_HEAD,                     /*!< HTTP method HEAD */
    LWESP_HTTP_METHOD_POST,                     /*!< HTTP method POST */
    LWESP_HTTP_METHOD_PUT,                      /*!< HTTP method PUT */
    LWESP_HTTP_METHOD_DELETE,                   /*!< HTTP method DELETE */
    LWESP_HTTP_METHOD_CONNECT,                  /*!< HTTP method CONNECT */
    LWESP_HTTP_METHOD_OPTIONS,                  /*!< HTTP method OPTIONS */
    LWESP_HTTP_METHOD_TRACE,                    /*!< HTTP method TRACE */
    LWESP_HTTP_METHOD_PATCH,                    /*!< HTTP method PATCH */
} lwesp_http_method_t;

/**
 * \ingroup         LWESP_CONN
 * \brief           List of possible connection types
 */
typedef enum {
    LWESP_CONN_TYPE_TCP,                        /*!< Connection type is TCP */
    LWESP_CONN_TYPE_UDP,                        /*!< Connection type is UDP */
    LWESP_CONN_TYPE_SSL,                        /*!< Connection type is SSL */
} lwesp_conn_type_t;

/* Forward declarations */
struct lwesp_evt;
struct lwesp_conn;
struct lwesp_pbuf;

/**
 * \ingroup         LWESP_CONN
 * \brief           Pointer to \ref lwesp_conn_t structure
 */
typedef struct lwesp_conn* lwesp_conn_p;

/**
 * \ingroup         LWESP_PBUF
 * \brief           Pointer to \ref lwesp_pbuf_t structure
 */
typedef struct lwesp_pbuf* lwesp_pbuf_p;

/**
 * \ingroup         LWESP_EVT
 * \brief           Event function prototype
 * \param[in]       evt: Callback event data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
typedef lwespr_t  (*lwesp_evt_fn)(struct lwesp_evt* evt);

/**
 * \ingroup         LWESP_EVT
 * \brief           List of possible callback types received to user
 */
typedef enum lwesp_evt_type_t {
    LWESP_EVT_INIT_FINISH,                      /*!< Initialization has been finished at this point */

    LWESP_EVT_RESET_DETECTED,                   /*!< Device reset detected */
    LWESP_EVT_RESET,                            /*!< Device reset operation finished */
    LWESP_EVT_RESTORE,                          /*!< Device restore operation finished */

    LWESP_EVT_CMD_TIMEOUT,                      /*!< Timeout on command.
                                                        When application receives this event,
                                                        it may reset system as there was (maybe) a problem in device */

    LWESP_EVT_DEVICE_PRESENT,                   /*!< Notification when device present status changes */

    LWESP_EVT_AT_VERSION_NOT_SUPPORTED,         /*!< Library does not support firmware version on ESP device. */

    LWESP_EVT_CONN_RECV,                        /*!< Connection data received */
    LWESP_EVT_CONN_SEND,                        /*!< Connection data send */
    LWESP_EVT_CONN_ACTIVE,                      /*!< Connection just became active */
    LWESP_EVT_CONN_ERROR,                       /*!< Client connection start was not successful */
    LWESP_EVT_CONN_CLOSE,                       /*!< Connection close event. Check status if successful */
    LWESP_EVT_CONN_POLL,                        /*!< Poll for connection if there are any changes */

    LWESP_EVT_SERVER,                           /*!< Server status changed */

#if LWESP_CFG_MODE_STATION || __DOXYGEN__
    LWESP_EVT_WIFI_CONNECTED,                   /*!< Station just connected to AP */
    LWESP_EVT_WIFI_GOT_IP,                      /*!< Station has valid IP.
                                                    When this event is received to application, no IP has been read from device.
                                                    Stack will proceed with IP read from device and will later send \ref LWESP_EVT_WIFI_IP_ACQUIRED event */
    LWESP_EVT_WIFI_DISCONNECTED,                /*!< Station just disconnected from AP */
    LWESP_EVT_WIFI_IP_ACQUIRED,                 /*!< Station IP address acquired.
                                                    At this point, valid IP address has been received from device.
                                                    Application may use \ref lwesp_sta_copy_ip function to read it */

    LWESP_EVT_STA_LIST_AP,                      /*!< Station listed APs event */
    LWESP_EVT_STA_JOIN_AP,                      /*!< Join to access point */
    LWESP_EVT_STA_INFO_AP,                      /*!< Station AP info (name, mac, channel, rssi) */
#endif /* LWESP_CFG_MODE_STATION || __DOXYGEN__ */
#if LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    LWESP_EVT_AP_CONNECTED_STA,                 /*!< New station just connected to ESP's access point */
    LWESP_EVT_AP_DISCONNECTED_STA,              /*!< New station just disconnected from ESP's access point */
    LWESP_EVT_AP_IP_STA,                        /*!< New station just received IP from ESP's access point */
#endif /* LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if LWESP_CFG_DNS || __DOXYGEN__
    LWESP_EVT_DNS_HOSTBYNAME,                   /*!< DNS domain service finished */
#endif /* LWESP_CFG_DNS || __DOXYGEN__ */
#if LWESP_CFG_PING || __DOXYGEN__
    LWESP_EVT_PING,                             /*!< PING service finished */
#endif /* LWESP_CFG_PING || __DOXYGEN__ */
} lwesp_evt_type_t;

/**
 * \ingroup         LWESP_EVT
 * \brief           Global callback structure to pass as parameter to callback function
 */
typedef struct lwesp_evt {
    lwesp_evt_type_t type;                      /*!< Callback type */
    union {
        struct {
            uint8_t forced;                     /*!< Set to `1` if reset forced by user */
        } reset_detected;                       /*!< Reset occurred. Use with \ref LWESP_EVT_RESET_DETECTED event */

        struct {
            lwespr_t res;                       /*!< Reset operation result */
        } reset;                                /*!< Reset sequence finish. Use with \ref LWESP_EVT_RESET event */
        struct {
            lwespr_t res;                       /*!< Restore operation result */
        } restore;                              /*!< Restore sequence finish. Use with \ref LWESP_EVT_RESTORE event */

        struct {
            lwesp_conn_p conn;                  /*!< Connection where data were received */
            lwesp_pbuf_p buff;                  /*!< Pointer to received data */
        } conn_data_recv;                       /*!< Network data received. Use with \ref LWESP_EVT_CONN_RECV event */
        struct {
            lwesp_conn_p conn;                  /*!< Connection where data were sent */
            size_t sent;                        /*!< Number of bytes sent on connection */
            lwespr_t res;                       /*!< Send data result */
        } conn_data_send;                       /*!< Data send. Use with \ref LWESP_EVT_CONN_SEND event */
        struct {
            const char* host;                   /*!< Host to use for connection */
            lwesp_port_t port;                  /*!< Remote port used for connection */
            lwesp_conn_type_t type;             /*!< Connection type */
            void* arg;                          /*!< Connection user argument */
            lwespr_t err;                       /*!< Error value */
        } conn_error;                           /*!< Client connection start error. Use with \ref LWESP_EVT_CONN_ERROR event */
        struct {
            lwesp_conn_p conn;                  /*!< Pointer to connection */
            uint8_t client;                     /*!< Set to 1 if connection is/was client mode */
            uint8_t forced;                     /*!< Set to 1 if connection action was forced
                                                        when active: 1 = CLIENT, 0 = SERVER
                                                        when closed, 1 = CMD, 0 = REMOTE */
            lwespr_t res;                       /*!< Result of close event.
                                                        Set to \ref lwespOK on success */
        } conn_active_close;                    /*!< Process active and closed statuses at the same time.
                                                        Use with \ref LWESP_EVT_CONN_ACTIVE or
                                                        \ref LWESP_EVT_CONN_CLOSE events */
        struct {
            lwesp_conn_p conn;                  /*!< Set connection pointer */
        } conn_poll;                            /*!< Polling active connection to check for timeouts.
                                                        Use with \ref LWESP_EVT_CONN_POLL event */

        struct {
            lwespr_t res;                       /*!< Status of command */
            uint8_t en;                         /*!< Status to enable/disable server */
            lwesp_port_t port;                  /*!< Server port number */
        } server;                               /*!< Server change event. Use with \ref LWESP_EVT_SERVER event */
#if LWESP_CFG_MODE_STATION || __DOXYGEN__
        struct {
            lwespr_t res;                       /*!< Result of command */
            lwesp_ap_t* aps;                    /*!< Pointer to access points */
            size_t len;                         /*!< Number of access points found */
        } sta_list_ap;                          /*!< Station list access points. Use with \ref LWESP_EVT_STA_LIST_AP event */
        struct {
            lwespr_t res;                       /*!< Result of command */
        } sta_join_ap;                          /*!< Join to access point. Use with \ref LWESP_EVT_STA_JOIN_AP event */
        struct {
            lwesp_sta_info_ap_t* info;          /*!< AP info of current station */
            lwespr_t res;                       /*!< Result of command */
        } sta_info_ap;                          /*!< Current AP informations. Use with \ref LWESP_EVT_STA_INFO_AP event */
#endif /* LWESP_CFG_MODE_STATION || __DOXYGEN__ */
#if LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
        struct {
            lwesp_mac_t* mac;                   /*!< Station MAC address */
        } ap_conn_disconn_sta;                  /*!< A new station connected or disconnected to
                                                        ESP's access point. Use with
                                                        \ref LWESP_EVT_AP_CONNECTED_STA or
                                                        \ref LWESP_EVT_AP_DISCONNECTED_STA events */
        struct {
            lwesp_mac_t* mac;                   /*!< Station MAC address */
            lwesp_ip_t* ip;                     /*!< Station IP address */
        } ap_ip_sta;                            /*!< Station got IP address from ESP's access point.
                                                        Use with \ref LWESP_EVT_AP_IP_STA event */
#endif /* LWESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if LWESP_CFG_DNS || __DOXYGEN__
        struct {
            lwespr_t res;                       /*!< Result of command */
            const char* host;                   /*!< Host name for DNS lookup */
            lwesp_ip_t* ip;                     /*!< Pointer to IP result */
        } dns_hostbyname;                       /*!< DNS domain service finished.
                                                        Use with \ref LWESP_EVT_DNS_HOSTBYNAME event */
#endif /* LWESP_CFG_DNS || __DOXYGEN__ */
#if LWESP_CFG_PING || __DOXYGEN__
        struct {
            lwespr_t res;                       /*!< Result of command */
            const char* host;                   /*!< Host name for ping */
            uint32_t time;                      /*!< Time required for ping. Valid only if operation succedded */
        } ping;                                 /*!< Ping finished. Use with \ref LWESP_EVT_PING event */
#endif /* LWESP_CFG_PING || __DOXYGEN__ */
    } evt;                                      /*!< Callback event union */
} lwesp_evt_t;

#define LWESP_SIZET_MAX                         ((size_t)(-1))  /*!< Maximal value of size_t variable type */

/**
 * \ingroup         LWESP_LL
 * \brief           Function prototype for AT output data
 * \param[in]       data: Pointer to data to send. This parameter can be set to `NULL`
 * \param[in]       len: Number of bytes to send. This parameter can be set to `0`
 *                      to indicate that internal buffer can be flushed to stream.
 *                      This is implementation defined and feature might be ignored
 * \return          Number of bytes sent
 */
typedef size_t (*lwesp_ll_send_fn)(const void* data, size_t len);

/**
 * \ingroup         LWESP_LL
 * \brief           Function prototype for hardware reset of ESP device
 * \param[in]       state: State indicating reset. When set to `1`, reset must be active (usually pin active low),
 *                      or set to `0` when reset is cleared
 * \return          `1` on successful action, `0` otherwise
 */
typedef uint8_t (*lwesp_ll_reset_fn)(uint8_t state);

/**
 * \ingroup         LWESP_LL
 * \brief           Low level user specific functions
 */
typedef struct {
    lwesp_ll_send_fn send_fn;                   /*!< Callback function to transmit data */
    lwesp_ll_reset_fn reset_fn;                 /*!< Reset callback function */
    struct {
        uint32_t baudrate;                      /*!< UART baudrate value */
    } uart;                                     /*!< UART communication parameters */
} lwesp_ll_t;

/**
 * \ingroup         LWESP_TIMEOUT
 * \brief           Timeout callback function prototype
 * \param[in]       arg: Custom user argument
 */
typedef void (*lwesp_timeout_fn)(void* arg);

/**
 * \ingroup         LWESP_TIMEOUT
 * \brief           Timeout structure
 */
typedef struct lwesp_timeout {
    struct lwesp_timeout* next;                 /*!< Pointer to next timeout entry */
    uint32_t time;                              /*!< Time difference from previous entry */
    void* arg;                                  /*!< Argument to pass to callback function */
    lwesp_timeout_fn fn;                        /*!< Callback function for timeout */
} lwesp_timeout_t;

/**
 * \ingroup         LWESP_BUFF
 * \brief           Buffer structure
 */
typedef struct {
    uint8_t* buff;                              /*!< Pointer to buffer data.
                                                    Buffer is considered initialized when `buff != NULL` */
    size_t size;                                /*!< Size of buffer data. Size of actual buffer is
                                                        `1` byte less than this value */
    size_t r;                                   /*!< Next read pointer. Buffer is considered empty
                                                        when `r == w` and full when `w == r - 1` */
    size_t w;                                   /*!< Next write pointer. Buffer is considered empty
                                                        when `r == w` and full when `w == r - 1` */
} lwesp_buff_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           Linear buffer structure
 */
typedef struct {
    uint8_t* buff;                              /*!< Pointer to buffer data array */
    size_t len;                                 /*!< Length of buffer array */
    size_t ptr;                                 /*!< Current buffer pointer */
} lwesp_linbuff_t;

/**
 * \ingroup         LWESP_TYPEDEFS
 * \brief           Function declaration for API function command event callback function
 * \param[in]       res: Operation result, member of \ref lwespr_t enumeration
 * \param[in]       arg: Custom user argument
 */
typedef void (*lwesp_api_cmd_evt_fn) (lwespr_t res, void* arg);

/**
 * \ingroup         LWESP_CONN
 * \brief           Connection start structure, used to start the connection in extended mode
 */
typedef struct {
    lwesp_conn_type_t type;                     /*!< Connection type */
    const char* remote_host;                    /*!< Host name or IP address in string format */
    lwesp_port_t remote_port;                   /*!< Remote server port */
    const char* local_ip;                       /*!< Local IP. Optional parameter, set to NULL if not used (most cases) */
    union {
        struct {
            uint16_t keep_alive;                /*!< Keep alive parameter for TCP/SSL connection in units of seconds.
                                                    Value can be between `0 - 7200` where `0` means no keep alive */
        } tcp_ssl;                              /*!< TCP/SSL specific features */
        struct {
            lwesp_port_t local_port;            /*!< Custom local port for UDP */
            uint8_t mode;                       /*!< UDP mode. Set to `0` by default. Check ESP AT commands instruction set for more info when needed */
        } udp;                                  /*!< UDP specific features */
    } ext;                                      /*!< Extended support union */
} lwesp_conn_start_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_DEFS_H */
