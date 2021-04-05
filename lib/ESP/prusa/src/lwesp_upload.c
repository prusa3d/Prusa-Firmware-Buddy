#include "lwesp_upload.h"
#include "dbg.h"

#define BUART_SEND_FLUSH()        esp.ll.send_fn(NULL, 0)
#define BUART_SEND_STR(str)       esp.ll.send_fn((const void *)(str), (size_t)strlen(str))
#define BUART_SEND_CONST_STR(str) esp.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
#define BUART_SEND_BEGIN_SLIP()       \
    do {                              \
        BUART_SEND_CONST_STR("0xC0"); \
    } while (0)
#define BUART_SEND_END_SLIP()         \
    do {                              \
        BUART_SEND_CONST_STR("0xC0"); \
        BUART_SEND_FLUSH();           \
    } while (0)

void esp_upload_start() {
}

// custom fn to handle cmds for ESP UART ROM bootloader
lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg) {

    switch (CMD_GET_CUR()) {
    case LWESP_CMD_TCPIP_CIPSTATUS: {
        _dbg0("START SYNC ESP ROM BOOTLOADER");
        // AT_PORT_SEND_BEGIN_AT();
        // AT_PORT_SEND_CONST_STR("+RST");
        // AT_PORT_SEND_END_AT();
        break;
    }
    }

    return lwespOK;
}
