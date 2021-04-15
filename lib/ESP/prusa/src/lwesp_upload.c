#include "lwesp_upload.h"
#include "dbg.h"

typedef struct {
    char data[128]; /*!< Received characters */
    size_t len;     /*!< Length of valid characters */
} lwesp_recv_t;

uint8_t active_cmd = ESP_SYNC;

/* Receive character macros */
#define RECV_UPLOAD_ADD(ch)                                 \
    do {                                                    \
        if (recv_buff.len < (sizeof(recv_buff.data)) - 1) { \
            recv_buff.data[recv_buff.len++] = ch;           \
            recv_buff.data[recv_buff.len] = 0;              \
        }                                                   \
    } while (0)
#define RECV_UPLOAD_RESET()    \
    do {                       \
        recv_buff.len = 0;     \
        recv_buff.data[0] = 0; \
    } while (0)
#define RECV_UPLOAD_LEN()      ((size_t)recv_buff.len)
#define RECV_UPLOAD_IDX(index) recv_buff.data[index]

/* Sending data over UART */
#define BUART_SEND_FLUSH()        esp.ll.send_fn(NULL, 0)
#define BUART_SEND_CHR(str)       esp.ll.send_fn((const void *)(str), (size_t)1)
#define BUART_SEND_STR(str)       esp.ll.send_fn((const void *)(str), (size_t)strlen(str))
#define BUART_SEND_CONST_STR(str) esp.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
#define BUART_SEND_BEGIN_SLIP() \
    do {                        \
        BUART_SEND_CHR("\x0c"); \
    } while (0)
#define BUART_SEND_END_SLIP()   \
    do {                        \
        BUART_SEND_CHR("\x0c"); \
        BUART_SEND_FLUSH();     \
    } while (0)

void esp_upload_start() {
}

uint32_t parse_int32(const char buf[4]) {
    uint32_t ui0 = buf[0], ui1 = buf[1], ui2 = buf[2], ui3 = buf[3];
    uint32_t val = ui0 | (ui1 << 8) | (ui2 << 16) | (ui3 << 24);
    return val;
}

void rom_cmd_req(unsigned char op, unsigned short datalen) {
    // ROM command pack itself into:
    // https://github.com/espressif/esptool/wiki/Serial-Protocol#command
    // char 1B - 0x00 (0x00 for request 0x01 for response)
    // char 1B - op (cmd)
    // int16_t 2B - datalen
    // int32_t 4B - checksum(for _DATA cmds) or just zero

    // -- first zero - request
    BUART_SEND_CHR("\x00");
    // -- command
    BUART_SEND_CHR(&op);
    // -- data length
    char dl[2];
    dl[0] = datalen & 0xff;
    dl[1] = (datalen >> 8) & 0xff;
    BUART_SEND_CONST_STR(&dl);
}

int32_t get_checksum(unsigned char *data) {
    int32_t state = (int32_t)(ESP_CHECKSUM_MAGIC);
    for (uint8_t i = 0; i < sizeof(data); i++) {
        state ^= data[i];
    }
    return state;
}

void get_32bit_as_char(char d[4], uint32_t v) {
    d[0] = (v >> 24) & 0xff;
    d[1] = (v >> 16) & 0xff;
    d[2] = (v >> 8) & 0xff;
    d[3] = v & 0xff;
}

void rom_cmd_checksum(unsigned char *data) {
    int32_t checksum = get_checksum(data);
    char z[4];
    get_32bit_as_char(z, checksum);
    BUART_SEND_CONST_STR(&z);
}

void rom_cmd_zero_checksum() {
    // zeroes
    int32_t zero = 0;
    char z[4];
    get_32bit_as_char(z, zero);
    BUART_SEND_CONST_STR(&z);
}

// ------------------------------------
// custom fn to handle cmds for ESP UART ROM bootloader
// sending SLIP packets to issue a cmd with data
// ------------------------------------
lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg) {
    switch (CMD_GET_CUR()) {

    // SYNC - ESP_SYNC (0x08)
    case LWESP_CMD_TCPIP_CIPSTATUS: {
        _dbg0("SYNC CMD PITCHO");
        // BUART_SEND_STR("AT\r\n");
        // break;
        active_cmd = ESP_SYNC;
        unsigned char cmd_data[36] = { 0x07, 0x07, 0x12, 0x20, [4 ... 35] = 0x55 };
        _dbg0("sync cmd to send - %s", cmd_data);
        // -- SLIP begin
        BUART_SEND_BEGIN_SLIP();
        // -- cmd structure (esptool.py:413)
        rom_cmd_req(ESP_SYNC, sizeof(cmd_data));
        rom_cmd_zero_checksum();
        // -- checksum is zero because it's sync command
        // rom_cmd_checksum(cmd_data);
        // -- sync data
        BUART_SEND_CONST_STR(&cmd_data);
        // -- SLIP end
        BUART_SEND_END_SLIP();
        break;
    }

    // READ_REG - ESP_READ_REG (0x0a)
    case LWESP_CMD_TCPIP_CIFSR: {
        active_cmd = ESP_READ_REG;
        _dbg0("READ REG ESP ROM BOOTLOADER");
        break;
    }

    // FLASH BEGIN - ESP_FLASH_BEGIN (0x02)
    // #1 - count num of blocks TODO
    // #2 - get erase size TODO
    // #3 - data (erase_size, num of blocks, write size, offset)
    // #4 - create request packet header
    // #5 - add data (#3)
    case LWESP_CMD_TCPIP_CIPSTART: {
        active_cmd = ESP_FLASH_BEGIN;
        // data structure:
        // #1 (4B) size to erase
        // #2 (4B) number of blocks
        // #3 (4B) data size of one block
        // #4 (4B) flash offset
        char cmd_data[16];

        size_t bin_size = msg->msg.conn_send.sent;
        uint32_t block_num = 4;
        size_t offset = msg->msg.conn_send.btw;

        char d_tmp[4] = {};
        // -- erase size
        get_32bit_as_char(d_tmp, (uint32_t)(bin_size));
        strcat(cmd_data, d_tmp);
        // -- number of blocks
        get_32bit_as_char(d_tmp, block_num);
        strcat(cmd_data, d_tmp);
        // -- one block size
        get_32bit_as_char(d_tmp, (uint32_t)(ESP_FLASH_WRITE_SIZE));
        strcat(cmd_data, d_tmp);
        // -- offset
        get_32bit_as_char(d_tmp, (uint32_t)(offset));
        strcat(cmd_data, d_tmp);

        // -- write command to UARt
        BUART_SEND_BEGIN_SLIP();
        // cmd structure (esptool.py:681)
        rom_cmd_req(ESP_FLASH_BEGIN, sizeof(cmd_data));
        rom_cmd_zero_checksum();
        // -- erase memory, start uploading at sent offset
        BUART_SEND_CONST_STR(&cmd_data);
        BUART_SEND_END_SLIP();

        break;
    }

    // FLASH DATA - ESP_FLASH_DATA (0x03)
    case LWESP_CMD_TCPIP_CIPSEND: {
        active_cmd = ESP_FLASH_DATA;
        _dbg0("FLASH DATA ESP ROM BOOTLOADER");
        break;
    }
    }

    return lwespOK;
}

static lwesp_recv_t recv_buff;

static void esp_upload_process_packet(lwesp_recv_t *rcv) {
    _dbg0("recv packet - %s", &rcv);
}

// ------------------------------------
// process & parse input data from ESP
// ------------------------------------
lwespr_t lwespi_upload_process(const void *data, size_t data_len) {
    uint8_t ch;
    const uint8_t *d = data;
    size_t d_len = data_len;
    static uint8_t ch_prev1, ch_prev2;

    _dbg0("MASLO ???----");
    /* Check status if device is available */
    if (!esp.status.f.dev_present) {
        return lwespERRNODEVICE;
    }

    uint8_t f_slip = 0;
    uint8_t f_escape = 0;
    while (d_len > 0) { /* Read entire set of characters from buffer */
        _dbg0("PROCESS-%s", data);
        ch = *d; /* Get next character */
        ++d;     /* Go to next character, must be here as it is used later on */
        --d_len; /* Decrease remaining length, must be here as it is decreased later too */

        if (!f_slip && ch == '\xc0') {
            // -- start SLIP packet
            f_slip = 1;
            RECV_UPLOAD_ADD(ch);
        } else if (f_escape) {
            // UART bootloader serial protocol replace:
            // 0xc0 for 0xdb 0xdc
            // 0xdb for 0xdb 0xdd
            f_escape = 0;
            if (ch == '\xdc') {
                RECV_UPLOAD_ADD('\xc0');
            } else if (ch == '\xdd') {
                RECV_UPLOAD_ADD('\xdb');
            }
        } else if (ch == '\xdb') {
            f_escape = 1;
        } else if (ch == '\xc0' && f_slip) {
            // -- end SLIP packet, start parsing
            RECV_UPLOAD_ADD(ch);
            esp_upload_process_packet(&recv_buff);
            RECV_UPLOAD_RESET();
        } else {
            RECV_UPLOAD_ADD(ch);
        }
    }

    return lwespOK;
}
