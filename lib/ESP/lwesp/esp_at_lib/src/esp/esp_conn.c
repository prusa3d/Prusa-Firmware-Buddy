/**
 * \file            esp_conn.c
 * \brief           Connection API
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
#include "esp/esp_conn.h"
#include "esp/esp_mem.h"
#include "esp/esp_timeout.h"

/**
 * \brief           Check if connection is closed or in closing state
 * \param[in]       conn: Connection handle
 * \hideinitializer
 */
#define CONN_CHECK_CLOSED_IN_CLOSING(conn) do { \
    espr_t r = espOK;                           \
    esp_core_lock();                            \
    if (conn->status.f.in_closing || !conn->status.f.active) {  \
        r = espCLOSED;                          \
    }                                           \
    esp_core_unlock();                          \
    if (r != espOK) {                           \
        return r;                               \
    }                                           \
} while (0)

/**
 * \brief           Timeout callback for connection
 * \param[in]       arg: Timeout callback custom argument
 */
static void
conn_timeout_cb(void* arg) {
    esp_conn_p conn = arg;                      /* Argument is actual connection */

    if (conn->status.f.active) {                /* Handle only active connections */
        esp.evt.type = ESP_EVT_CONN_POLL;       /* Poll connection event */
        esp.evt.evt.conn_poll.conn = conn;      /* Set connection pointer */
        espi_send_conn_cb(conn, NULL);          /* Send connection callback */

        espi_conn_start_timeout(conn);          /* Schedule new timeout */
        ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
            "[CONN] Poll event: %p\r\n", conn);
    }

#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
    espi_conn_manual_tcp_try_read_data(conn);   /* Try to read data manually */
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
}

/**
 * \brief           Start timeout function for connection
 * \param[in]       conn: Connection handle as user argument
 */
void
espi_conn_start_timeout(esp_conn_p conn) {
    esp_timeout_add(ESP_CFG_CONN_POLL_INTERVAL, conn_timeout_cb, conn); /* Add connection timeout */
}

#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE

/**
 * \brief           Callback function when manual TCP receive finishes
 * \param[in]       res: Result of reading
 * \param[in]       arg: Custom user argument
 */
static void
manual_tcp_read_data_evt_fn(espr_t res, void* arg) {
    esp_conn_p conn = arg;

    conn->status.f.receive_is_command_queued = 0;
    espi_conn_manual_tcp_try_read_data(conn);
}

/**
 * \brief           Manually start data read operation with desired length on specific connection
 * \param[in]       conn: Connection handle
 * \param[in]       len: Number of bytes to read
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
espi_conn_manual_tcp_try_read_data(esp_conn_p conn) {
    uint32_t blocking = 0;
    espr_t res = espOK;
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("conn != NULL", conn != NULL);

    /* Receive must not be blocked and other command must not be in queue to read data */
    if (conn->status.f.receive_blocked
        || conn->status.f.receive_is_command_queued) {
        return espINPROG;
    }

    /* Any available data to process? */
    if (conn->tcp_available_bytes == 0) {
        return espERR;
    }

    ESP_MSG_VAR_ALLOC(msg, blocking);           /* Allocate first, will return on failure */
    ESP_MSG_VAR_SET_EVT(msg, manual_tcp_read_data_evt_fn, conn);/* Set event callback function */
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPRECVDATA;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPRECVLEN;
    ESP_MSG_VAR_REF(msg).msg.ciprecvdata.len = 0;   /* Filled after RECVLEN received */
    ESP_MSG_VAR_REF(msg).msg.ciprecvdata.buff = NULL;   /* Filled after RECVLEN received */
    ESP_MSG_VAR_REF(msg).msg.ciprecvdata.conn = conn;

    /* Try to start command */
    if ((res = espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 60000)) == espOK) {
        conn->status.f.receive_is_command_queued = 1;   /* Command queued */
    }
    return res;
}
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

/**
 * \brief           Get connection validation ID
 * \param[in]       conn: Connection handle
 * \return          Connection current validation ID
 */
uint8_t
espi_conn_get_val_id(esp_conn_p conn) {
    uint8_t val_id;
    esp_core_lock();
    val_id = conn->val_id;
    esp_core_unlock();

    return val_id;
}

/**
 * \brief           Send data on already active connection of type UDP to specific remote IP and port
 * \note            In case IP and port values are not set, it will behave as normal send function (suitable for TCP too)
 * \param[in]       conn: Pointer to connection to send data
 * \param[in]       ip: Remote IP address for UDP connection
 * \param[in]       port: Remote port connection
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       fau: "Free After Use" flag. Set to `1` if stack should free the memory after data sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
static espr_t
conn_send(esp_conn_p conn, const esp_ip_t* const ip, esp_port_t port, const void* data,
            size_t btw, size_t* const bw, uint8_t fau, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("conn != NULL", conn != NULL);
    ESP_ASSERT("data != NULL", data != NULL);
    ESP_ASSERT("btw > 0", btw > 0);

    if (bw != NULL) {
        *bw = 0;
    }

    CONN_CHECK_CLOSED_IN_CLOSING(conn);         /* Check if we can continue */

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSEND;

    ESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_send.data = data;
    ESP_MSG_VAR_REF(msg).msg.conn_send.btw = btw;
    ESP_MSG_VAR_REF(msg).msg.conn_send.bw = bw;
    ESP_MSG_VAR_REF(msg).msg.conn_send.remote_ip = ip;
    ESP_MSG_VAR_REF(msg).msg.conn_send.remote_port = port;
    ESP_MSG_VAR_REF(msg).msg.conn_send.fau = fau;
    ESP_MSG_VAR_REF(msg).msg.conn_send.val_id = espi_conn_get_val_id(conn);

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 60000);
}

/**
 * \brief           Flush buffer on connection
 * \param[in]       conn: Connection to flush buffer on
 * \return          \ref espOK if data flushed and put to queue, member of \ref espr_t otherwise
 */
static espr_t
flush_buff(esp_conn_p conn) {
    espr_t res = espOK;
    esp_core_lock();
    if (conn != NULL && conn->buff.buff != NULL) {  /* Do we have something ready? */
        /*
         * If there is nothing to write or if write was not successful,
         * simply free the memory and stop execution
         */
        if (conn->buff.ptr > 0) {               /* Anything to send at the moment? */
            res = conn_send(conn, NULL, 0, conn->buff.buff, conn->buff.ptr, NULL, 1, 0);
        } else {
            res = espERR;
        }
        if (res != espOK) {
            ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
                "[CONN] Free write buffer: %p\r\n", (void *)conn->buff.buff);
            esp_mem_free_s((void **)&conn->buff.buff);
        }
        conn->buff.buff = NULL;
    }
    esp_core_unlock();
    return res;
}

/**
 * \brief           Initialize connection module
 */
void
espi_conn_init(void) {

}

/**
 * \brief           Start a new connection of specific type
 * \param[out]      conn: Pointer to connection handle to set new connection reference in case of successfully connected
 * \param[in]       type: Connection type. This parameter can be a value of \ref esp_conn_type_t enumeration
 * \param[in]       remote_host: Connection host. In case of IP, write it as string, ex. "192.168.1.1"
 * \param[in]       remote_port: Connection port
 * \param[in]       arg: Pointer to user argument passed to connection if successfully connected
 * \param[in]       conn_evt_fn: Callback function for this connection
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_start(esp_conn_p* conn, esp_conn_type_t type, const char* const remote_host, esp_port_t remote_port,
                void* const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("remote_host != NULL", remote_host != NULL);
    ESP_ASSERT("remote_port > 0", remote_port > 0);
    ESP_ASSERT("conn_evt_fn != NULL", conn_evt_fn != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSTART;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSTATUS;
    ESP_MSG_VAR_REF(msg).msg.conn_start.num = ESP_CFG_MAX_CONNS;/* Set maximal value as invalid number */
    ESP_MSG_VAR_REF(msg).msg.conn_start.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.type = type;
    ESP_MSG_VAR_REF(msg).msg.conn_start.remote_host = remote_host;
    ESP_MSG_VAR_REF(msg).msg.conn_start.remote_port = remote_port;
    ESP_MSG_VAR_REF(msg).msg.conn_start.evt_func = conn_evt_fn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.arg = arg;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 60000);
}

/**
 * \brief           Start a new connection of specific type in extended mode
 * \param[out]      conn: Pointer to connection handle to set new connection reference in case of successfully connected
 * \param[in]       start_struct: Connection information are handled by one giant structure
 * \param[in]       arg: Pointer to user argument passed to connection if successfully connected
 * \param[in]       conn_evt_fn: Callback function for this connection
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_startex(esp_conn_p* conn, esp_conn_start_t* start_struct,
    void* const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("start_struct != NULL", start_struct != NULL);
    ESP_ASSERT("start_struct->remote_host != NULL", start_struct->remote_host != NULL);
    ESP_ASSERT("start_struct->remote_port > 0", start_struct->remote_port > 0);
    ESP_ASSERT("conn_evt_fn != NULL", conn_evt_fn != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSTART;
    ESP_MSG_VAR_REF(msg).cmd = ESP_CMD_TCPIP_CIPSTATUS;
    ESP_MSG_VAR_REF(msg).msg.conn_start.num = ESP_CFG_MAX_CONNS;/* Set maximal value as invalid number */
    ESP_MSG_VAR_REF(msg).msg.conn_start.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.type = start_struct->type;
    ESP_MSG_VAR_REF(msg).msg.conn_start.remote_host = start_struct->remote_host;
    ESP_MSG_VAR_REF(msg).msg.conn_start.remote_port = start_struct->remote_port;
    ESP_MSG_VAR_REF(msg).msg.conn_start.local_ip = start_struct->local_ip;
    ESP_MSG_VAR_REF(msg).msg.conn_start.evt_func = conn_evt_fn;
    ESP_MSG_VAR_REF(msg).msg.conn_start.arg = arg;

    /* Add connection type specific features */
    if (start_struct->type != ESP_CONN_TYPE_UDP) {
        ESP_MSG_VAR_REF(msg).msg.conn_start.tcp_ssl_keep_alive = start_struct->ext.tcp_ssl.keep_alive;
    } else {
        ESP_MSG_VAR_REF(msg).msg.conn_start.udp_local_port = start_struct->ext.udp.local_port;
        ESP_MSG_VAR_REF(msg).msg.conn_start.udp_mode = start_struct->ext.udp.mode;
    }

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 60000);
}

/**
 * \brief           Close specific or all connections
 * \param[in]       conn: Connection handle to close. Set to NULL if you want to close all connections.
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_close(esp_conn_p conn, const uint32_t blocking) {
    espr_t res;
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("conn != NULL", conn != NULL);

    CONN_CHECK_CLOSED_IN_CLOSING(conn);         /* Check if we can continue */

    /* Proceed with close event at this point! */
    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPCLOSE;
    ESP_MSG_VAR_REF(msg).msg.conn_close.conn = conn;
    ESP_MSG_VAR_REF(msg).msg.conn_close.val_id = espi_conn_get_val_id(conn);

    flush_buff(conn);                           /* First flush buffer */
    res = espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
    if (res == espOK && !blocking) {            /* Function succedded in non-blocking mode */
        esp_core_lock();
        ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
            "[CONN] Connection %d set to closing state\r\n", (int)conn->num);
        conn->status.f.in_closing = 1;          /* Connection is in closing mode but not yet closed */
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Send data on active connection of type UDP to specific remote IP and port
 * \note            In case IP and port values are not set, it will behave as normal send function (suitable for TCP too)
 * \param[in]       conn: Connection handle to send data
 * \param[in]       ip: Remote IP address for UDP connection
 * \param[in]       port: Remote port connection
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_sendto(esp_conn_p conn, const esp_ip_t* const ip, esp_port_t port, const void* data,
                size_t btw, size_t* bw, const uint32_t blocking) {
    ESP_ASSERT("conn != NULL", conn != NULL);

    flush_buff(conn);                           /* Flush currently written memory if exists */
    return conn_send(conn, ip, port, data, btw, bw, 0, blocking);
}

/**
 * \brief           Send data on already active connection either as client or server
 * \param[in]       conn: Connection handle to send data
 * \param[in]       data: Data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent.
 *                      Parameter value might not be accurate if you combine \ref esp_conn_write and \ref esp_conn_send functions
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_send(esp_conn_p conn, const void* data, size_t btw, size_t* const bw,
                const uint32_t blocking) {
    espr_t res;
    const uint8_t* d = data;

    ESP_ASSERT("conn != NULL", conn != NULL);
    ESP_ASSERT("data != NULL", data != NULL);
    ESP_ASSERT("btw > 0", btw > 0);

    esp_core_lock();
    if (conn->buff.buff != NULL) {              /* Check if memory available */
        size_t to_copy;
        to_copy = ESP_MIN(btw, conn->buff.len - conn->buff.ptr);
        if (to_copy > 0) {
            ESP_MEMCPY(&conn->buff.buff[conn->buff.ptr], d, to_copy);
            conn->buff.ptr += to_copy;
            d += to_copy;
            btw -= to_copy;
        }
    }
    esp_core_unlock();
    res = flush_buff(conn);                     /* Flush currently written memory if exists */
    if (btw > 0) {                              /* Check for remaining data */
        res = conn_send(conn, NULL, 0, d, btw, bw, 0, blocking);
    }
    return res;
}

/**
 * \brief           Notify connection about received data which means connection is ready to accept more data
 *
 * Once data reception is confirmed, stack will try to send more data to user.
 *
 * \note            Since this feature is not supported yet by AT commands, function is only prototype
 *                  and should be used in connection callback when data are received
 *
 * \note            Function is not thread safe and may only be called from connection event function
 *
 * \param[in]       conn: Connection handle
 * \param[in]       pbuf: Packet buffer received on connection
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_recved(esp_conn_p conn, esp_pbuf_p pbuf) {
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
    size_t len;
    len = esp_pbuf_length(pbuf, 1);             /* Get length of pbuf */
    if (conn->tcp_not_ack_bytes >= len) {       /* Check length of not-acknowledged bytes */
        conn->tcp_not_ack_bytes -= len;
    } else {
        /* Warning here, de-sync happened somewhere! */
    }
    espi_conn_manual_tcp_try_read_data(conn);   /* Try to read more connection data */
#else /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
    ESP_UNUSED(conn);
    ESP_UNUSED(pbuf);
#endif /* !ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
    return espOK;
}

/**
 * \brief           Set argument variable for connection
 * \param[in]       conn: Connection handle to set argument
 * \param[in]       arg: Pointer to argument
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 * \sa              esp_conn_get_arg
 */
espr_t
esp_conn_set_arg(esp_conn_p conn, void* const arg) {
    esp_core_lock();
    conn->arg = arg;                            /* Set argument for connection */
    esp_core_unlock();
    return espOK;
}

/**
 * \brief           Get user defined connection argument
 * \param[in]       conn: Connection handle to get argument
 * \return          User argument
 * \sa              esp_conn_set_arg
 */
void *
esp_conn_get_arg(esp_conn_p conn) {
    void* arg;
    esp_core_lock();
    arg = conn->arg;                            /* Set argument for connection */
    esp_core_unlock();
    return arg;
}

/**
 * \brief           Gets connections status
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_get_conns_status(const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSTATUS;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Check if connection type is client
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_conn_is_client(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        esp_core_lock();
        res = conn->status.f.active && conn->status.f.client;
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Check if connection type is server
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_conn_is_server(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        esp_core_lock();
        res = conn->status.f.active && !conn->status.f.client;
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Check if connection is active
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_conn_is_active(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        esp_core_lock();
        res = conn->status.f.active;
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Check if connection is closed
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_conn_is_closed(esp_conn_p conn) {
    uint8_t res = 0;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        esp_core_lock();
        res = !conn->status.f.active;
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Get the number from connection
 * \param[in]       conn: Connection pointer
 * \return          Connection number in case of success or -1 on failure
 */
int8_t
esp_conn_getnum(esp_conn_p conn) {
    int8_t res = -1;
    if (conn != NULL && espi_is_valid_conn_ptr(conn)) {
        /* Protection not needed as every connection has always the same number */
        res = conn->num;                        /* Get number */
    }
    return res;
}

/**
 * \brief           Set internal buffer size for SSL connection on ESP device
 * \note            Use this function before you start first SSL connection
 * \param[in]       size: Size of buffer in units of bytes. Valid range is between 2048 and 4096 bytes
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_set_ssl_buffersize(size_t size, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_TCPIP_CIPSSLSIZE;
    ESP_MSG_VAR_REF(msg).msg.tcpip_sslsize.size = size;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Get connection from connection based event
 * \param[in]       evt: Event which happened for connection
 * \return          Connection pointer on success, `NULL` otherwise
 */
esp_conn_p
esp_conn_get_from_evt(esp_evt_t* evt) {
    switch (evt->type) {
        case ESP_EVT_CONN_ACTIVE: return esp_evt_conn_active_get_conn(evt);
        case ESP_EVT_CONN_CLOSE: return esp_evt_conn_close_get_conn(evt);
        case ESP_EVT_CONN_RECV: return esp_evt_conn_recv_get_conn(evt);
        case ESP_EVT_CONN_SEND: return esp_evt_conn_send_get_conn(evt);
        case ESP_EVT_CONN_POLL: return esp_evt_conn_poll_get_conn(evt);
        default: return NULL;
    }
}

/**
 * \brief           Write data to connection buffer and if it is full, send it non-blocking way
 * \note            This function may only be called from core (connection callbacks)
 * \param[in]       conn: Connection to write
 * \param[in]       data: Data to copy to write buffer
 * \param[in]       btw: Number of bytes to write
 * \param[in]       flush: Flush flag. Set to `1` if you want to send data immediatelly after copying
 * \param[out]      mem_available: Available memory size available in current write buffer.
 *                  When the buffer length is reached, current one is sent and a new one is automatically created.
 *                  If function returns \ref espOK and `*mem_available = 0`, there was a problem
 *                  allocating a new buffer for next operation
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_conn_write(esp_conn_p conn, const void* data, size_t btw, uint8_t flush,
                size_t* const mem_available) {
    size_t len;

    const uint8_t* d = data;

    ESP_ASSERT("conn != NULL", conn != NULL);

    /*
     * Steps during write process:
     *
     * 1. Check if we have buffer already allocated and
     *      write data to the tail of buffer
     *   1.1. In case buffer is full, send it non-blocking,
     *      and enable freeing after it is sent
     * 2. Check how many bytes we can copy as single buffer directly and send
     * 3. Create last buffer and copy remaining data to it even if no remaining data
     *      This is useful when calling function with no parameters (len = 0)
     * 4. Flush (send) current buffer if necessary
     */

    /* Step 1 */
    if (conn->buff.buff != NULL) {
        len = ESP_MIN(conn->buff.len - conn->buff.ptr, btw);
        ESP_MEMCPY(&conn->buff.buff[conn->buff.ptr], d, len);

        d += len;
        btw -= len;
        conn->buff.ptr += len;

        /* Step 1.1 */
        if (conn->buff.ptr == conn->buff.len || flush) {
            /* Try to send to processing queue in non-blocking way */
            if (conn_send(conn, NULL, 0, conn->buff.buff, conn->buff.ptr, NULL, 1, 0) != espOK) {
                ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
                    "[CONN] Free write buffer: %p\r\n", conn->buff.buff);
                esp_mem_free_s((void **)&conn->buff.buff);
            }
            conn->buff.buff = NULL;
        }
    }

    /* Step 2 */
    while (btw >= ESP_CFG_CONN_MAX_DATA_LEN) {
        uint8_t* buff;
        buff = esp_mem_malloc(sizeof(*buff) * ESP_CFG_CONN_MAX_DATA_LEN);
        if (buff != NULL) {
            ESP_MEMCPY(buff, d, ESP_CFG_CONN_MAX_DATA_LEN); /* Copy data to buffer */
            if (conn_send(conn, NULL, 0, buff, ESP_CFG_CONN_MAX_DATA_LEN, NULL, 1, 0) != espOK) {
                ESP_DEBUGF(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE,
                    "[CONN] Free write buffer: %p\r\n", (void *)buff);
                esp_mem_free_s((void **)&buff);
                return espERRMEM;
            }
        } else {
            return espERRMEM;
        }

        btw -= ESP_CFG_CONN_MAX_DATA_LEN;       /* Decrease remaining length */
        d += ESP_CFG_CONN_MAX_DATA_LEN;         /* Advance data pointer */
    }

    /* Step 3 */
    if (conn->buff.buff == NULL) {
        conn->buff.buff = esp_mem_malloc(sizeof(*conn->buff.buff) * ESP_CFG_CONN_MAX_DATA_LEN);
        conn->buff.len = ESP_CFG_CONN_MAX_DATA_LEN;
        conn->buff.ptr = 0;

        ESP_DEBUGW(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE, conn->buff.buff != NULL,
            "[CONN] New write buffer allocated, addr = %p\r\n", conn->buff.buff);
        ESP_DEBUGW(ESP_CFG_DBG_CONN | ESP_DBG_TYPE_TRACE, conn->buff.buff == NULL,
            "[CONN] Cannot allocate new write buffer\r\n");
    }
    if (btw > 0) {
        if (conn->buff.buff != NULL) {
            ESP_MEMCPY(conn->buff.buff, d, btw);    /* Copy data to memory */
            conn->buff.ptr = btw;
        } else {
            return espERRMEM;
        }
    }

    /* Step 4 */
    if (flush && conn->buff.buff != NULL) {
        flush_buff(conn);
    }

    /* Calculate number of available memory after write operation */
    if (mem_available != NULL) {
        if (conn->buff.buff != NULL) {
            *mem_available = conn->buff.len - conn->buff.ptr;
        } else {
            *mem_available = 0;
        }
    }
    return espOK;
}

/**
 * \brief           Get total number of bytes ever received on connection and sent to user
 * \param[in]       conn: Connection handle
 * \return          Total number of received bytes on connection
 */
size_t
esp_conn_get_total_recved_count(esp_conn_p conn) {
    size_t tot = 0;

    if (conn != NULL) {
        esp_core_lock();
        tot = conn->total_recved;               /* Get total received bytes */
        esp_core_unlock();
    }
    return tot;
}

/**
 * \brief           Get connection remote IP address
 * \param[in]       conn: Connection handle
 * \param[out]      ip: Pointer to IP output handle
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_conn_get_remote_ip(esp_conn_p conn, esp_ip_t* ip) {
    if (conn != NULL && ip != NULL) {
        esp_core_lock();
        ESP_MEMCPY(ip, &conn->remote_ip, sizeof(*ip));  /* Copy data */
        esp_core_unlock();
        return 1;
    }
    return 0;
}

/**
 * \brief           Get connection remote port number
 * \param[in]       conn: Connection handle
 * \return          Port number on success, `0` otherwise
 */
esp_port_t
esp_conn_get_remote_port(esp_conn_p conn) {
    esp_port_t port = 0;
    if (conn != NULL) {
        esp_core_lock();
        port = conn->remote_port;
        esp_core_unlock();
    }
    return port;
}

/**
 * \brief           Get connection local port number
 * \param[in]       conn: Connection handle
 * \return          Port number on success, `0` otherwise
 */
esp_port_t
esp_conn_get_local_port(esp_conn_p conn) {
    esp_port_t port = 0;
    if (conn != NULL) {
        esp_core_lock();
        port = conn->local_port;
        esp_core_unlock();
    }
    return port;
}
