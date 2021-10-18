/**
 * \file            esp_int.c
 * \brief           Internal functions
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
#include "esp/esp_private.h"
#include "esp/esp.h"
#include "esp/esp_int.h"
#include "esp/esp_mem.h"
#include "esp/esp_parser.h"
#include "esp/esp_unicode.h"
#include "system/esp_ll.h"

#if !__DOXYGEN__
/**
 * \brief           Receive character structure to handle full line terminated with `\n` character
 */
typedef struct {
    char data[128];                             /*!< Received characters */
    size_t len;                                 /*!< Length of valid characters */
} esp_recv_t;

/* Receive character macros */
#define RECV_ADD(ch)                        do { if (recv_buff.len < (sizeof(recv_buff.data)) - 1) { recv_buff.data[recv_buff.len++] = ch; recv_buff.data[recv_buff.len] = 0; } } while (0)
#define RECV_RESET()                        do { recv_buff.len = 0; recv_buff.data[0] = 0; } while (0)
#define RECV_LEN()                          ((size_t)recv_buff.len)
#define RECV_IDX(index)                     recv_buff.data[index]

/* Send data over AT port */
#define AT_PORT_SEND_STR(str)               esp.ll.send_fn((const void *)(str), (size_t)strlen(str))
#define AT_PORT_SEND_CONST_STR(str)         esp.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
#define AT_PORT_SEND_CHR(str)               esp.ll.send_fn((const void *)(str), (size_t)1)
#define AT_PORT_SEND_FLUSH()                esp.ll.send_fn(NULL, 0)
#define AT_PORT_SEND(d, l)                  esp.ll.send_fn((const void *)(d), (size_t)(l))
#define AT_PORT_SEND_WITH_FLUSH(d, l)       do { AT_PORT_SEND((d), (l)); AT_PORT_SEND_FLUSH(); } while (0)

/* Beginning and end of every AT command */
#define AT_PORT_SEND_BEGIN_AT()             do { AT_PORT_SEND_CONST_STR("AT"); } while (0)
#define AT_PORT_SEND_END_AT()               do { AT_PORT_SEND(CRLF, CRLF_LEN); AT_PORT_SEND_FLUSH(); } while (0)

/* Send special characters over AT port with condition */
#define AT_PORT_SEND_QUOTE_COND(q)          do { if ((q)) { AT_PORT_SEND_CONST_STR("\""); } } while (0)
#define AT_PORT_SEND_COMMA_COND(c)          do { if ((c)) { AT_PORT_SEND_CONST_STR(","); } } while (0)
#define AT_PORT_SEND_EQUAL_COND(e)          do { if ((e)) { AT_PORT_SEND_CONST_STR("="); } } while (0)
#endif /* !__DOXYGEN__ */

static esp_recv_t recv_buff;
static espr_t espi_process_sub_cmd(esp_msg_t* msg, uint8_t* is_ok, uint8_t* is_error, uint8_t* is_ready);

/**
 * \brief           Free connection send data memory
 * \param[in]       m: Send data message type
 */
#define CONN_SEND_DATA_FREE(m)      do {            \
    if ((m) != NULL && (m)->msg.conn_send.fau) {    \
        (m)->msg.conn_send.fau = 0;                 \
        if ((m)->msg.conn_send.data != NULL) {      \
            ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,   \
                "[CONN] Free write buffer fau: %p\r\n", (void *)(m)->msg.conn_send.data);   \
            esp_mem_free_s((void **)&((m)->msg.conn_send.data));\
        }                                           \
    }                                               \
} while (0)

/**
 * \brief           Send connection callback for "data send"
 * \param[in]       m: Connection send message
 * \param[in]       err: Error of type \ref espr_t
 */
#define CONN_SEND_DATA_SEND_EVT(m, err)  do {       \
    CONN_SEND_DATA_FREE(m);                         \
    esp.evt.type = ESP_EVT_CONN_SEND;               \
    esp.evt.evt.conn_data_send.res = err;           \
    esp.evt.evt.conn_data_send.conn = (m)->msg.conn_send.conn;  \
    esp.evt.evt.conn_data_send.sent = (m)->msg.conn_send.sent_all;   \
    espi_send_conn_cb((m)->msg.conn_send.conn, NULL);   \
} while (0)

/**
 * \brief           Send reset sequence event
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define RESET_SEND_EVT(m, err)  do {                \
    esp.evt.evt.reset.res = err;                    \
    espi_send_cb(ESP_EVT_RESET);                    \
} while (0)

/**
 * \brief           Send restore sequence event
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define RESTORE_SEND_EVT(m, err)  do {              \
    esp.evt.evt.restore.res = err;                  \
    espi_send_cb(ESP_EVT_RESTORE);                  \
} while (0)
 
 /**
 * \brief           Send ping event to user
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define PING_SEND_EVT(m, err)   do {                \
    esp.evt.evt.ping.res = err;  \
    esp.evt.evt.ping.host = (m)->msg.tcpip_ping.host;   \
    esp.evt.evt.ping.time = (m)->msg.tcpip_ping.time;   \
    espi_send_cb(ESP_EVT_PING);                     \
} while (0)

/**
 * \brief           Send cipdomain (DNS function) event to user
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define CIPDOMAIN_SEND_EVT(m, err)   do {           \
    esp.evt.evt.dns_hostbyname.res = err;           \
    esp.evt.evt.dns_hostbyname.host = msg->msg.dns_getbyhostname.host;  \
    esp.evt.evt.dns_hostbyname.ip = msg->msg.dns_getbyhostname.ip;  \
    espi_send_cb(ESP_EVT_DNS_HOSTBYNAME);           \
} while (0)

/**
 * \brief           Send join AP event to user
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define STA_JOIN_AP_SEND_EVT(m, err) do {           \
    esp.evt.evt.sta_join_ap.res = err;              \
    espi_send_cb(ESP_EVT_STA_JOIN_AP);              \
} while (0)

/**
 * \brief           Send list AP event to user
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define STA_LIST_AP_SEND_EVT(m, err) do {           \
    esp.evt.evt.sta_list_ap.res = err;              \
    esp.evt.evt.sta_list_ap.aps = msg->msg.ap_list.aps; \
    esp.evt.evt.sta_list_ap.len = msg->msg.ap_list.apsi;\
    espi_send_cb(ESP_EVT_STA_LIST_AP);              \
} while (0)

/**
 * \brief           Send info AP event to user
 * \param[in]       m: Command message
 * \param[in]       err: Error of type \ref espr_t
 */
#define STA_INFO_AP_SEND_EVT(m, err) do {           \
    esp.evt.evt.sta_info_ap.res = err;              \
    esp.evt.evt.sta_info_ap.info = esp.msg->msg.sta_info_ap.info;   \
    espi_send_cb(ESP_EVT_STA_INFO_AP);              \
} while (0)

/**
 * \brief           Send IP or MAC address to AT port
 * \param[in]       d: Pointer to IP or MAC address
 * \param[in]       is_ip: Set to `1` when sending IP, `0` when MAC
 * \param[in]       q: Set to `1` to include start and ending quotes
 * \param[in]       c: Set to `1` to include comma before string
 */
void
espi_send_ip_mac(const void* d, uint8_t is_ip, uint8_t q, uint8_t c) {
    uint8_t ch;
    char str[4];
    const esp_mac_t* mac = d;
    const esp_ip_t* ip = d;

    AT_PORT_SEND_COMMA_COND(c);                 /* Send comma */
    if (d == NULL) {
        return;
    }
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
    ch = is_ip ? '.' : ':';                     /* Get delimiter character */
    for (uint8_t i = 0; i < (is_ip ? 4 : 6); i++) { /* Process byte by byte */
        if (is_ip) {                            /* In case of IP ... */
            esp_u8_to_str(ip->ip[i], str);      /* ... go to decimal format ... */
        } else {                                /* ... in case of MAC ... */
            esp_u8_to_hex_str(mac->mac[i], str, 2); /* ... go to HEX format */
        }
        AT_PORT_SEND_STR(str);                  /* Send str */
        if (i < (is_ip ? 4 : 6) - 1) {          /* Check end if characters */
            AT_PORT_SEND_CHR(&ch);              /* Send character */
        }
    }
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
}

/**
 * \brief           Send string to AT port, either plain or escaped
 * \param[in]       str: Pointer to input string to string
 * \param[in]       e: Value to indicate string send format, escaped (`1`) or plain (`0`)
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void
espi_send_string(const char* str, uint8_t e, uint8_t q, uint8_t c) {
    char special = '\\';

    AT_PORT_SEND_COMMA_COND(c);                 /* Send comma */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
    if (str != NULL) {
        if (e) {                                /* Do we have to escape string? */
            while (*str) {                      /* Go through string */
                if (*str == ',' || *str == '"' || *str == '\\') {   /* Check for special character */
                    AT_PORT_SEND_CHR(&special); /* Send special character */
                }
                AT_PORT_SEND_CHR(str);          /* Send character */
                str++;
            }
        } else {
            AT_PORT_SEND_STR(str);              /* Send plain string */
        }
    }
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
}

/**
 * \brief           Send number (decimal) to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void
espi_send_number(uint32_t num, uint8_t q, uint8_t c) {
    char str[11];

    esp_u32_to_str(num, str);                   /* Convert digit to decimal string */

    AT_PORT_SEND_COMMA_COND(c);                 /* Send comma */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
    AT_PORT_SEND_STR(str);                      /* Send string with number */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
}

/**
 * \brief           Send port number to AT port
 * \param[in]       port: Port number to send
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void
espi_send_port(esp_port_t port, uint8_t q, uint8_t c) {
    char str[6];

    esp_u16_to_str(ESP_PORT2NUM(port), str);    /* Convert digit to decimal string */

    AT_PORT_SEND_COMMA_COND(c);                 /* Send comma */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
    AT_PORT_SEND_STR(str);                      /* Send string with number */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
}

/**
 * \brief           Send signed number to AT port
 * \param[in]       num: Number to send to AT port
 * \param[in]       q: Value to indicate starting and ending quotes, enabled (`1`) or disabled (`0`)
 * \param[in]       c: Set to `1` to include comma before string
 */
void
espi_send_signed_number(int32_t num, uint8_t q, uint8_t c) {
    char str[11];

    esp_i32_to_str(num, str);                   /* Convert digit to decimal string */

    AT_PORT_SEND_COMMA_COND(c);                 /* Send comma */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
    AT_PORT_SEND_STR(str);                      /* Send string with number */
    AT_PORT_SEND_QUOTE_COND(q);                 /* Send quote */
}

/**
 * \brief           Reset all connections
 * \note            Used to notify upper layer stack to close everything and reset the memory if necessary
 * \param[in]       forced: Flag indicating reset was forced by command
 */
static void
reset_connections(uint8_t forced) {
    esp.evt.type = ESP_EVT_CONN_CLOSE;
    esp.evt.evt.conn_active_close.forced = forced;
    esp.evt.evt.conn_active_close.res = espOK;

    for (size_t i = 0; i < ESP_CFG_MAX_CONNS; i++) {/* Check all connections */
        if (esp.m.conns[i].status.f.active) {
            esp.m.conns[i].status.f.active = 0;

            esp.evt.evt.conn_active_close.conn = &esp.m.conns[i];
            esp.evt.evt.conn_active_close.client = esp.m.conns[i].status.f.client;
            espi_send_conn_cb(&esp.m.conns[i], NULL);   /* Send callback function */
        }
    }
}

/**
 * \brief           Reset everything after reset was detected
 * \param[in]       forced: Set to `1` if reset forced by user
 */
void
espi_reset_everything(uint8_t forced) {
    /**
     * \todo: Put stack to default state:
     *          - Close all the connections in memory
     *          - Clear entire data memory
     *          - Reset esp structure with IP
     *          - Start over init procedure
     */

    /* Step 1: Close all connections in memory */
    reset_connections(forced);

#if ESP_CFG_MODE_STATION
    esp.m.sta.has_ip = 0;
    if (esp.m.sta.is_connected) {
        espi_send_cb(ESP_EVT_WIFI_DISCONNECTED);
    }
    esp.m.sta.is_connected = 0;
#endif /* ESP_CFG_MODE_STATION */

    /* Check if IPD active */
    if (esp.m.ipd.buff != NULL) {
        esp_pbuf_free(esp.m.ipd.buff);
        esp.m.ipd.buff = NULL;
    }

    /* Invalid ESP modules */
    ESP_MEMSET(&esp.m, 0x00, sizeof(esp.m));

    /* Set default device */
#if ESP_CFG_ESP8266 && !ESP_CFG_ESP32
    esp.m.device = ESP_DEVICE_ESP8266;
#elif !ESP_CFG_ESP8266 && ESP_CFG_ESP32
    esp.m.device = ESP_DEVICE_ESP32;
#else
    esp.m.device = ESP_DEVICE_UNKNOWN;
#endif  /* ESP_CFG_ESP8266 && !ESP_CFG_ESP32 */

    /* Reset baudrate to default */
    esp.ll.uart.baudrate = ESP_CFG_AT_PORT_BAUDRATE;
    esp_ll_init(&esp.ll);

    /* If reset was not forced by user, repeat with manual reset */
    if (!forced) {
        esp_reset(NULL, NULL, 0);
    }
}

/**
 * \brief           Process callback function to user with specific type
 * \param[in]       type: Callback event type
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_send_cb(esp_evt_type_t type) {
    esp.evt.type = type;                        /* Set callback type to process */

    /* Call callback function for all registered functions */
    for (esp_evt_func_t* link = esp.evt_func; link != NULL; link = link->next) {
        link->fn(&esp.evt);
    }
    return espOK;
}

/**
 * \brief           Process connection callback
 * \note            Before calling function, callback structure must be prepared
 * \param[in]       conn: Pointer to connection to use as callback
 * \param[in]       evt: Event callback function for connection
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_send_conn_cb(esp_conn_t* conn, esp_evt_fn evt) {
    if (conn->status.f.in_closing && esp.evt.type != ESP_EVT_CONN_CLOSE) {  /* Do not continue if in closing mode */
        /* return espOK; */
    }

    if (evt != NULL) {                          /* Try with user connection */
        return evt(&esp.evt);                   /* Call temporary function */
    } else if (conn != NULL && conn->evt_func != NULL) {/* Connection custom callback? */
        return conn->evt_func(&esp.evt);        /* Process callback function */
    } else if (conn == NULL) {
        return espOK;
    }

    /*
     * On normal API operation,
     * we should never enter to this part of code
     */

    /*
     * If connection doesn't have callback function
     * automatically close the connection?
     *
     * Since function call is non-blocking,
     * it will set active connection to closing mode
     * and further callback events should not be executed anymore
     */
    return esp_conn_close(conn, 0);
}

/**
 * \brief           Process and send data from device buffer
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
espi_tcpip_process_send_data(void) {
    esp_conn_t* c = esp.msg->msg.conn_send.conn;
    if (!esp_conn_is_active(c) ||               /* Is the connection already closed? */
        esp.msg->msg.conn_send.val_id != c->val_id  /* Did validation ID change after we set parameter? */
    ) {
        /* Send event to user about failed send event */
        CONN_SEND_DATA_SEND_EVT(esp.msg, espCLOSED);
        return espERR;
    }
    esp.msg->msg.conn_send.sent = ESP_MIN(esp.msg->msg.conn_send.btw, ESP_CFG_CONN_MAX_DATA_LEN);

    AT_PORT_SEND_BEGIN_AT();
    AT_PORT_SEND_CONST_STR("+CIPSEND=");
    espi_send_number(ESP_U32(c->num), 0, 0);    /* Send connection number */
    espi_send_number(ESP_U32(esp.msg->msg.conn_send.sent), 0, 1);   /* Send length number */

    /* On UDP connections, IP address and port may be included */
    if (c->type == ESP_CONN_TYPE_UDP) {
        if (esp.msg->msg.conn_send.remote_ip != NULL && esp.msg->msg.conn_send.remote_port) {
            espi_send_ip_mac(esp.msg->msg.conn_send.remote_ip, 1, 1, 1);/* Send IP address including quotes */
            espi_send_port(esp.msg->msg.conn_send.remote_port, 0, 1);   /* Send length number */
        }
    }
    AT_PORT_SEND_END_AT();
    return espOK;
}

/**
 * \brief           Process data sent and send remaining
 * \param[in]       sent: Status whether data were sent or not,
 *                      info received from ESP with `SEND OK` or `SEND FAIL`
 * \return          `1` in case we should stop sending or `0` if we still have data to process
 */
static uint8_t
espi_tcpip_process_data_sent(uint8_t sent) {
    if (sent) {                                 /* Data were successfully sent */
        esp.msg->msg.conn_send.sent_all += esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.btw -= esp.msg->msg.conn_send.sent;
        esp.msg->msg.conn_send.ptr += esp.msg->msg.conn_send.sent;
        if (esp.msg->msg.conn_send.bw != NULL) {
            *esp.msg->msg.conn_send.bw += esp.msg->msg.conn_send.sent;
        }
        esp.msg->msg.conn_send.tries = 0;
    } else {                                    /* We were not successful */
        esp.msg->msg.conn_send.tries++;         /* Increase number of tries */
        if (esp.msg->msg.conn_send.tries == ESP_CFG_MAX_SEND_RETRIES) { /* In case we reached max number of retransmissions */
            return 1;                           /* Return 1 and indicate error */
        }
    }
    if (esp.msg->msg.conn_send.btw > 0) {       /* Do we still have data to send? */
        if (espi_tcpip_process_send_data() != espOK) {  /* Check if we can continue */
            return 1;                           /* Finish at this point */
        }
        return 0;                               /* We still have data to send */
    }
    return 1;                                   /* Everything was sent, we can stop execution */
}

/**
 * \brief           Send error event to application layer
 * \param[in]       msg: Message from user with connection start
 * \param[in]       error: Value indicating cause of error
 */
static void
espi_send_conn_error_cb(esp_msg_t* msg, espr_t error) {
    esp.evt.type = ESP_EVT_CONN_ERROR;          /* Connection error */
    esp.evt.evt.conn_error.host = esp.msg->msg.conn_start.remote_host;
    esp.evt.evt.conn_error.port = esp.msg->msg.conn_start.remote_port;
    esp.evt.evt.conn_error.type = esp.msg->msg.conn_start.type;
    esp.evt.evt.conn_error.arg = esp.msg->msg.conn_start.arg;
    esp.evt.evt.conn_error.err = error;

    /* Call callback specified by user on connection startup */
    esp.msg->msg.conn_start.evt_func(&esp.evt);
    ESP_UNUSED(msg);
}

/**
 * \brief           Process received string from ESP
 * \param[in]       recv: Pointer to \ref esp_rect_t structure with input string
 */
static void
espi_parse_received(esp_recv_t* rcv) {
    uint8_t is_ok = 0, is_error = 0, is_ready = 0;
    const char* s;

    /* Try to remove non-parsable strings */
    if ((rcv->len == 2 && rcv->data[0] == '\r' && rcv->data[1] == '\n')
        /*
         * Condition below can only be used if AT echo is disabled
         * otherwise it may happen that message is inserted in between AT command echo, such as:
         *
         * AT+CIPCLOSE=0+LINK_CONN:0,2,"TCP",1,"192.168.0.14",57551,80\r\n\r\n
         *
         * Instead of:
         * AT+CIPCLOSE=0\r\n
         * +LINK_CONN:0,2,"TCP",1,"192.168.0.14",57551,80\r\n
         */
        /* || (rcv->len > 3 && rcv->data[0] == 'A' && rcv->data[1] == 'T' && rcv->data[2] == '+') */) {
        return;
    }

    /* Detect most common responses from device */
    is_ok = !strcmp(rcv->data, "OK" CRLF);      /* Check if received string is OK */
    if (!is_ok) {
        is_error = !strcmp(rcv->data, "ERROR" CRLF) || !strcmp(rcv->data, "FAIL" CRLF); /* Check if received string is error */
        if (!is_error) {
            is_ready = !strcmp(rcv->data, "ready" CRLF);    /* Check if received string is ready */
        }
    }

    /*
     * In case ready was received, there was a reset on device,
     * either forced by command or problem on device itself
     */
    if (is_ready) {
        if (CMD_IS_CUR(ESP_CMD_RESET) || CMD_IS_CUR(ESP_CMD_RESTORE)) {   /* Did we force reset? */
            esp.evt.evt.reset_detected.forced = 1;
        } else {                                /* Reset due unknown error */
            esp.evt.evt.reset_detected.forced = 0;
            if (esp.msg != NULL) {
                is_ok = 0;
                is_error = 1;
                is_ready = 0;
            }
        }
        espi_reset_everything(esp.evt.evt.reset_detected.forced);   /* Put everything to default state */
        espi_send_cb(ESP_EVT_RESET_DETECTED);   /* Call user callback function */
    }

    /* Read and process statements starting with '+' character */
    if (rcv->data[0] == '+') {
        if (!strncmp("+IPD", rcv->data, 4)) {   /* Check received network data */
            espi_parse_ipd(rcv->data);          /* Parse IPD statement and start receiving network data */
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
            /* IPD message notification? */
            espi_conn_manual_tcp_try_read_data(esp.m.ipd.conn);
        } else if (!strncmp("+CIPRECVDATA", rcv->data, 12)) {
            espi_parse_ciprecvdata(rcv->data);  /* Parse CIPRECVDATA statement and start receiving network data */
        } else if (!strncmp("+CIPRECVLEN", rcv->data, 11)) {
            espi_parse_ciprecvlen(rcv->data);   /* Parse CIPRECVLEN statement */
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
#if ESP_CFG_MODE_ACCESS_POINT
        } else if (!strncmp(rcv->data, "+STA_CONNECTED", 14)) {
            espi_parse_ap_conn_disconn_sta(&rcv->data[15], 1);  /* Parse string and send to user layer */
        } else if (!strncmp(rcv->data, "+STA_DISCONNECTED", 17)) {
            espi_parse_ap_conn_disconn_sta(&rcv->data[18], 0);  /* Parse string and send to user layer */
        } else if (!strncmp(rcv->data, "+DIST_STA_IP", 12)) {
            espi_parse_ap_ip_sta(&rcv->data[13]);   /* Parse string and send to user layer */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        } else if (esp.msg != NULL) {
            if (0
#if ESP_CFG_MODE_STATION
                || (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTAMAC_GET) && !strncmp(rcv->data, "+CIPSTAMAC", 10))
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
                || (CMD_IS_CUR(ESP_CMD_WIFI_CIPAPMAC_GET) && !strncmp(rcv->data, "+CIPAPMAC", 9))
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ) {
                const char* tmp;
                esp_mac_t mac;

                if (rcv->data[9] == ':') {
                    tmp = &rcv->data[10];
                } else if (rcv->data[10] == ':') {
                    tmp = &rcv->data[11];
                }

                espi_parse_mac(&tmp, &mac);     /* Save as current MAC address */
#if ESP_CFG_MODE_STATION
                if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTAMAC_GET)) {
                    ESP_MEMCPY(&esp.m.sta.mac, &mac, 6);/* Copy to current setup */
                }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
                if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAPMAC_GET)) {
                    ESP_MEMCPY(&esp.m.ap.mac, &mac, 6); /* Copy to current setup */
                }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
                if (esp.msg->msg.sta_ap_getmac.mac != NULL && CMD_IS_CUR(CMD_GET_DEF())) {
                    ESP_MEMCPY(esp.msg->msg.sta_ap_getmac.mac, &mac, sizeof(mac));  /* Copy to current setup */
                }
            } else if (0
#if ESP_CFG_MODE_STATION
                || (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET) && !strncmp(rcv->data, "+CIPSTA", 7))
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
                || (CMD_IS_CUR(ESP_CMD_WIFI_CIPAP_GET) && !strncmp(rcv->data, "+CIPAP", 6))
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            ) {
                const char* tmp = NULL;
                esp_ip_t ip, *a = NULL, *b = NULL;
                esp_ip_mac_t* im;
                uint8_t ch = 0;

#if ESP_CFG_MODE_STATION
                if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET)) {
                    im = &esp.m.sta;            /* Get IP and MAC structure first */
                }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
                if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAP_GET)) {
                    im = &esp.m.ap;             /* Get IP and MAC structure first */
                }
#endif /* ESP_CFG_MODE_ACCESS_POINT */

                if (im != NULL) {
                    /* We expect "+CIPSTA:" or "+CIPAP:" ... */
                    if (rcv->data[6] == ':') {
                        ch = rcv->data[7];
                    } else if (rcv->data[7] == ':') {
                        ch = rcv->data[8];
                    }
                    switch (ch) {
                        case 'i': tmp = &rcv->data[10]; a = &im->ip; b = esp.msg->msg.sta_ap_getip.ip; break;
                        case 'g': tmp = &rcv->data[15]; a = &im->gw; b = esp.msg->msg.sta_ap_getip.gw; break;
                        case 'n': tmp = &rcv->data[15]; a = &im->nm; b = esp.msg->msg.sta_ap_getip.nm; break;
                        default: tmp = NULL; a = NULL; b = NULL; break;
                    }
                    if (tmp != NULL) {          /* Do we have temporary string? */
                        if (rcv->data[6] == '_' || rcv->data[7] == '_') {   /* Do we have "_CUR" or "_DEF" included? */
                            tmp += 4;           /* Skip it */
                        }
                        if (*tmp == ':') {
                            tmp++;
                        }
                        espi_parse_ip(&tmp, &ip);   /* Parse IP address */
                        ESP_MEMCPY(a, &ip, sizeof(ip)); /* Copy to current setup */
                        if (b != NULL && CMD_IS_CUR(CMD_GET_DEF())) {   /* Is current command the same as default one? */
                            ESP_MEMCPY(b, &ip, sizeof(ip)); /* Copy to user variable */
                        }
                    }
                }
#if ESP_CFG_MODE_STATION
            } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWLAP) && !strncmp(rcv->data, "+CWLAP", 6)) {
                espi_parse_cwlap(rcv->data, esp.msg);   /* Parse CWLAP entry */
            } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWJAP) && !strncmp(rcv->data, "+CWJAP", 6)) {
                const char* tmp = &rcv->data[7];/* Go to the number position */
                esp.msg->msg.sta_join.error_num = (uint8_t)espi_parse_number(&tmp);
            } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWJAP_GET) && !strncmp(rcv->data, "+CWJAP", 6)) {
                espi_parse_cwjap(rcv->data, esp.msg);/* Parse CWJAP */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_DNS
            } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPDOMAIN) && !strncmp(rcv->data, "+CIPDOMAIN", 10)) {
                espi_parse_cipdomain(rcv->data, esp.msg);   /* Parse CIPDOMAIN entry */
#endif /* ESP_CFG_DNS */ 
#if ESP_CFG_PING
            } else if (CMD_IS_CUR(ESP_CMD_TCPIP_PING) && !strncmp(rcv->data, "+PING", 5)) {
                espi_parse_ping_time(rcv->data, esp.msg);   /* Parse ping time */
#endif /* ESP_CFG_PING */
#if ESP_CFG_SNTP
            } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSNTPTIME) && !strncmp(rcv->data, "+CIPSNTPTIME", 12)) {
                espi_parse_cipsntptime(rcv->data, esp.msg); /* Parse CIPSNTPTIME entry */
#endif /* ESP_CFG_SNTP */
#if ESP_CFG_HOSTNAME
            } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWHOSTNAME_GET) && !strncmp(rcv->data, "+CWHOSTNAME", 11)) {
                espi_parse_hostname(rcv->data, esp.msg);/* Parse HOSTNAME entry */
#endif /* ESP_CFG_HOSTNAME */
            } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_GET) && !strncmp(rcv->data, "+CWDHCP", 7)) {
                espi_parse_cwdhcp(rcv->data);   /* Parse CWDHCP state */
            }
        }
#if ESP_CFG_MODE_STATION
    } else if (strlen(rcv->data) > 4 && !strncmp(rcv->data, "WIFI", 4)) {
        if (!strncmp(&rcv->data[5], "CONNECTED", 9)) {
            esp.m.sta.is_connected = 1;         /* Wifi is connected */
            espi_send_cb(ESP_EVT_WIFI_CONNECTED);   /* Call user callback function */
            if (!CMD_IS_CUR(ESP_CMD_WIFI_CWJAP)) {  /* In case of auto connection */
                esp_sta_getip(NULL, NULL, NULL, NULL, NULL, 0);  /* Get new IP address */
            }
        } else if (!strncmp(&rcv->data[5], "DISCONNECT", 10)) {
            esp.m.sta.is_connected = 0;         /* Wifi is disconnected */
            esp.m.sta.has_ip = 0;               /* There is no valid IP */
            espi_send_cb(ESP_EVT_WIFI_DISCONNECTED);/* Call user callback function */
        } else if (!strncmp(&rcv->data[5], "GOT IP", 6)) {
            esp.m.sta.has_ip = 1;               /* Wifi got IP address */
            espi_send_cb(ESP_EVT_WIFI_GOT_IP);  /* Call user callback function */
            if (!CMD_IS_CUR(ESP_CMD_WIFI_CWJAP)) { /* In case of auto connection */
                esp_sta_getip(NULL, NULL, NULL, NULL, NULL, 0); /* Get new IP address */
            }
        }
    } else if (CMD_IS_CUR(ESP_CMD_GMR)) {
        if (!strncmp(rcv->data, "AT version", 10)) {
            uint8_t ok = 0, major = 0, minor = 0, patch = 0;
            espi_parse_at_sdk_version(&rcv->data[11], &esp.m.version_at);

#if ESP_CFG_ESP8266
            if (esp.m.device == ESP_DEVICE_ESP8266) {
                major = ESP_MIN_AT_VERSION_MAJOR_ESP8266;
                minor = ESP_MIN_AT_VERSION_MINOR_ESP8266;
                patch = ESP_MIN_AT_VERSION_PATCH_ESP8266;
            }
#endif /* ESP_CFG_ESP8266 */
#if ESP_CFG_ESP32
            if (esp.m.device == ESP_DEVICE_ESP32) {
                major = ESP_MIN_AT_VERSION_MAJOR_ESP32;
                minor = ESP_MIN_AT_VERSION_MINOR_ESP32;
                patch = ESP_MIN_AT_VERSION_PATCH_ESP32;
            }
#endif /* ESP_CFG_ESP32 */

            /* TODO: Compare ESP8266 vs ESP32 separatelly */
            /* Compare versions */
            if (esp.m.version_at.major > major) {
                ok = 1;
            } else if (esp.m.version_at.major == major) {
                if (esp.m.version_at.minor > minor) {
                    ok = 1;
                } else if (esp.m.version_at.minor == minor) {
                    if ((int8_t)esp.m.version_at.patch >= (int8_t)patch) {
                        ok = 1;
                    }
                }
            }
            if (!ok) {
                espi_send_cb(ESP_EVT_AT_VERSION_NOT_SUPPORTED);
            }
        } else if (!strncmp(rcv->data, "SDK version", 11)) {
            espi_parse_at_sdk_version(&rcv->data[12], &esp.m.version_sdk);
        }
#endif /* ESP_CFG_MODE_STATION */
    }

    /* Start processing received data */
    if (esp.msg != NULL) {                      /* Do we have valid message? */
        if ((CMD_IS_CUR(ESP_CMD_RESET) || CMD_IS_CUR(ESP_CMD_RESTORE)) && is_ok) {  /* Check for reset/restore command */
            is_ok = 0;                          /* We must wait for "ready", not only "OK" */
            esp.ll.uart.baudrate = ESP_CFG_AT_PORT_BAUDRATE;/* Save user baudrate */
            esp_ll_init(&esp.ll);               /* Set new baudrate */
        } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTATUS)) {
            if (!strncmp(rcv->data, "+CIPSTATUS", 10)) {
                espi_parse_cipstatus(rcv->data + 11);   /* Parse CIPSTATUS response */
            } else if (is_ok) {
                for (size_t i = 0; i < ESP_CFG_MAX_CONNS; i++) {    /* Set current connection statuses */
                    esp.m.conns[i].status.f.active = !!(esp.m.active_conns & (1 << i));
                }
            }
        } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTART)) {
            /*
             * \todo: Request Espressif for numerical response instead of random messages
             */
            /*
            if (!strncmp(rcv->data, "DNS Fail", 8)) {

            } else if (!strncmp(rcv->data, "ID ERROR", 8)) {

            } else if (!strncmp(rcv->data, "Link type ERROR", 15)) {

            }
            */
        } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSEND)) {
            if (is_ok) {                        /* Check for OK and clear as we have to check for "> " statement after OK */
                is_ok = 0;                      /* Do not reach on OK */
            }
            if (esp.msg->msg.conn_send.wait_send_ok_err) {
                if (!strncmp("SEND OK", rcv->data, 7)) {    /* Data were sent successfully */
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_ok = espi_tcpip_process_data_sent(1);    /* Process as data were sent */
                    if (is_ok && esp.msg->msg.conn_send.conn->status.f.active) {
                        CONN_SEND_DATA_SEND_EVT(esp.msg, espOK);
                    }
                } else if (is_error || !strncmp("SEND FAIL", rcv->data, 9)) {
                    esp.msg->msg.conn_send.wait_send_ok_err = 0;
                    is_error = espi_tcpip_process_data_sent(0); /* Data were not sent due to SEND FAIL or command didn't even start */
                    if (is_error && esp.msg->msg.conn_send.conn->status.f.active) {
                        CONN_SEND_DATA_SEND_EVT(esp.msg, espERR);
                    }
                }
            } else if (is_error) {
                CONN_SEND_DATA_SEND_EVT(esp.msg, espERR);
            }
        } else if (CMD_IS_CUR(ESP_CMD_UART)) {  /* In case of UART command */
            if (is_ok) {                        /* We have valid OK result */
                esp.ll.uart.baudrate = esp.msg->msg.uart.baudrate;  /* Save user baudrate */
                esp_ll_init(&esp.ll);           /* Set new baudrate */
            }
#if ESP_CFG_MODE_ACCESS_POINT
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWLIF) && ESP_CHARISNUM(rcv->data[0])) {
            espi_parse_cwlif(rcv->data, esp.msg);   /* Parse CWLIF entry */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        }
    }

    /*
     * Check if connection is just active (or closed):
     *
     * Since new ESP AT release, it is possible to get
     * connection status by using +LINK_CONN message.
     *
     * Check LINK_CONN messages
     */
    if (rcv->len > 20 && (s = strstr(rcv->data, "+LINK_CONN:")) != NULL) {
        if (espi_parse_link_conn(s) && esp.m.link_conn.num < ESP_CFG_MAX_CONNS) {
            uint8_t id;
            esp_conn_t* conn = &esp.m.conns[esp.m.link_conn.num];   /* Get connection pointer */
            if (esp.m.link_conn.failed && conn->status.f.active) {  /* Connection failed and now closed? */
                conn->status.f.active = 0;      /* Connection was just closed */

                esp.evt.type = ESP_EVT_CONN_CLOSE;
                esp.evt.evt.conn_active_close.conn = conn;
                esp.evt.evt.conn_active_close.client = conn->status.f.client;   /* Set if it is client or not */
                /** @todo: Check if we really tried to close connection which was just closed */
                esp.evt.evt.conn_active_close.forced = CMD_IS_CUR(ESP_CMD_TCPIP_CIPCLOSE);  /* Set if action was forced = current action = close connection */
                esp.evt.evt.conn_active_close.client = espOK;
                espi_send_conn_cb(conn, NULL);  /* Send event */

                /* Check if write buffer is set */
                if (conn->buff.buff != NULL) {
                    ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
                        "[CONN] Free write buffer: %p\r\n", conn->buff.buff);
                    esp_mem_free_s((void **)&conn->buff.buff);
                }
            } else if (!esp.m.link_conn.failed && !conn->status.f.active) {
                id = conn->val_id;
                ESP_MEMSET(conn, 0x00, sizeof(*conn));  /* Reset connection parameters */
                conn->num = esp.m.link_conn.num;/* Set connection number */
                conn->status.f.active = !esp.m.link_conn.failed;    /* Check if connection active */
                conn->val_id = ++id;            /* Set new validation ID */

                conn->type = esp.m.link_conn.type;/* Set connection type */
                ESP_MEMCPY(&conn->remote_ip, &esp.m.link_conn.remote_ip, sizeof(conn->remote_ip));
                conn->remote_port = esp.m.link_conn.remote_port;
                conn->local_port = esp.m.link_conn.local_port;
                conn->status.f.client = !esp.m.link_conn.is_server;

                if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTART)
                    && esp.m.link_conn.num == esp.msg->msg.conn_start.num
                    && conn->status.f.client) { /* Did we start connection on our own and connection is client? */
                    conn->status.f.client = 1;  /* Go to client mode */
                    conn->evt_func = esp.msg->msg.conn_start.evt_func;  /* Set callback function */
                    conn->arg = esp.msg->msg.conn_start.arg;    /* Set argument for function */
                    esp.msg->msg.conn_start.success = 1;
                } else {                        /* Server connection start */
                    conn->evt_func = esp.evt_server;/* Set server default callback */
                    conn->arg = NULL;
                    conn->type = ESP_CONN_TYPE_TCP; /* Set connection type to TCP. @todo: Wait for ESP team to upgrade AT commands to set other type */
                }

                esp.evt.type = ESP_EVT_CONN_ACTIVE; /* Connection just active */
                esp.evt.evt.conn_active_close.conn = conn; /* Set connection */
                esp.evt.evt.conn_active_close.client = conn->status.f.client;   /* Set if it is client or not */
                esp.evt.evt.conn_active_close.forced = conn->status.f.client;   /* Set if action was forced = if client mode */
                espi_send_conn_cb(conn, NULL);  /* Send event */
                espi_conn_start_timeout(conn);  /* Start connection timeout timer */
            }
        }
    /*
    } else if (!strncmp(",CLOSED", &rcv->data[1], 7)) {
        const char* tmp = rcv->data; */
    } else if ( (rcv->len > 9  && (s = strstr(rcv->data, ",CLOSED" CRLF)) != NULL) ||
                (rcv->len > 15 && (s = strstr(rcv->data, ",CONNECT FAIL" CRLF)) != NULL)) {
        const char* tmp = s;
        uint32_t num = 0;
        while (tmp > rcv->data && ESP_CHARISNUM(tmp[-1])) {
            tmp--;
        }
        num = espi_parse_number(&tmp);          /* Parse connection number */
        if (num < ESP_CFG_MAX_CONNS) {
            esp_conn_t* conn = &esp.m.conns[num];   /* Parse received data */
            conn->num = num;                    /* Set connection number */
            if (conn->status.f.active) {        /* Is connection actually active? */
                conn->status.f.active = 0;      /* Connection was just closed */

                esp.evt.type = ESP_EVT_CONN_CLOSE;
                esp.evt.evt.conn_active_close.conn = conn;
                esp.evt.evt.conn_active_close.client = conn->status.f.client;   /* Set if it is client or not */
                esp.evt.evt.conn_active_close.forced = CMD_IS_CUR(ESP_CMD_TCPIP_CIPCLOSE);  /* Set if action was forced = current action = close connection */
                esp.evt.evt.conn_active_close.res = espOK;
                espi_send_conn_cb(conn, NULL);  /* Send event */

                /*
                 * In case we received x,CLOSED on connection we are currently sending data,
                 * terminate sending of connection with failure
                 */
                if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSEND)) {
                    if (esp.msg->msg.conn_send.conn == conn) {
                        /** \todo: Find better idea to handle what to do in this case */
                        //is_error = 1;           /* Set as error to stop processing or waiting for connection */
                    }
                }
            }

            /* Check if write buffer is set */
            if (conn->buff.buff != NULL) {
                ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
                    "[CONN] Free write buffer: %p\r\n", conn->buff.buff);
                esp_mem_free_s((void **)&conn->buff.buff);
            }
        }
    } else if (is_error && CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTART)) {
        /*
         * Notify user about failed connection,
         * but only if connection callback is known.
         *
         * This will prevent notifying wrong connection
         * in case connection is already active for some reason
         * but new callback is not set by user
         */
        if (esp.msg->msg.conn_start.evt_func != NULL) { /* Connection must be closed */
            espi_send_conn_error_cb(esp.msg, espERRCONNFAIL);
        }
    }

    /*
     * In case of any of these events, simply release semaphore
     * and proceed with next command
     */
    if (is_ok || is_error || is_ready) {
        espr_t res = espOK;
        if (esp.msg != NULL) {                  /* Do we have active message? */
            res = espi_process_sub_cmd(esp.msg, &is_ok, &is_error, &is_ready);
            if (res != espCONT) {               /* Shall we continue with next subcommand under this one? */
                if (is_ok || is_ready) {        /* Check ready or ok status */
                    res = esp.msg->res = espOK;
                } else {                        /* Or error status */
                    res = esp.msg->res = res;   /* Set the error status */
                }
            } else {
                esp.msg->i++;                   /* Number of continue calls */
            }

            /*
             * When the command is finished,
             * release synchronization semaphore
             * from user thread and start with next command
             */
            if (res != espCONT) {               /* Do we have to continue to wait for command? */
                esp_sys_sem_release(&esp.sem_sync); /* Release semaphore */
            }
        }
    }
}

#if !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__
/**
 * \brief           Process data from input buffer
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_process_buffer(void) {
    void* data;
    size_t len;

    do {
        /*
         * Get length of linear memory in buffer
         * we can process directly as memory
         */
        len = esp_buff_get_linear_block_read_length(&esp.buff);
        if (len > 0) {
            /*
             * Get memory address of first element
             * in linear block of data to process
             */
            data = esp_buff_get_linear_block_read_address(&esp.buff);

            /* Process actual received data */
            espi_process(data, len);

            /*
             * Once data is processed, simply skip
             * the buffer memory and start over
             */
            esp_buff_skip(&esp.buff, len);
        }
    } while (len);
    return espOK;
}
#endif /* !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__ */

/**
 * \brief           Process input data received from ESP device
 * \param[in]       data: Pointer to data to process
 * \param[in]       data_len: Length of data to process in units of bytes
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_process(const void* data, size_t data_len) {
    uint8_t ch;
    const uint8_t* d = data;
    size_t d_len = data_len;
    static uint8_t ch_prev1, ch_prev2;
    static esp_unicode_t unicode;

    /* Check status if device is available */
    if (!esp.status.f.dev_present) {
        return espERRNODEVICE;
    }

    while (d_len) {                             /* Read entire set of characters from buffer */
        ch = *d++;                              /* Get next character */
        d_len--;                                /* Decrease remaining length */

        /*
         * First check if we are in IPD mode and process plain data
         * without checking for valid ASCII or unicode format
         */
        if (esp.m.ipd.read) {                   /* Do we have to read incoming IPD data? */
            size_t len;

            if (esp.m.ipd.buff != NULL) {       /* Do we have active buffer? */
                esp.m.ipd.buff->payload[esp.m.ipd.buff_ptr] = ch;   /* Save data character */
            }
            esp.m.ipd.buff_ptr++;
            esp.m.ipd.rem_len--;

            /* Try to read more data directly from buffer */
            len = ESP_MIN(d_len, ESP_MIN(esp.m.ipd.rem_len, esp.m.ipd.buff != NULL ? (esp.m.ipd.buff->len - esp.m.ipd.buff_ptr) : esp.m.ipd.rem_len));
            ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                "[IPD] New length to read: %d bytes\r\n", (int)len);
            if (len > 0) {
                if (esp.m.ipd.buff != NULL) {   /* Is buffer valid? */
                    ESP_MEMCPY(&esp.m.ipd.buff->payload[esp.m.ipd.buff_ptr], d, len);
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                        "[IPD] Bytes read: %d\r\n", (int)len);
                } else {                        /* Simply skip the data in buffer */
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                        "[IPD] Bytes skipped: %d\r\n", (int)len);
                }
                d_len -= len;                   /* Decrease effective length */
                d += len;                       /* Skip remaining length */
                esp.m.ipd.buff_ptr += len;      /* Forward buffer pointer */
                esp.m.ipd.rem_len -= len;       /* Decrease remaining length */
            }

            /* Did we reach end of buffer or no more data? */
            if (esp.m.ipd.rem_len == 0 || (esp.m.ipd.buff != NULL && esp.m.ipd.buff_ptr == esp.m.ipd.buff->len)) {
                espr_t res = espOK;

                /* Call user callback function with received data */
                if (esp.m.ipd.buff != NULL) {     /* Do we have valid buffer? */
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
                    size_t pbuf_len;

                    pbuf_len = esp_pbuf_length(esp.m.ipd.buff, 1);
                    esp.m.ipd.conn->tcp_not_ack_bytes += pbuf_len;
                    if (esp.m.ipd.conn->tcp_available_bytes >= pbuf_len) {
                        esp.m.ipd.conn->tcp_available_bytes -= pbuf_len;
                    }
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

                    esp.m.ipd.conn->total_recved += esp.m.ipd.buff->tot_len;  /* Increase number of bytes received */
                    
                    /*
                     * Send data buffer to upper layer
                     *
                     * From this moment, user is responsible for packet
                     * buffer and must free it manually
                     */
                    esp.evt.type = ESP_EVT_CONN_RECV;
                    esp.evt.evt.conn_data_recv.buff = esp.m.ipd.buff;
                    esp.evt.evt.conn_data_recv.conn = esp.m.ipd.conn;
                    res = espi_send_conn_cb(esp.m.ipd.conn, NULL);

                    esp_pbuf_free(esp.m.ipd.buff);  /* Free packet buffer at this point */
                    ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                        "[IPD] Free packet buffer\r\n");
                    if (res == espOKIGNOREMORE) {   /* We should ignore more data */
                        ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                            "[IPD] Ignoring more data from this IPD if available\r\n");
                        esp.m.ipd.buff = NULL;  /* Set to NULL to ignore more data if possibly available */
                    }

                    /*
                     * Create new data packet if case if:
                     *
                     *  - Previous one was successful and more data to read and
                     *  - Connection is not in closing state
                     */
                    if (esp.m.ipd.buff != NULL && esp.m.ipd.rem_len > 0 && !esp.m.ipd.conn->status.f.in_closing) {
                        size_t new_len = ESP_MIN(esp.m.ipd.rem_len, ESP_CFG_IPD_MAX_BUFF_SIZE); /* Calculate new buffer length */

                        ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                            "[IPD] Allocating new packet buffer of size: %d bytes\r\n", (int)new_len);
                        esp.m.ipd.buff = esp_pbuf_new(new_len); /* Allocate new packet buffer */

                        ESP_DEBUGW(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING,
                            esp.m.ipd.buff == NULL, "[IPD] Buffer allocation failed for %d bytes\r\n", (int)new_len);

                        if (esp.m.ipd.buff != NULL) {
                            esp_pbuf_set_ip(esp.m.ipd.buff, &esp.m.ipd.ip, esp.m.ipd.port); /* Set IP and port for received data */
                        }
                    } else {
                        esp.m.ipd.buff = NULL;  /* Reset it */
                    }
                }
                if (esp.m.ipd.rem_len == 0) {   /* Check if we read everything */
                    esp.m.ipd.buff = NULL;      /* Reset buffer pointer */
                    esp.m.ipd.read = 0;         /* Stop reading data */
                }
                esp.m.ipd.buff_ptr = 0;         /* Reset input buffer pointer */
                RECV_RESET();                   /* Reset receive data */
            }

        /*
         * We are in command mode where we have to process byte by byte
         * Simply check for ASCII and unicode format and process data accordingly
         */
        } else {
            espr_t res = espERR;
            if (ESP_ISVALIDASCII(ch)) {         /* Manually check if valid ASCII character */
                res = espOK;
                unicode.t = 1;                  /* Manually set total to 1 */
                unicode.r = 0;                  /* Reset remaining bytes */
            } else if (ch >= 0x80) {            /* Process only if more than ASCII can hold */
                res = espi_unicode_decode(&unicode, ch);    /* Try to decode unicode format */
            }

            if (res == espERR) {                /* In case of an ERROR */
                unicode.r = 0;
            }
            if (res == espOK) {                 /* Can we process the character(s) */
                if (unicode.t == 1) {           /* Totally 1 character? */
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
                    char* tmp_ptr;
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
                    switch (ch) {
                        case '\n':
                            RECV_ADD(ch);       /* Add character to input buffer */
                            espi_parse_received(&recv_buff);/* Parse received string */
                            RECV_RESET();       /* Reset received string */
                            break;
                        default:
                            RECV_ADD(ch);       /* Any ASCII valid character */
                            break;
                    }

                    /* If we are waiting for "\n> " sequence when CIPSEND command is active */
                    if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSEND)) {
                        if (ch_prev2 == '\r' && ch_prev1 == '\n' && ch == '>') {
                            RECV_RESET();       /* Reset received object */

                            /* Now actually send the data prepared before */
                            AT_PORT_SEND_WITH_FLUSH(&esp.msg->msg.conn_send.data[esp.msg->msg.conn_send.ptr], esp.msg->msg.conn_send.sent);
                            esp.msg->msg.conn_send.wait_send_ok_err = 1;    /* Now we are waiting for "SEND OK" or "SEND ERROR" */
                        }
                    }

#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
                    /*
                     * Check if "+CIPRECVDATA" statement is in array and now we received colon,
                     * indicating end of +CIPRECVDATA statement and start of actual data
                     */
                    if (ch == ':' && RECV_LEN() > 12 && RECV_IDX(0) == '+' && !strncmp(recv_buff.data, "+CIPRECVDATA", 12)
                        && (tmp_ptr = strchr(recv_buff.data, ',')) != NULL  // Search for first comma 
                        && strchr(tmp_ptr + 1, ':') != NULL) {  /* Search for second comma, the one just received */
                        espi_parse_received(&recv_buff);    /* Parse received string */
                        if (esp.m.ipd.read) {   /* Shall we start read procedure? */
                            /*
                             * We should have already allocated pbuf memory at this stage
                             * in current message from actual command handle
                             *
                             * Pbuf length should not be bigger than number of received bytes
                             */
                            esp.m.ipd.buff = esp.msg->msg.ciprecvdata.buff;
                            esp.m.ipd.conn = esp.msg->msg.ciprecvdata.conn;
                            esp_pbuf_set_length(esp.m.ipd.buff, esp.m.ipd.tot_len); /* Set new length of buffer */
                            esp.msg->msg.ciprecvdata.buff = NULL;   /* Clear reference for this pbuf */
                        } else {
                            /* ERROR handling */
                        }
                    } else
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

                    /*
                     * Check if "+IPD" statement is in array and now we received colon,
                     * indicating end of +IPD and start of actual data
                     */
                    if (ch == ':' && RECV_LEN() > 4 && RECV_IDX(0) == '+' && !strncmp(recv_buff.data, "+IPD", 4)) {
                        espi_parse_received(&recv_buff);/* Parse received string */
                        if (esp.m.ipd.read) {   /* Shall we start read procedure? */
                            size_t len;
                            ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                                "[IPD] Data on connection %d with total size %d byte(s)\r\n",
                                (int)esp.m.ipd.conn->num, (int)esp.m.ipd.tot_len);

                            len = ESP_MIN(esp.m.ipd.rem_len, ESP_CFG_IPD_MAX_BUFF_SIZE);

                            /*
                             * Read received data in case of:
                             *
                             *  - Connection is active and
                             *  - Connection is not in closing mode
                             */
                            if (esp.m.ipd.conn->status.f.active && !esp.m.ipd.conn->status.f.in_closing) {
                                esp.m.ipd.buff = esp_pbuf_new(len); /* Allocate new packet buffer */
                                if (esp.m.ipd.buff != NULL) {
                                    esp_pbuf_set_ip(esp.m.ipd.buff, &esp.m.ipd.ip, esp.m.ipd.port); /* Set IP and port for received data */
                                }
                                ESP_DEBUGW(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, esp.m.ipd.buff == NULL,
                                    "[IPD] Buffer allocation failed for %d byte(s)\r\n", (int)len);
                            } else {
                                esp.m.ipd.buff = NULL;  /* Ignore reading on closed connection */
                                ESP_DEBUGF(ESP_CFG_DBG_IPD | ESP_DBG_TYPE_TRACE,
                                    "[IPD] Connection %d closed or in closing, skipping %d byte(s)\r\n",
                                    (int)esp.m.ipd.conn->num, (int)len);
                            }
                            esp.m.ipd.conn->status.f.data_received = 1; /* We have first received data */
                            esp.m.ipd.buff_ptr = 0; /* Reset buffer write pointer */
                        }
                        RECV_RESET();           /* Reset received buffer */
                    }
                } else {                        /* We have sequence of unicode characters */
                    /*
                     * Unicode sequence characters are not "meta" characters
                     * so it is safe to just add them to receive array without checking
                     * what are the actual values
                     */
                    for (uint8_t i = 0; i < unicode.t; i++) {
                        RECV_ADD(unicode.ch[i]);/* Add character to receive array */
                    }
                }
            } else if (res != espINPROG) {      /* Not in progress? */
                RECV_RESET();                   /* Invalid character in sequence */
            }
        }

        ch_prev2 = ch_prev1;                    /* Save previous character as previous previous */
        ch_prev1 = ch;                          /* Set current as previous */
    }
    return espOK;
}

/* Temporary macros, only available for inside gsmi_process_sub_cmd function */
/* Set new command, but first check for error on previous */
#define SET_NEW_CMD_CHECK_ERROR(new_cmd) do {   \
    if (!*(is_error)) {                         \
        n_cmd = (new_cmd);                      \
    }                                           \
} while (0)

/* Set new command, ignore result of previous */
#define SET_NEW_CMD(new_cmd) do {               \
    n_cmd = (new_cmd);                          \
} while (0)

/* Set new cmd if condition passes */
#define SET_NEW_CMD_COND(new_cmd, cond)  do {   \
    if ((cond)) {                               \
        n_cmd = (new_cmd);                      \
    }                                           \
} while (0)

/**
 * \brief           Get next sub command for reset or restore sequence
 * \param[in]       msg: Pointer to current message
 * \param[in]       is_ok: Status whether last command result was OK
 * \param[in]       is_error: Status whether last command result was ERROR
 * \param[in]       is_ready: Status whether last command result was ready
 * \return          Next command to execute
 */
static esp_cmd_t
espi_get_reset_sub_cmd(esp_msg_t* msg, uint8_t* is_ok, uint8_t* is_error, uint8_t* is_ready) {
    esp_cmd_t n_cmd = ESP_CMD_IDLE;
    switch (CMD_GET_CUR()) {
        case ESP_CMD_RESET:
        case ESP_CMD_RESTORE:
            SET_NEW_CMD(ESP_CFG_AT_ECHO ? ESP_CMD_ATE1 : ESP_CMD_ATE0); break;
        case ESP_CMD_ATE0:
        case ESP_CMD_ATE1:
            SET_NEW_CMD(ESP_CMD_SYSMSG_CUR); break;
        case ESP_CMD_SYSMSG_CUR:
            SET_NEW_CMD(!*is_ok ? ESP_CMD_GMR : ESP_CMD_SYSMSG); break;
        case ESP_CMD_SYSMSG:
#if ESP_CFG_ESP32 && ESP_CFG_ESP8266
            SET_NEW_CMD(ESP_CMD_BLEINIT_GET); break;
        case ESP_CMD_BLEINIT_GET:
            if (*is_ok) {
                esp.m.device = ESP_DEVICE_ESP32;
            } else {
                esp.m.device = ESP_DEVICE_ESP8266;
            }
#endif /* ESP_CFG_ESP32 */
            SET_NEW_CMD(ESP_CMD_GMR); break;
        case ESP_CMD_GMR: 
            SET_NEW_CMD(ESP_CMD_WIFI_CWMODE); break;
        case ESP_CMD_WIFI_CWMODE: 
            SET_NEW_CMD(ESP_CMD_WIFI_CWDHCP_GET); break;
        case ESP_CMD_WIFI_CWDHCP_GET: 
            SET_NEW_CMD(ESP_CMD_TCPIP_CIPMUX); break;
        case ESP_CMD_TCPIP_CIPMUX:
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
            SET_NEW_CMD(ESP_CMD_TCPIP_CIPRECVMODE); break;
        case ESP_CMD_TCPIP_CIPRECVMODE:
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
#if ESP_CFG_MODE_STATION
            SET_NEW_CMD(ESP_CMD_WIFI_CWLAPOPT); break;/* Set visible data for CWLAP command */
        case ESP_CMD_WIFI_CWLAPOPT: 
            SET_NEW_CMD(ESP_CMD_TCPIP_CIPSTATUS); break;/* Get connection status */
        case ESP_CMD_TCPIP_CIPSTATUS:
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            SET_NEW_CMD(ESP_CMD_WIFI_CIPAP_GET); break; /* Get access point IP */
        case ESP_CMD_WIFI_CIPAP_GET: 
            SET_NEW_CMD(ESP_CMD_WIFI_CIPAPMAC_GET); break; /* Get access point MAC */
        case ESP_CMD_WIFI_CIPAPMAC_GET:
#endif /* ESP_CFG_MODE_STATION */
            SET_NEW_CMD(ESP_CMD_TCPIP_CIPDINFO); break; /* Set visible data on +IPD */
        default: break;
    }
    ESP_UNUSED(msg);
    ESP_UNUSED(is_error);
    ESP_UNUSED(is_ok);
    ESP_UNUSED(is_ready);
    return n_cmd;
}

/**
 * \brief           Process current command with known execution status and start another if necessary
 * \param[in]       msg: Pointer to current message
 * \param[in]       is_ok: Status whether last command result was OK
 * \param[in]       is_error: Status whether last command result was ERROR
 * \param[in]       is_ready: Status whether last command result was ready
 * \return          espCONT if you sent more data and we need to process more data, or espOK on success, or espERR on error
 */
static espr_t
espi_process_sub_cmd(esp_msg_t* msg, uint8_t* is_ok, uint8_t* is_error, uint8_t* is_ready) {
    esp_cmd_t n_cmd = ESP_CMD_IDLE;
    if (CMD_IS_DEF(ESP_CMD_RESET)) {            /* Device is in reset mode */
        n_cmd = espi_get_reset_sub_cmd(msg, is_ok, is_error, is_ready);
        if (n_cmd == ESP_CMD_IDLE) {            /* Last command? */
            RESET_SEND_EVT(msg, *is_ok ? espOK : espERR);
        }
    } else if (CMD_IS_DEF(ESP_CMD_RESTORE)) {
        if ((CMD_IS_CUR(ESP_CMD_RESET)) && *is_ready) {
            SET_NEW_CMD(ESP_CMD_RESTORE);
        } else if ((CMD_IS_CUR(ESP_CMD_RESTORE) && *is_ready)
            || !CMD_IS_CUR(ESP_CMD_RESTORE)) {
            SET_NEW_CMD(espi_get_reset_sub_cmd(msg, is_ok, is_error, is_ready));
        }
        if (n_cmd == ESP_CMD_IDLE) {
            RESTORE_SEND_EVT(msg, *is_ok ? espOK : espERR);
        }
#if ESP_CFG_MODE_STATION
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CWJAP)) {/* Is our intention to join to access point? */
        if (CMD_IS_CUR(ESP_CMD_WIFI_CWJAP)) {   /* Is the current command join? */
            if (*is_ok) {                       /* Did we join successfully? */
                SET_NEW_CMD(ESP_CMD_WIFI_CWDHCP_GET);   /* Check IP address status */
            } else {
                esp.m.sta.is_connected = 0;     /* Force disconnected status */
                /*
                 * Parse received error message,
                 * if final result was error, decide what type
                 * of error should be returned for user
                 */
                switch (msg->msg.sta_join.error_num) {
                    case 1: esp.evt.evt.sta_join_ap.res = espERRCONNTIMEOUT; break;
                    case 2: esp.evt.evt.sta_join_ap.res = espERRPASS;        break;
                    case 3: esp.evt.evt.sta_join_ap.res = espERRNOAP;        break;
                    case 4: esp.evt.evt.sta_join_ap.res = espERRCONNFAIL;    break;
                    default: esp.evt.evt.sta_join_ap.res = espERR;
                }
            }
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_GET)) {
            SET_NEW_CMD(ESP_CMD_WIFI_CIPSTA_GET);   /* Get IP address */
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET)) {
            espi_send_cb(ESP_EVT_WIFI_IP_ACQUIRED); /* Notify upper layer */
            SET_NEW_CMD(ESP_CMD_WIFI_CIPSTAMAC_GET);/* Go to next command to get MAC address */
        } else {
            esp.evt.evt.sta_join_ap.res = espOK;/* Connected ok */
        }

        /* Check command finish */
        if (n_cmd == ESP_CMD_IDLE) {
            STA_JOIN_AP_SEND_EVT(msg, esp.evt.evt.sta_join_ap.res);
        }
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CWLAP)) {
        STA_LIST_AP_SEND_EVT(msg, *is_ok ? espOK : espERR);
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CWJAP_GET)) {
        STA_INFO_AP_SEND_EVT(msg, *is_ok ? espOK : espERR);
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CIPSTA_SET)) {
        if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_SET)) {
            SET_NEW_CMD(ESP_CMD_WIFI_CWDHCP_GET);
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_GET)) {
            SET_NEW_CMD(ESP_CMD_WIFI_CIPSTA_GET);
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET)) {
            espi_send_cb(ESP_EVT_WIFI_IP_ACQUIRED); /* Notify upper layer */
        }
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CIPSTA_GET)) {
        if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_GET)) {
            SET_NEW_CMD(ESP_CMD_WIFI_CIPSTA_GET);
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET)) {
            espi_send_cb(ESP_EVT_WIFI_IP_ACQUIRED);
        }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CWMODE) &&
        (msg->msg.wifi_mode.mode == ESP_MODE_AP
#if ESP_CFG_MODE_STATION
        || msg->msg.wifi_mode.mode == ESP_MODE_STA_AP
#endif /* ESP_CFG_MODE_STATION */
        )) {
        if (CMD_IS_CUR(ESP_CMD_WIFI_CWMODE)) {
            SET_NEW_CMD_COND(ESP_CMD_WIFI_CIPAP_GET, *is_ok);
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAP_GET)) {
            SET_NEW_CMD_COND(ESP_CMD_WIFI_CWDHCP_GET, *is_ok);
        } else if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_GET)) {
            SET_NEW_CMD_COND(ESP_CMD_WIFI_CIPAPMAC_GET, *is_ok);
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
#if ESP_CFG_DNS
    } else if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPDOMAIN)) {
        CIPDOMAIN_SEND_EVT(esp.msg, *is_ok ? espOK : espERR);
#endif /* ESP_CFG_DNS */
#if ESP_CFG_PING
    } else if (CMD_IS_DEF(ESP_CMD_TCPIP_PING)) {
        PING_SEND_EVT(esp.msg, *is_ok ? espOK : espERR);
#endif
    } else if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPSTART)) {/* Is our intention to join to access point? */
        if (msg->i == 0 && CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTATUS)) {   /* Was the current command status info? */
            SET_NEW_CMD_COND(ESP_CMD_TCPIP_CIPSTART, *is_ok);   /* Now actually start connection */
        } else if (msg->i == 1 && CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTART)) {
            SET_NEW_CMD(ESP_CMD_TCPIP_CIPSTATUS);   /* Go to status mode */
        } else if (msg->i == 2 && CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTATUS)) {
            /* Check if connect actually succedded */
            if (!msg->msg.conn_start.success) {
                *is_ok = 0;
                *is_error = 1;
            }
        }
    } else if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPCLOSE)) {
        if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPCLOSE) && *is_error) {
            /* Notify upper layer about failed close event */
            esp.evt.type = ESP_EVT_CONN_CLOSE;
            esp.evt.evt.conn_active_close.conn = msg->msg.conn_close.conn;
            esp.evt.evt.conn_active_close.forced = 1;
            esp.evt.evt.conn_active_close.res = espERR;
            esp.evt.evt.conn_active_close.client = msg->msg.conn_close.conn->status.f.active && msg->msg.conn_close.conn->status.f.client;
            espi_send_conn_cb(msg->msg.conn_close.conn, NULL);
        }
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
    } else if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPRECVDATA)) {
        if (msg->i == 0 && CMD_IS_CUR(ESP_CMD_TCPIP_CIPRECVLEN)) {
            uint8_t set_error = 0;
            ESP_DEBUGW(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_SEVERE, *is_error,
                "[CONN] CIPRECVLEN returned ERROR\r\n");

            /* If command is OK */
            if (*is_ok) {
                size_t len;

                /* Number of bytes to read */
                len = ESP_MIN(ESP_CFG_CONN_MAX_DATA_LEN, msg->msg.ciprecvdata.conn->tcp_available_bytes);
                if (len > 0) {
                    esp_pbuf_p p = NULL;

                    /* Try to allocate packet buffer */
                    while (p == NULL) {
                        p = esp_pbuf_new(len);  /* Try to allocate buffer */
                        if (p == NULL) {        /* In case of failure */
                            len /= 2;           /* Try with half of value on next try */
                            if (len < 10) {     /* If not possible to allocate at least 10 bytes from first try, stop immediately */
                                break;
                            }
                        }
                    }

                    /* Start reading procedure */
                    if (p != NULL) {
                        msg->msg.ciprecvdata.buff = p;
                        msg->msg.ciprecvdata.len = len;
                        SET_NEW_CMD(ESP_CMD_TCPIP_CIPRECVDATA);
                    } else {
                        set_error = 1;
                    }
                } else {
                    /* No error if buffer empty */
                }
            } else {
                set_error = 1;
            }
            if (set_error) {
                *is_ok = 0;
                *is_error = 1;
            }
        } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPRECVDATA)) {
            /* Read failed? Handle queue len */
            if (*is_error) {
                if (msg->msg.ciprecvdata.buff != NULL) {
                    esp_pbuf_free(msg->msg.ciprecvdata.buff);
                    msg->msg.ciprecvdata.buff = NULL;
                }
            }

            /* This one is optional, to check for more data just at the end */
            //SET_NEW_CMD(ESP_CMD_TCPIP_CIPRECVLEN);  /* Inquiry for latest status on data */
        } else if (msg->i == 2 && CMD_IS_CUR(ESP_CMD_TCPIP_CIPRECVLEN)) {

        }
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
    } else if (CMD_IS_DEF(ESP_CMD_WIFI_CWDHCP_SET)) {
        if (CMD_IS_CUR(ESP_CMD_WIFI_CWDHCP_SET)) {
            SET_NEW_CMD(ESP_CMD_WIFI_CWDHCP_GET);
        }
    }

    /* Are we enabling server mode for some reason? */
    if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPSERVER)) {
        if (msg->msg.tcpip_server.en) {
            if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSERVERMAXCONN)) {
                /* Since not all AT versions support CIPSERVERMAXCONN command, ignore result */
                SET_NEW_CMD(ESP_CMD_TCPIP_CIPSERVER);
            } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSERVER)) {
                if (*is_ok) {
                    esp.evt_server = msg->msg.tcpip_server.cb;  /* Set server callback function */
                    SET_NEW_CMD(ESP_CMD_TCPIP_CIPSTO);
                }
            } else if (CMD_IS_CUR(ESP_CMD_TCPIP_CIPSTO)) {
                *is_ok = 1;                     /* Force to 1 */
            }
        }
        if (n_cmd == ESP_CMD_IDLE) {            /* Do we still have execution? */
            esp.evt.evt.server.res = *is_ok ? espOK : espERR;
            esp.evt.evt.server.en = msg->msg.tcpip_server.en;
            esp.evt.evt.server.port = msg->msg.tcpip_server.port;
            espi_send_cb(ESP_EVT_SERVER);       /* Send to upper layer */
        }
    }

    /* Check and start a new command */
    if (n_cmd != ESP_CMD_IDLE) {
        espr_t res;
        msg->cmd = n_cmd;
        if ((res = msg->fn(msg)) == espOK) {
            return espCONT;
        } else {
            *is_ok = 0;
            *is_error = 1;
            return res;
        }
    } else {
        msg->cmd = ESP_CMD_IDLE;
    }
    return *is_ok || *is_ready ? espOK : espERR;
}

/**
 * \brief           Function to initialize every AT command
 * \note            Never call this function directly. Set as initialization function for command and use `msg->fn(msg)`
 * \param[in]       msg: Pointer to \ref esp_msg_t with data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
espi_initiate_cmd(esp_msg_t* msg) {
    switch (CMD_GET_CUR()) {                    /* Check current message we want to send over AT */
        case ESP_CMD_RESET: {                   /* Reset MCU with AT commands */
            /* Try hardware reset first */
            if (esp.ll.reset_fn != NULL && esp.ll.reset_fn(1)) {

                /* Set baudrate to default one */
                esp.ll.uart.baudrate = ESP_CFG_AT_PORT_BAUDRATE;
                esp_ll_init(&esp.ll);           /* Set new baudrate */

                esp_delay(10);                  /* Wait some time */
                esp.ll.reset_fn(0);             /* Release reset */
            } else {
                AT_PORT_SEND_BEGIN_AT();
                AT_PORT_SEND_CONST_STR("+RST");
                AT_PORT_SEND_END_AT();
                /* Do not modify baudrate yet as we need "OK" or "ERROR" response first */
            }
            break;
        }
        case ESP_CMD_RESTORE: {                 /* Reset MCU with AT commands */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+RESTORE");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_ATE0: {                    /* Disable AT echo mode */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("E0");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_ATE1: {                    /* Enable AT echo mode */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("E1");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_GMR: {                     /* Get AT version */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+GMR");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_SYSMSG: {                  /* Set system messages */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+SYSMSG=3");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_SYSMSG_CUR: {              /* Set system messages */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+SYSMSG_CUR=3");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_UART: {                    /* Change UART parameters for AT port */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+UART_CUR=");
            espi_send_number(ESP_U32(msg->msg.uart.baudrate), 0, 0);
            AT_PORT_SEND_CONST_STR(",8,1,0,0");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWLAPOPT: {           /* Set visible data on CWLAP command */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWLAPOPT=1,31");
            AT_PORT_SEND_END_AT();
            break;
        }

        /* WiFi related commands */

#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CWJAP: {              /* Try to join to access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWJAP=");
            espi_send_string(msg->msg.sta_join.name, 1, 1, 0);
            espi_send_string(msg->msg.sta_join.pass, 1, 1, 1);
            if (msg->msg.sta_join.mac != NULL) {
                espi_send_ip_mac(msg->msg.sta_join.mac, 0, 1, 1);
            }
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWJAP_GET: {          /* Get the info of the connected access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWJAP?");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWQAP: {              /* Quit from access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWQAP");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWLAP: {              /* List access points */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWLAP");
            if (msg->msg.ap_list.ssid != NULL) {/* Do we want to filter by SSID? */
                AT_PORT_SEND_CONST_STR("=");
                espi_send_string(msg->msg.ap_list.ssid, 1, 1, 0);
            }
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWAUTOCONN: {         /* Set autoconnect feature */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWAUTOCONN=");
            espi_send_number(ESP_U32(!!msg->msg.sta_autojoin.en), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIUPDATE: {          /* Update ESP software remotely */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIUPDATE");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_MODE_STATION */
        case ESP_CMD_WIFI_CWMODE: {             /* Set WIFI mode */
            esp_mode_t m;

            if (!CMD_IS_DEF(ESP_CMD_WIFI_CWMODE)) { /* Is this command part of reset sequence? */
#if ESP_CFG_MODE_STATION_ACCESS_POINT
                m = ESP_MODE_STA_AP;            /* Set station and access point mode */
#elif ESP_CFG_MODE_STATION
                m = ESP_MODE_STA;               /* Set station mode */
#else /* ESP_CFG_MODE_STATION */
                m = ESP_MODE_AP;                /* Set access point mode */
#endif /* !ESP_CFG_MODE_STATION_ACCESS_POINT */
            } else {
                /* Use user setup */
                m = msg->msg.wifi_mode.mode;
            }

            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWMODE=");
            espi_send_number(ESP_U32(m), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTA_GET:           /* Get station IP address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAP_GET:            /* Get access point IP address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIP");
#if ESP_CFG_MODE_STATION
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_GET)) {
                AT_PORT_SEND_CONST_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAP_GET)) {
                AT_PORT_SEND_CONST_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            AT_PORT_SEND_CONST_STR("?");
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTAMAC_GET:        /* Get station MAC address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAPMAC_GET:         /* Get access point MAC address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIP");
#if ESP_CFG_MODE_STATION
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTAMAC_GET)) {
                AT_PORT_SEND_CONST_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAPMAC_GET)) {
                AT_PORT_SEND_CONST_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            AT_PORT_SEND_CONST_STR("MAC?");
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTA_SET:           /* Set station IP address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAP_SET:            /* Set access point IP address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIP");
#if ESP_CFG_MODE_STATION
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTA_SET)) {
                AT_PORT_SEND_CONST_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAP_SET)) {
                AT_PORT_SEND_CONST_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            AT_PORT_SEND_CONST_STR("=");
            espi_send_ip_mac(msg->msg.sta_ap_setip.ip, 1, 1, 0);/* Send IP address */
            if (msg->msg.sta_ap_setip.gw != NULL) { /* Is gateway set? */
                espi_send_ip_mac(msg->msg.sta_ap_setip.gw, 1, 1, 1);/* Send gateway address */
                if (msg->msg.sta_ap_setip.nm != NULL) { /* Is netmask set ? */
                    espi_send_ip_mac(msg->msg.sta_ap_setip.nm, 1, 1, 1);/* Send netmask address */
                }
            }
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CIPSTAMAC_SET:        /* Set station MAC address */
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CIPAPMAC_SET:         /* Set access point MAC address */
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIP");
#if ESP_CFG_MODE_STATION
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPSTAMAC_SET)) {
                AT_PORT_SEND_CONST_STR("STA");
            }
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_MODE_ACCESS_POINT
            if (CMD_IS_CUR(ESP_CMD_WIFI_CIPAPMAC_SET)) {
                AT_PORT_SEND_CONST_STR("AP");
            }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
            AT_PORT_SEND_CONST_STR("MAC=");
            espi_send_ip_mac(msg->msg.sta_ap_setmac.mac, 0, 1, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWDHCP_GET: {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWDHCP?");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWDHCP_SET: {
            uint32_t num = 0;

            /* Configure DHCP setup */
            num |= (msg->msg.wifi_cwdhcp.sta > 0 ? 0x01 : 0x00);
            num |= (msg->msg.wifi_cwdhcp.ap > 0 ? 0x02 : 0x00);

            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWDHCP=");
            espi_send_number(ESP_U32(msg->msg.wifi_cwdhcp.en > 0), 0, 0);
            espi_send_number(num, 0, 1);
            AT_PORT_SEND_END_AT();
            break;
        }

#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_CMD_WIFI_CWSAP_SET: {          /* Set access point parameters */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWSAP=");
            espi_send_string(msg->msg.ap_conf.ssid, 1, 1, 0);
            espi_send_string(msg->msg.ap_conf.pwd, 1, 1, 1);
            espi_send_number(ESP_U32(msg->msg.ap_conf.ch), 0, 1);
            espi_send_number(ESP_U32(msg->msg.ap_conf.ecn), 0, 1);
            espi_send_number(ESP_U32(msg->msg.ap_conf.max_sta), 0, 1);
            espi_send_number(ESP_U32(!!msg->msg.ap_conf.hid), 0, 1);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWLIF: {              /* List stations connected on access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWLIF");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
#if ESP_CFG_WPS
        case ESP_CMD_WIFI_WPS: {                /* Enable WPS function */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+WPS=");
            espi_send_number(ESP_U32(!!msg->msg.wps_cfg.en), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_WPS */
#if ESP_CFG_HOSTNAME
        case ESP_CMD_WIFI_CWHOSTNAME_SET: {     /* List stations connected on access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWHOSTNAME=");
            espi_send_string(msg->msg.wifi_hostname.hostname_set, 1, 1, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_WIFI_CWHOSTNAME_GET: {     /* List stations connected on access point */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CWHOSTNAME?");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_HOSTNAME */
#if ESP_CFG_MDNS
        case ESP_CMD_WIFI_MDNS: {               /* Set mDNS parameters */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+MDNS=");
            if (msg->msg.mdns.en) {             /* Send the rest only in case mDNS should be enabled */
                AT_PORT_SEND_CONST_STR("1");
                espi_send_string(msg->msg.mdns.host, 0, 1, 1);
                espi_send_string(msg->msg.mdns.server, 0, 1, 1);
                espi_send_port(msg->msg.mdns.port, 0, 1);
            } else {
                AT_PORT_SEND_CONST_STR("0");
            }
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_MDNS */

        /* TCP/IP related commands */

        case ESP_CMD_TCPIP_CIPSERVER: {         /* Enable or disable server */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSERVER=");
            if (msg->msg.tcpip_server.en) {     /* Do we want to enable server? */
                AT_PORT_SEND_CONST_STR("1");
                espi_send_port(msg->msg.tcpip_server.port, 0, 1);
            } else {                            /* Disable server */
                AT_PORT_SEND_CONST_STR("0");
            }
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPSERVERMAXCONN: {  /* Maximal number of connections */
            uint16_t max_conn;
            if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPSERVER)) {
                max_conn = ESP_MIN(msg->msg.tcpip_server.max_conn, ESP_CFG_MAX_CONNS);
            } else {
                max_conn = ESP_CFG_MAX_CONNS;
            }
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSERVERMAXCONN=");
            espi_send_number(ESP_U32(max_conn), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPSTO: {            /* Set server connection timeout */
            uint16_t timeout;
            if (CMD_IS_DEF(ESP_CMD_TCPIP_CIPSERVER)) {
                timeout = msg->msg.tcpip_server.timeout;
            } else {
                timeout = 100;
            }
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSTO=");
            espi_send_number(ESP_U32(timeout), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_MODE_STATION
        case ESP_CMD_TCPIP_CIPSTART: {          /* Start a new connection */
            esp_conn_t* c = NULL;

            /* Do we have wifi connection? */
            if (!esp_sta_has_ip()) {
                espi_send_conn_error_cb(msg, espERRNOIP);
                return espERRNOIP;
            }

            msg->msg.conn_start.num = 0;        /* Start with max value = invalidated */
            for (int16_t i = ESP_CFG_MAX_CONNS - 1; i >= 0; i--) {  /* Find available connection */
                if (!esp.m.conns[i].status.f.active
                    || !(esp.m.active_conns & (1 << i))) {
                    c = &esp.m.conns[i];
                    c->num = ESP_U8(i);
                    msg->msg.conn_start.num = ESP_U8(i);    /* Set connection number for message structure */
                    break;
                }
            }
            if (c == NULL) {
                espi_send_conn_error_cb(msg, espERRNOFREECONN);
                return espERRNOFREECONN;        /* We don't have available connection */
            }

            if (msg->msg.conn_start.conn != NULL) { /* Is user interested about connection info? */
                *msg->msg.conn_start.conn = c;  /* Save connection for user */
            }

            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSTART=");
            espi_send_number(ESP_U32(c->num), 0, 0);
            if (msg->msg.conn_start.type == ESP_CONN_TYPE_SSL) {
                espi_send_string("SSL", 0, 1, 1);
            } else if (msg->msg.conn_start.type == ESP_CONN_TYPE_TCP) {
                espi_send_string("TCP", 0, 1, 1);
            } else if (msg->msg.conn_start.type == ESP_CONN_TYPE_UDP) {
                espi_send_string("UDP", 0, 1, 1);
            }
            espi_send_string(msg->msg.conn_start.remote_host, 0, 1, 1);
            espi_send_port(msg->msg.conn_start.remote_port, 0, 1);

            /* Connection-type specific features */
            if (msg->msg.conn_start.type != ESP_CONN_TYPE_UDP) {
                espi_send_number(ESP_U32(msg->msg.conn_start.tcp_ssl_keep_alive), 0, 1);
            } else {
                if (msg->msg.conn_start.udp_local_port > 0) {
                    espi_send_port(msg->msg.conn_start.udp_local_port, 0, 1);
                } else {
                    AT_PORT_SEND_CONST_STR(",");
                }
                espi_send_number(msg->msg.conn_start.udp_mode, 0, 1);
            }
            if (msg->msg.conn_start.local_ip != NULL) {
                espi_send_string(msg->msg.conn_start.local_ip, 0, 1, 1);
            }
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_MODE_STATION */

        case ESP_CMD_TCPIP_CIPCLOSE: {          /* Close the connection */
            esp_conn_p c = msg->msg.conn_close.conn;
            if (c != NULL &&
                /*
                 * Is connection already closed or command
                 * for this connection is not valid anymore?
                 */
                (!esp_conn_is_active(c) || c->val_id != msg->msg.conn_close.val_id)) {
                return espERR;
            }
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPCLOSE=");
            espi_send_number(ESP_U32(msg->msg.conn_close.conn ? msg->msg.conn_close.conn->num : ESP_CFG_MAX_CONNS), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPSEND: {           /* Send data to connection */
            return espi_tcpip_process_send_data();  /* Process send data */
        }
        case ESP_CMD_TCPIP_CIPSTATUS: {         /* Get status of device and all connections */
            esp.m.active_conns_last = esp.m.active_conns;   /* Save as last status */
            esp.m.active_conns = 0;             /* Reset new status before parsing starts */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSTATUS");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPDINFO: {          /* Set info data on +IPD command */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPDINFO=1");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPMUX: {            /* Set multiple connections */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPMUX=1");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPSSLSIZE: {        /* Set SSL size */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSSLSIZE=");
            espi_send_number(ESP_U32(msg->msg.tcpip_sslsize.size), 0, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
        case ESP_CMD_TCPIP_CIPRECVMODE: {       /* Set TCP data receive mode */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPRECVMODE=1");
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPRECVDATA: {       /* Manually read data */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPRECVDATA=");
            // Store connection. As Old ESP FW does not report connection ID in
            // repsponse this relies on next reponse will be response to this
            // command.
            esp.m.ipd.conn = &esp.m.conns[msg->msg.ciprecvdata.conn->num];
            espi_send_number(ESP_U32(msg->msg.ciprecvdata.conn->num), 0, 0);
            espi_send_number(ESP_U32(msg->msg.ciprecvdata.len), 0, 1);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPRECVLEN: {        /* Get length to read */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPRECVLEN?");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
#if ESP_CFG_DNS
        case ESP_CMD_TCPIP_CIPDOMAIN: {         /* DNS function */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPDOMAIN=");
            espi_send_string(msg->msg.dns_getbyhostname.host, 1, 1, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPDNS_SET: {        /* DNS set config */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPDNS=");
            espi_send_number(ESP_U32(!!msg->msg.dns_setconfig.en), 0, 0);
            if (msg->msg.dns_setconfig.en) {
                if (msg->msg.dns_setconfig.s1 != NULL) {
                    espi_send_string(msg->msg.dns_setconfig.s1, 0, 1, 1);
                }
                if (msg->msg.dns_setconfig.s2 != NULL) {
                    espi_send_string(msg->msg.dns_setconfig.s2, 0, 1, 1);
                }
            }
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_DNS */
#if ESP_CFG_PING
        case ESP_CMD_TCPIP_PING: {              /* Ping hostname or IP address */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+PING=");
            espi_send_string(msg->msg.tcpip_ping.host, 1, 1, 0);
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_PING */
#if ESP_CFG_SNTP
        case ESP_CMD_TCPIP_CIPSNTPCFG: {        /* Configure SNTP */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSNTPCFG=");
            espi_send_number(ESP_U32(!!msg->msg.tcpip_sntp_cfg.en), 0, 0);
            espi_send_signed_number(ESP_U32(msg->msg.tcpip_sntp_cfg.tz), 0, 1);
            if (msg->msg.tcpip_sntp_cfg.h1 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h1)) {
                espi_send_string(msg->msg.tcpip_sntp_cfg.h1, 0, 1, 1);
            }
            if (msg->msg.tcpip_sntp_cfg.h2 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h2)) {
                espi_send_string(msg->msg.tcpip_sntp_cfg.h2, 0, 1, 1);
            }
            if (msg->msg.tcpip_sntp_cfg.h3 != NULL && strlen(msg->msg.tcpip_sntp_cfg.h3)) {
                espi_send_string(msg->msg.tcpip_sntp_cfg.h3, 0, 1, 1);
            }
            AT_PORT_SEND_END_AT();
            break;
        }
        case ESP_CMD_TCPIP_CIPSNTPTIME: {       /* Get time over SNTP */
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+CIPSNTPTIME?");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_SNTP */

#if ESP_CFG_ESP32
        case ESP_CMD_BLEINIT_GET: {
            AT_PORT_SEND_BEGIN_AT();
            AT_PORT_SEND_CONST_STR("+BLEINIT?");
            AT_PORT_SEND_END_AT();
            break;
        }
#endif /* ESP_CFG_ESP32 */

        default:
            return espERR;                      /* Invalid command */
    }
    return espOK;                               /* Valid command */
}

/**
 * \brief           Checks if connection pointer has valid address
 * \param[in]       conn: Address to check if valid connection ptr
 * \return          `1` on success, `0` otherwise
 */
uint8_t
espi_is_valid_conn_ptr(esp_conn_p conn) {
    for (size_t i = 0; i < ESP_ARRAYSIZE(esp.m.conns); i++) {
        if (conn == &esp.m.conns[i]) {
            return 1;
        }
    }
    return 0;
}

/**
 * \brief           Send message from API function to producer queue for further processing
 * \param[in]       msg: New message to process
 * \param[in]       process_fn: callback function used to process message
 * \param[in]       max_block_time: Maximal time command can block in units of milliseconds
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_send_msg_to_producer_mbox(esp_msg_t* msg, espr_t (*process_fn)(esp_msg_t *), uint32_t max_block_time) {
    espr_t res = msg->res = espOK;

    /* Check here if stack is even enabled or shall we disable new command entry? */
    esp_core_lock();
    /* If locked more than 1 time, means we were called from callback or internally */
    if (esp.locked_cnt > 1 && msg->is_blocking) {
        res = espERRBLOCKING;                   /* Blocking mode not allowed */
    }
    /* Check if device present */
    if (res == espOK && !esp.status.f.dev_present) {
        res = espERRNODEVICE;                   /* No device connected */
    }
    esp_core_unlock();
    if (res != espOK) {
        ESP_MSG_VAR_FREE(msg);                  /* Free memory and return */
        return res;
    }

    if (msg->is_blocking) {                     /* In case message is blocking */
        if (!esp_sys_sem_create(&msg->sem, 0)) {/* Create semaphore and lock it immediatelly */
            ESP_MSG_VAR_FREE(msg);              /* Release memory and return */
            return espERRMEM;
        }
    }
    if (!msg->cmd) {                            /* Set start command if not set by user */
        msg->cmd = msg->cmd_def;                /* Set it as default */
    }
    msg->block_time = max_block_time;           /* Set blocking status if necessary */
    msg->fn = process_fn;                       /* Save processing function to be called as callback */
    if (msg->is_blocking) {
        esp_sys_mbox_put(&esp.mbox_producer, msg);  /* Write message to producer queue and wait forever */
    } else {
        if (!esp_sys_mbox_putnow(&esp.mbox_producer, msg)) {    /* Write message to producer queue immediatelly */
            ESP_MSG_VAR_FREE(msg);              /* Release message */
            return espERRMEM;
        }
    }
    if (res == espOK && msg->is_blocking) {     /* In case we have blocking request */
        uint32_t time;
        time = esp_sys_sem_wait(&msg->sem, 0);  /* Wait forever for semaphore */
        if (time == ESP_SYS_TIMEOUT) {          /* If semaphore was not accessed in given time */
            res = espTIMEOUT;                   /* Semaphore not released in time */
        } else {
            res = msg->res;                     /* Set response status from message response */
        }
        ESP_MSG_VAR_FREE(msg);                  /* Release message */
    }
    return res;
}

/**
 * \brief           Process events in case of timeout on command or invalid message (if device is not present)
 *
 *                  Function is called from processing thread:
 *
 *                      - On command timeout error
 *                      - If command was sent to queue and before processed, device present status changed
 *
 * \param[in]       msg: Current message
 * \param[in]       err: Error message to send
 */
void
espi_process_events_for_timeout_or_error(esp_msg_t* msg, espr_t err) {
    switch (msg->cmd_def) {
        case ESP_CMD_RESET: {
            /* Error on reset */
            RESET_SEND_EVT(msg, err);
            break;
        }

        case ESP_CMD_RESTORE: {
            /* Error on restore */
            RESTORE_SEND_EVT(msg, err);
            break;
        }

        case ESP_CMD_TCPIP_CIPSTART: {
            /* Start connection error */
            espi_send_conn_error_cb(msg, err);
            break;
        }

        case ESP_CMD_TCPIP_CIPSEND: {
            /* Send data error */
            CONN_SEND_DATA_SEND_EVT(msg, err);
            break;
        }

#if ESP_CFG_MODE_STATION
        case ESP_CMD_WIFI_CWJAP: {
            /* Join access point error */
            STA_JOIN_AP_SEND_EVT(msg, err);
            break;
        }

        case ESP_CMD_WIFI_CWLAP: {
            /* List failed event */
            STA_LIST_AP_SEND_EVT(msg, err);
            break;
        }

        case ESP_CMD_WIFI_CWJAP_GET: {
            /* Info failed event */
            STA_INFO_AP_SEND_EVT(msg, err);
            break;
        }
#endif /* ESP_CFG_MODE_STATION */

#if ESP_CFG_PING
        case ESP_CMD_TCPIP_PING: {
            /* Ping error */
            PING_SEND_EVT(msg, err);
            break;
        }
#endif /* ESP_CFG_PING */

#if ESP_CFG_DNS
        case ESP_CMD_TCPIP_CIPDOMAIN: {
            /* DNS error */
            CIPDOMAIN_SEND_EVT(msg, err);
            break;
        }
#endif /* ESP_CFG_DNS */

        default: break;
    }
}
