#include "lwesp/lwesp_private.h"
#include "lwesp/lwesp_conn.h"
#include "lwesp/lwesp_mem.h"
#include "lwesp/lwesp_timeout.h"
#include "lwesp_upload.h"

/* // # Commands supported by ESP8266 ROM bootloader */
// #define ESP_FLASH_BEGIN = 0x02
// #define ESP_FLASH_DATA  = 0x03
// #define ESP_FLASH_END   = 0x04
// #define ESP_MEM_BEGIN   = 0x05
// #define ESP_MEM_END     = 0x06
// #define ESP_MEM_DATA    = 0x07
// #define ESP_SYNC        = 0x08
// #define ESP_WRITE_REG   = 0x09
/* #define ESP_READ_REG    = 0x0a */

// # Some comands supported by ESP32 ROM bootloader (or -8266 w/ stub)
#define ESP_SPI_SET_PARAMS   = 0x0B
#define ESP_SPI_ATTACH       = 0x0D
#define ESP_READ_FLASH_SLOW  = 0x0e // #ROM only, much slower than the stub flash read
#define ESP_CHANGE_BAUDRATE  = 0x0F
#define ESP_FLASH_DEFL_BEGIN = 0x10
#define ESP_FLASH_DEFL_DATA  = 0x11
#define ESP_FLASH_DEFL_END   = 0x12
#define ESP_SPI_FLASH_MD5    = 0x13

// # Commands supported by ESP32-S2/S3/C3 ROM bootloader only
#define ESP_GET_SECURITY_INFO = 0x14

// # Some commands supported by stub only
#define ESP_ERASE_FLASH   = 0xD0
#define ESP_ERASE_REGION  = 0xD1
#define ESP_READ_FLASH    = 0xD2
#define ESP_RUN_USER_CODE = 0xD3

// # Flash encryption encrypted data command
#define ESP_FLASH_ENCRYPT_DATA = 0xD4

// # Response code(s) sent by ROM
#define ROM_INVALID_RECV_MSG = 0x05 // #response if an invalid message is received

// # Maximum block sized for RAM and Flash writes, respectively.
#define ESP_RAM_BLOCK = 0x1800

#define FLASH_WRITE_SIZE = 0x400

// step#1 - start to sync with ESP-01 module 
lwespr_t lwesp_conn_upload_start(lwesp_conn_p *conn, void *const arg,
    lwesp_evt_fn conn_evt_fn, const uint32_t blocking) {

    LWESP_MSG_VAR_DEFINE(msg);
    LWESP_ASSERT("conn_evt_fn != NULL", conn_evt_fn != NULL);
    LWESP_MSG_VAR_ALLOC(msg, blocking);


    // we have to use AT cmds as suplement for UART bootloader's cmds
    // custom cmds will be handle in our own cb fn - "lwespi_upload_cmd"
    // see custom cmds in "lwesp_upload.h"
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIPSTATUS;
    LWESP_MSG_VAR_REF(msg).cmd = LWESP_CMD_TCPIP_CIPSTATUS;
    LWESP_MSG_VAR_REF(msg).msg.conn_start.num = LWESP_CFG_MAX_CONNS; /* Set maximal value as invalid number */
    LWESP_MSG_VAR_REF(msg).msg.conn_start.conn = conn;
    LWESP_MSG_VAR_REF(msg).msg.conn_start.evt_func = conn_evt_fn;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000);
}



/**
 * \brief           Timeout callback for connection
 * \param[in]       arg: Timeout callback custom argument
 */
// static void
// conn_timeout_cb(void *arg) {
//     lwesp_conn_p conn = arg; [> Argument is actual connection <]
//
//     if (conn->status.f.active) {            [> Handle only active connections <]
//         esp.evt.type = LWESP_EVT_CONN_POLL; [> Poll connection event <]
//         esp.evt.evt.conn_poll.conn = conn;  [> Set connection pointer <]
//         lwespi_send_conn_cb(conn, NULL);    [> Send connection callback <]
//
//         lwespi_conn_start_timeout(conn); [> Schedule new timeout <]
//         LWESP_DEBUGF(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE,
//             "[CONN] Poll event: %p\r\n", conn);
//     }
// }
//
/**
 * \brief           Start timeout function for connection
 * \param[in]       conn: Connection handle as user argument
 */
// void lwespi_conn_start_timeout(lwesp_conn_p conn) {
//     lwesp_timeout_add(LWESP_CFG_CONN_POLL_INTERVAL, conn_timeout_cb, conn); [> Add connection timeout <]
// }
//
/**
 * \brief           Callback function when manual TCP receive finishes
 * \param[in]       res: Result of reading
 * \param[in]       arg: Custom user argument
 */
// static void
// manual_tcp_read_data_evt_fn(lwespr_t res, void *arg) {
//     lwesp_conn_p conn = arg;
//
//     // conn->status.f.receive_is_command_queued = 0;
//     // lwespi_conn_manual_tcp_try_read_data(conn);
// }
//
/**
 * \brief           Manually start data read operation with desired length on specific connection
 * \param[in]       conn: Connection handle
 * \param[in]       len: Number of bytes to read
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwespi_conn_manual_tcp_try_read_data(lwesp_conn_p conn) {
//     uint32_t blocking = 0;
//     lwespr_t res = lwespOK;
//     LWESP_MSG_VAR_DEFINE(msg);
//
//     LWESP_ASSERT("conn != NULL", conn != NULL);
//
//     // [> Receive must not be blocked and other command must not be in queue to read data <]
//     // if (conn->status.f.receive_blocked
//     //     || conn->status.f.receive_is_command_queued) {
//     //     return lwespINPROG;
//     // }
//     //
//     // [> Any available data to process? <]
//     // if (conn->tcp_available_bytes == 0
//     //     || !conn->status.f.active) {
//     //     return lwespERR;
//     // }
//     //
//     // LWESP_MSG_VAR_ALLOC(msg, blocking);                            [> Allocate first, will return on failure <]
//     // LWESP_MSG_VAR_SET_EVT(msg, manual_tcp_read_data_evt_fn, conn); [> Set event callback function <]
//     // LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIPRECVDATA;
//     // LWESP_MSG_VAR_REF(msg).cmd = LWESP_CMD_TCPIP_CIPRECVLEN;
//     // LWESP_MSG_VAR_REF(msg).msg.ciprecvdata.len = 0;     [> Filled after RECVLEN received <]
//     // LWESP_MSG_VAR_REF(msg).msg.ciprecvdata.buff = NULL; [> Filled after RECVLEN received <]
//     // LWESP_MSG_VAR_REF(msg).msg.ciprecvdata.conn = conn;
//     //
//     // [> Try to start command <]
//     // if ((res = lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000)) == lwespOK) {
//     //     conn->status.f.receive_is_command_queued = 1; [> Command queued <]
//     [> } <]
//     return res;
// }
//
/**
 * \brief           Callback function for checking receive length in manual TCP receive buffer
 * \param[in]       res: Result of reading
 * \param[in]       arg: Custom user argument
 */
// static void
// check_available_rx_data_evt_fn(lwespr_t res, void *arg) {
//     [> Try to read data if possible <]
//     for (size_t i = 0; i < LWESP_CFG_MAX_CONNS; ++i) {
//         lwespi_conn_manual_tcp_try_read_data(&esp.m.conns[i]);
//     }
// }
//
/**
 * \brief           Manually check for received buffer status for connections
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
// lwespr_t
// lwespi_conn_check_available_rx_data(void) {
//     LWESP_MSG_VAR_DEFINE(msg);
//
//     LWESP_MSG_VAR_ALLOC(msg, 0);                                      [> Allocate first, will return on failure <]
//     LWESP_MSG_VAR_SET_EVT(msg, check_available_rx_data_evt_fn, NULL); [> Set event callback function <]
//
//     return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 1000);
// }
//
/**
 * \brief           Get connection validation ID
 * \param[in]       conn: Connection handle
 * \return          Connection current validation ID
 */
// uint8_t
// lwespi_conn_get_val_id(lwesp_conn_p conn) {
//     uint8_t val_id;
//     lwesp_core_lock();
//     val_id = conn->val_id;
//     lwesp_core_unlock();
//
//     return val_id;
// }
//
/**
 * \brief           Send data on already active connection of type UDP to specific remote IP and port
 * \note            In case IP and port values are not set, it will behave as normal send function (suitable for TCP too)
 * \param[in]       conn: Pointer to connection to send data
 * \param[in]       data: Pointer to data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent
 * \param[in]       fau: "Free After Use" flag. Set to `1` if stack should free the memory after data sent
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// static lwespr_t
// conn_send(lwesp_conn_p conn, const void *data, size_t btw, size_t *const bw, uint8_t fau, const uint32_t blocking) {
//     LWESP_MSG_VAR_DEFINE(msg);
//
//     LWESP_ASSERT("conn != NULL", conn != NULL);
//     LWESP_ASSERT("data != NULL", data != NULL);
//     LWESP_ASSERT("btw > 0", btw > 0);
//
//     if (bw != NULL) {
//         *bw = 0;
//     }
//
//     CONN_CHECK_CLOSED_IN_CLOSING(conn); [> Check if we can continue <]
//
//     LWESP_MSG_VAR_ALLOC(msg, blocking);
//
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.data = data;
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.btw = btw;
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.bw = bw;
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.fau = fau;
//     LWESP_MSG_VAR_REF(msg).msg.conn_send.val_id = lwespi_conn_get_val_id(conn);
//
//     return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000);
// }
//
/**
 * \brief           Flush buffer on connection
 * \param[in]       conn: Connection to flush buffer on
 * \return          \ref lwespOK if data flushed and put to queue, member of \ref lwespr_t otherwise
 */
// static lwespr_t
// flush_buff(lwesp_conn_p conn) {
//     lwespr_t res = lwespOK;
//     lwesp_core_lock();
//     if (conn != NULL && conn->buff.buff != NULL) { [> Do we have something ready? <]
/*
         * If there is nothing to write or if write was not successful,
         * simply free the memory and stop execution
         */
//         if (conn->buff.ptr > 0) { [> Anything to send at the moment? <]
//             res = conn_send(conn, NULL, 0, conn->buff.buff, conn->buff.ptr, NULL, 1, 0);
//         } else {
//             res = lwespERR;
//         }
//         if (res != lwespOK) {
//             LWESP_DEBUGF(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE,
//                 "[CONN] Free write buffer: %p\r\n", (void *)conn->buff.buff);
//             lwesp_mem_free_s((void **)&conn->buff.buff);
//         }
//         conn->buff.buff = NULL;
//     }
//     lwesp_core_unlock();
//     return res;
// }
//
/**
 * \brief           Initialize connection module
 */
// void lwespi_conn_init(void) {
// }
//
/**
 * \brief           Start a new connection of specific type
 * \param[out]      conn: Pointer to connection handle to set new connection reference in case of successfully connected
 * \param[in]       type: Connection type. This parameter can be a value of \ref lwesp_conn_type_t enumeration
 * \param[in]       remote_host: Connection host. In case of IP, write it as string, ex. "192.168.1.1"
 * \param[in]       remote_port: Connection port
 * \param[in]       arg: Pointer to user argument passed to connection if successfully connected
 * \param[in]       conn_evt_fn: Callback function for this connection
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_conn_upload_start(lwesp_conn_p *conn, lwesp_conn_type_t type, const char *const remote_host, lwesp_port_t remote_port,
//     void *const arg, lwesp_evt_fn conn_evt_fn, const uint32_t blocking) {
//     LWESP_MSG_VAR_DEFINE(msg);
//
//     LWESP_ASSERT("conn_evt_fn != NULL", conn_evt_fn != NULL);
//
//     LWESP_MSG_VAR_ALLOC(msg, blocking);
//
//     return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000);
// }
//
/**
 * \brief           Close specific or all connections
 * \param[in]       conn: Connection handle to close. Set to NULL if you want to close all connections.
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_conn_upload_close(lwesp_conn_p conn, const uint32_t blocking) {
//     lwespr_t res;
//     res = lwespOK;
//     [>     LWESP_MSG_VAR_DEFINE(msg); <]
//     //
//     // LWESP_ASSERT("conn != NULL", conn != NULL);
//     //
//     // CONN_CHECK_CLOSED_IN_CLOSING(conn); [> Check if we can continue <]
//     //
//     // [> Proceed with close event at this point! <]
//     // LWESP_MSG_VAR_ALLOC(msg, blocking);
//     // LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIPCLOSE;
//     // LWESP_MSG_VAR_REF(msg).msg.conn_close.conn = conn;
//     // LWESP_MSG_VAR_REF(msg).msg.conn_close.val_id = lwespi_conn_get_val_id(conn);
//     //
//     // flush_buff(conn); [> First flush buffer <]
//     // res = lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 1000);
//     // if (res == lwespOK && !blocking) { [> Function succedded in non-blocking mode <]
//     //     lwesp_core_lock();
//     //     LWESP_DEBUGF(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE,
//     //         "[CONN] Connection %d set to closing state\r\n", (int)conn->num);
//     //     conn->status.f.in_closing = 1; [> Connection is in closing mode but not yet closed <]
//     //     lwesp_core_unlock();
//     [> } <]
//     return res;
// }
//
/**
 * \brief           Send data on already active connection either as client or server
 * \param[in]       conn: Connection handle to send data
 * \param[in]       data: Data to send
 * \param[in]       btw: Number of bytes to send
 * \param[out]      bw: Pointer to output variable to save number of sent data when successfully sent.
 *                      Parameter value might not be accurate if you combine \ref lwesp_conn_write and \ref lwesp_conn_send functions
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_conn_upload_send(lwesp_conn_p conn, const void *data, size_t btw, size_t *const bw,
//     const uint32_t blocking) {
//     lwespr_t res;
//     const uint8_t *d = data;
//
//     LWESP_ASSERT("conn != NULL", conn != NULL);
//     LWESP_ASSERT("data != NULL", data != NULL);
//     LWESP_ASSERT("btw > 0", btw > 0);
//
//     lwesp_core_lock();
//     if (conn->buff.buff != NULL) { [> Check if memory available <]
//         size_t to_copy;
//         to_copy = LWESP_MIN(btw, conn->buff.len - conn->buff.ptr);
//         if (to_copy > 0) {
//             LWESP_MEMCPY(&conn->buff.buff[conn->buff.ptr], d, to_copy);
//             conn->buff.ptr += to_copy;
//             d += to_copy;
//             btw -= to_copy;
//         }
//     }
//     lwesp_core_unlock();
//     res = flush_buff(conn); [> Flush currently written memory if exists <]
//     if (btw > 0) {          [> Check for remaining data <]
//         res = conn_send(conn, NULL, 0, d, btw, bw, 0, blocking);
//     }
//     return res;
// }
//
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
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_conn_upload_recved(lwesp_conn_p conn, lwesp_pbuf_p pbuf) {
//     [> #if LWESP_CFG_CONN_MANUAL_TCP_RECEIVE <]
//     //     size_t len;
//     //     len = lwesp_pbuf_length(pbuf, 1);     [> Get length of pbuf <]
//     //     if (conn->tcp_not_ack_bytes >= len) { [> Check length of not-acknowledged bytes <]
//     //         conn->tcp_not_ack_bytes -= len;
//     //     } else {
//     //         [> Warning here, de-sync happened somewhere! <]
//     //     }
//     //     lwespi_conn_manual_tcp_try_read_data(conn); [> Try to read more connection data <]
//     // #else                                           [> LWESP_CFG_CONN_MANUAL_TCP_RECEIVE <]
//     //     LWESP_UNUSED(conn);
//     //     LWESP_UNUSED(pbuf);
//     [> #endif                                          [> !LWESP_CFG_CONN_MANUAL_TCP_RECEIVE <] <]
//     return lwespOK;
// }
//
/**
 * \brief           Set argument variable for connection
 * \param[in]       conn: Connection handle to set argument
 * \param[in]       arg: Pointer to argument
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 * \sa              lwesp_conn_get_arg
 */
// lwespr_t
// lwesp_conn_upload_set_arg(lwesp_conn_p conn, void *const arg) {
//     lwesp_core_lock();
//     conn->arg = arg; [> Set argument for connection <]
//     lwesp_core_unlock();
//     return lwespOK;
// }
//
/**
 * \brief           Get user defined connection argument
 * \param[in]       conn: Connection handle to get argument
 * \return          User argument
 * \sa              lwesp_conn_set_arg
 */
// void *
// lwesp_conn_upload_get_arg(lwesp_conn_p conn) {
//     void *arg;
//     lwesp_core_lock();
//     arg = conn->arg; [> Set argument for connection <]
//     lwesp_core_unlock();
//     return arg;
// }
//
/**
 * \brief           Gets connections status
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_get_conns_status(const uint32_t blocking) {
//     return lwespOK;
//     // LWESP_MSG_VAR_DEFINE(msg);
//     //
//     // LWESP_MSG_VAR_ALLOC(msg, blocking);
//     // LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIPSTATUS;
//     //
//     // return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 1000);
// }
//
/**
 * \brief           Check if connection type is client
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
// uint8_t
// lwesp_conn_upload_is_client(lwesp_conn_p conn) {
//     uint8_t res = 0;
//     if (conn != NULL && lwespi_is_valid_conn_ptr(conn)) {
//         lwesp_core_lock();
//         res = conn->status.f.active && conn->status.f.client;
//         lwesp_core_unlock();
//     }
//     return res;
// }
//
/**
 * \brief           Check if connection type is server
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
// uint8_t
// lwesp_conn_upload_is_server(lwesp_conn_p conn) {
//     uint8_t res = 0;
//     if (conn != NULL && lwespi_is_valid_conn_ptr(conn)) {
//         lwesp_core_lock();
//         res = conn->status.f.active && !conn->status.f.client;
//         lwesp_core_unlock();
//     }
//     return res;
// }
//
/**
 * \brief           Check if connection is active
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
// uint8_t
// lwesp_conn_upload_is_active(lwesp_conn_p conn) {
//     uint8_t res = 0;
//     if (conn != NULL && lwespi_is_valid_conn_ptr(conn)) {
//         lwesp_core_lock();
//         res = conn->status.f.active;
//         lwesp_core_unlock();
//     }
//     return res;
// }
//
/**
 * \brief           Check if connection is closed
 * \param[in]       conn: Pointer to connection to check for status
 * \return          `1` on success, `0` otherwise
 */
// uint8_t
// lwesp_conn_upload_is_closed(lwesp_conn_p conn) {
//     uint8_t res = 0;
//     if (conn != NULL && lwespi_is_valid_conn_ptr(conn)) {
//         lwesp_core_lock();
//         res = !conn->status.f.active;
//         lwesp_core_unlock();
//     }
//     return res;
// }
//
/**
 * \brief           Get the number from connection
 * \param[in]       conn: Connection pointer
 * \return          Connection number in case of success or -1 on failure
 */
// int8_t
// lwesp_conn_upload_getnum(lwesp_conn_p conn) {
//     int8_t res = -1;
//     if (conn != NULL && lwespi_is_valid_conn_ptr(conn)) {
//         [> Protection not needed as every connection has always the same number <]
//         res = conn->num; [> Get number <]
//     }
//     return res;
// }
//
/**
 * \brief           Get connection from connection based event
 * \param[in]       evt: Event which happened for connection
 * \return          Connection pointer on success, `NULL` otherwise
 */
// lwesp_conn_p
// lwesp_conn_upload_get_from_evt(lwesp_evt_t *evt) {
//     switch (evt->type) {
//     case LWESP_EVT_CONN_ACTIVE:
//         return lwesp_evt_conn_active_get_conn(evt);
//     case LWESP_EVT_CONN_CLOSE:
//         return lwesp_evt_conn_close_get_conn(evt);
//     case LWESP_EVT_CONN_RECV:
//         return lwesp_evt_conn_recv_get_conn(evt);
//     case LWESP_EVT_CONN_SEND:
//         return lwesp_evt_conn_send_get_conn(evt);
//     case LWESP_EVT_CONN_POLL:
//         return lwesp_evt_conn_poll_get_conn(evt);
//     default:
//         return NULL;
//     }
// }
//
/**
 * \brief           Write data to connection buffer and if it is full, send it non-blocking way
 * \note            This function may only be called from core (connection callbacks)
 * \param[in]       conn: Connection to write
 * \param[in]       data: Data to copy to write buffer
 * \param[in]       btw: Number of bytes to write
 * \param[in]       flush: Flush flag. Set to `1` if you want to send data immediately after copying
 * \param[out]      mem_available: Available memory size available in current write buffer.
 *                  When the buffer length is reached, current one is sent and a new one is automatically created.
 *                  If function returns \ref lwespOK and `*mem_available = 0`, there was a problem
 *                  allocating a new buffer for next operation
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
// lwespr_t
// lwesp_conn_upload_write(lwesp_conn_p conn, const void *data, size_t btw, uint8_t flush,
//     size_t *const mem_available) {
//     size_t len;
//
//     const uint8_t *d = data;
//
//     LWESP_ASSERT("conn != NULL", conn != NULL);
//
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
//
//     [> Step 1 <]
//     if (conn->buff.buff != NULL) {
//         len = LWESP_MIN(conn->buff.len - conn->buff.ptr, btw);
//         LWESP_MEMCPY(&conn->buff.buff[conn->buff.ptr], d, len);
//
//         d += len;
//         btw -= len;
//         conn->buff.ptr += len;
//
//         [> Step 1.1 <]
//         if (conn->buff.ptr == conn->buff.len || flush) {
//             [> Try to send to processing queue in non-blocking way <]
//             if (conn_send(conn, NULL, 0, conn->buff.buff, conn->buff.ptr, NULL, 1, 0) != lwespOK) {
//                 LWESP_DEBUGF(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE,
//                     "[CONN] Free write buffer: %p\r\n", conn->buff.buff);
//                 lwesp_mem_free_s((void **)&conn->buff.buff);
//             }
//             conn->buff.buff = NULL;
//         }
//     }
//
//     [> Step 2 <]
//     while (btw >= LWESP_CFG_CONN_MAX_DATA_LEN) {
//         uint8_t *buff;
//         buff = lwesp_mem_malloc(sizeof(*buff) * LWESP_CFG_CONN_MAX_DATA_LEN);
//         if (buff != NULL) {
//             LWESP_MEMCPY(buff, d, LWESP_CFG_CONN_MAX_DATA_LEN); [> Copy data to buffer <]
//             if (conn_send(conn, NULL, 0, buff, LWESP_CFG_CONN_MAX_DATA_LEN, NULL, 1, 0) != lwespOK) {
//                 LWESP_DEBUGF(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE,
//                     "[CONN] Free write buffer: %p\r\n", (void *)buff);
//                 lwesp_mem_free_s((void **)&buff);
//                 return lwespERRMEM;
//             }
//         } else {
//             return lwespERRMEM;
//         }
//
//         btw -= LWESP_CFG_CONN_MAX_DATA_LEN; [> Decrease remaining length <]
//         d += LWESP_CFG_CONN_MAX_DATA_LEN;   [> Advance data pointer <]
/* } */
//
// [> Step 3 <]
// if (conn->buff.buff == NULL) {
//     conn->buff.buff = lwesp_mem_malloc(sizeof(*conn->buff.buff) * LWESP_CFG_CONN_MAX_DATA_LEN);
//     conn->buff.len = LWESP_CFG_CONN_MAX_DATA_LEN;
//     conn->buff.ptr = 0;
//
//     LWESP_DEBUGW(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE, conn->buff.buff != NULL,
//         "[CONN] New write buffer allocated, addr = %p\r\n", conn->buff.buff);
//     LWESP_DEBUGW(LWESP_CFG_DBG_CONN | LWESP_DBG_TYPE_TRACE, conn->buff.buff == NULL,
//         "[CONN] Cannot allocate new write buffer\r\n");
// }
// if (btw > 0) {
//     if (conn->buff.buff != NULL) {
//         LWESP_MEMCPY(conn->buff.buff, d, btw); [> Copy data to memory <]
//         conn->buff.ptr = btw;
//     } else {
//         return lwespERRMEM;
//     }
// }
//
// [> Step 4 <]
// if (flush && conn->buff.buff != NULL) {
//     flush_buff(conn);
// }
//
// [> Calculate number of available memory after write operation <]
// if (mem_available != NULL) {
//     if (conn->buff.buff != NULL) {
//         *mem_available = conn->buff.len - conn->buff.ptr;
//     } else {
//         *mem_available = 0;
//     }
// }
// return lwespOK;
// }
//
/**
 * \brief           Get total number of bytes ever received on connection and sent to user
 * \param[in]       conn: Connection handle
 * \return          Total number of received bytes on connection
 */
// size_t
// lwesp_conn_upload_get_total_recved_count(lwesp_conn_p conn) {
//     size_t tot = 0;
//
//     if (conn != NULL) {
//         lwesp_core_lock();
//         tot = conn->total_recved; [> Get total received bytes <]
//         lwesp_core_unlock();
//     }
//     return tot;
// }
//
/**
 * \brief           Get connection remote IP address
 * \param[in]       conn: Connection handle
 * \param[out]      ip: Pointer to IP output handle
 * \return          `1` on success, `0` otherwise
 */
// uint8_t
// lwesp_conn_upload_get_remote_ip(lwesp_conn_p conn, lwesp_ip_t *ip) {
//     if (conn != NULL && ip != NULL) {
//         lwesp_core_lock();
//         LWESP_MEMCPY(ip, &conn->remote_ip, sizeof(*ip)); [> Copy data <]
//         lwesp_core_unlock();
//         return 1;
//     }
//     return 0;
// }
//
/**
 * \brief           Get connection remote port number
 * \param[in]       conn: Connection handle
 * \return          Port number on success, `0` otherwise
 */
// lwesp_port_t
// lwesp_conn_upload_get_remote_port(lwesp_conn_p conn) {
//     lwesp_port_t port = 0;
//     if (conn != NULL) {
//         lwesp_core_lock();
//         port = conn->remote_port;
//         lwesp_core_unlock();
//     }
//     return port;
// }
//
/**
 * \brief           Get connection local port number
 * \param[in]       conn: Connection handle
 * \return          Port number on success, `0` otherwise
 */
// lwesp_port_t
// lwesp_conn_upload_get_local_port(lwesp_conn_p conn) {
//     lwesp_port_t port = 0;
//     if (conn != NULL) {
//         lwesp_core_lock();
//         port = conn->local_port;
//         lwesp_core_unlock();
//     }
//     return port;
/* } */
