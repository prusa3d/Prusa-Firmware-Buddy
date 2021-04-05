#include "lwesp_upload.h"
#include "dbg.h"

#define BUART_SEND_FLUSH()        esp.ll.send_fn(NULL, 0)
#define BUART_SEND_CHR(str)       esp.ll.send_fn((const void *)(str), (size_t)1)
#define BUART_SEND_STR(str)       esp.ll.send_fn((const void *)(str), (size_t)strlen(str))
#define BUART_SEND_CONST_STR(str) esp.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
#define BUART_SEND_BEGIN_SLIP()     \
    do {                            \
        unsigned char s_s = "\x0c"; \
        BUART_SEND_CHR(&s_s);       \
    } while (0)
#define BUART_SEND_END_SLIP()       \
    do {                            \
        unsigned char s_s = "\x0c"; \
        BUART_SEND_CHR(&s_s);       \
        BUART_SEND_FLUSH();         \
    } while (0)

void esp_upload_start() {
}

void issue_ROM_command(unsigned char op, unsigned short datalen) {
    // ROM command pack itself into:
    // unsigned char 1B - 0x00
    // unsigned char 1B - op (cmd)
    // unsigned short 2B - datalen
    // unsigned int 4B - just zero 0 (or READ_REG response value)

    // -- first zero
    BUART_SEND_CHR("\x00");
    // -- command
    BUART_SEND_CHR(&op);
    // -- data length
    char dl[2];
    dl[0] = datalen & 0xff;
    dl[1] = (datalen >> 8) & 0xff;
    BUART_SEND_CONST_STR(&dl);
    // zeroes
    unsigned int zero = 0;
    unsigned char z[4];
    z[0] = (zero >> 24) & 0xff;
    z[1] = (zero >> 16) & 0xff;
    z[2] = (zero >> 8) & 0xff;
    z[3] = zero & 0xff;
    BUART_SEND_CONST_STR(&z);
}

// custom fn to handle cmds for ESP UART ROM bootloader
// sending SLIP packets to issue a cmd with data
lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg) {

    switch (CMD_GET_CUR()) {

    // SYNC - ESP_SYNC (0x08)
    case LWESP_CMD_TCPIP_CIPSTATUS: {
        _dbg0("START SYNC ESP ROM BOOTLOADER");

        unsigned char cmd[36] = { 0x07, 0x07, 0x12, 0x20, [4 ... 35] = 0x55 };
        _dbg0("sync cmd to send - %s", cmd);

        BUART_SEND_BEGIN_SLIP();
        // -- cmd structure (esptool.py:413)
        issue_ROM_command(ESP_SYNC, sizeof(cmd));

        // -- sync data
        BUART_SEND_STR(cmd);
        BUART_SEND_END_SLIP();

        // BUART_SEND_STR("AT+RST\r\n");
        // AT_PORT_SEND_BEGIN_AT();
        // AT_PORT_SEND_CONST_STR("");
        // AT_PORT_SEND_END_AT();
        break;
    }

    // READ_REG - ESP_READ_REG (0x0a)
    case LWESP_CMD_TCPIP_CIFSR: {
        _dbg0("READ REG ESP ROM BOOTLOADER");
        break;
    }

    // FLASH BEGIN - ESP_FLASH_BEGIN (0x02)
    case LWESP_CMD_TCPIP_CIPSTART: {
        _dbg0("FLASH BEGIN ESP ROM BOOTLOADER");
        break;
    }

    // FLASH DATA - ESP_FLASH_DATA (0x03)
    case LWESP_CMD_TCPIP_CIPSEND: {
        _dbg0("FLASH DATA ESP ROM BOOTLOADER");
        break;
    }
    }

    return lwespOK;
}
