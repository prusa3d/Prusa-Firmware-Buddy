#include "lwesp/lwesp_private.h"
#include "lwesp/lwesp_conn.h"
#include "lwesp/lwesp_mem.h"
#include "lwesp/lwesp_timeout.h"
#include "lwesp_upload.h"

/* // # Some comands supported by ESP32 ROM bootloader (or -8266 w/ stub) */
// #define ESP_SPI_SET_PARAMS   = 0x0B
// #define ESP_SPI_ATTACH       = 0x0D
// #define ESP_READ_FLASH_SLOW  = 0x0e // #ROM only, much slower than the stub flash read
// #define ESP_CHANGE_BAUDRATE  = 0x0F
// #define ESP_FLASH_DEFL_BEGIN = 0x10
// #define ESP_FLASH_DEFL_DATA  = 0x11
// #define ESP_FLASH_DEFL_END   = 0x12
// #define ESP_SPI_FLASH_MD5    = 0x13
//
// // # Commands supported by ESP32-S2/S3/C3 ROM bootloader only
// #define ESP_GET_SECURITY_INFO = 0x14
//
// // # Some commands supported by stub only
// #define ESP_ERASE_FLASH   = 0xD0
// #define ESP_ERASE_REGION  = 0xD1
// #define ESP_READ_FLASH    = 0xD2
// #define ESP_RUN_USER_CODE = 0xD3
//
// // # Flash encryption encrypted data command
// #define ESP_FLASH_ENCRYPT_DATA = 0xD4
//
// // # Response code(s) sent by ROM
// #define ROM_INVALID_RECV_MSG = 0x05 // #response if an invalid message is received
//
// // # Maximum block sized for RAM and Flash writes, respectively.
// #define ESP_RAM_BLOCK = 0x1800
//
/* #define FLASH_WRITE_SIZE = 0x400 */

// step#1 - start to sync with ESP-01 module
lwespr_t lwesp_conn_upload_start(lwesp_conn_p *conn, void *const arg,
    lwesp_evt_fn conn_evt_fn, const uint32_t blocking) {

    LWESP_MSG_VAR_DEFINE(msg);
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

lwespr_t lwesp_conn_upload_read_reg(lwesp_conn_p *conn, void *const arg,
    uint32_t addr, lwesp_evt_fn conn_evt_fn, const uint32_t blocking) {

    LWESP_MSG_VAR_DEFINE(msg);
    LWESP_MSG_VAR_ALLOC(msg, blocking);

    // we have to use AT cmds as suplement for UART bootloader's cmds
    // custom cmds will be handle in our own cb fn - "lwespi_upload_cmd"
    // see custom cmds in "lwesp_upload.h"
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIFSR;
    LWESP_MSG_VAR_REF(msg).cmd = LWESP_CMD_TCPIP_CIFSR;
    LWESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
    // need 32-bit to send size
    LWESP_MSG_VAR_REF(msg).msg.conn_send.sent = addr;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000);
}

lwespr_t lwesp_conn_upload_flash(lwesp_conn_p *conn, void *const arg,
    uint32_t bin_size, uint32_t offset, lwesp_evt_fn conn_evt_fn, const uint32_t blocking) {

    LWESP_MSG_VAR_DEFINE(msg);
    LWESP_MSG_VAR_ALLOC(msg, blocking);

    // we have to use AT cmds as suplement for UART bootloader's cmds
    // custom cmds will be handle in our own cb fn - "lwespi_upload_cmd"
    // see custom cmds in "lwesp_upload.h"
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_TCPIP_CIPSTART;
    LWESP_MSG_VAR_REF(msg).cmd = LWESP_CMD_TCPIP_CIPSTART;
    LWESP_MSG_VAR_REF(msg).msg.conn_send.conn = conn;
    // need 32-bit to send size
    LWESP_MSG_VAR_REF(msg).msg.conn_send.sent = bin_size;
    // need 32-bit to send offset where to write
    LWESP_MSG_VAR_REF(msg).msg.conn_send.btw = offset;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_upload_cmd, 60000);
}

