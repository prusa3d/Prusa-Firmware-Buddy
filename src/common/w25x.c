#include <string.h>
#include <stdlib.h>
#include <w25x.h>
#include "log.h"
#include "main.h"
#include "FreeRTOS.h"
#include "w25x_communication.h"

LOG_COMPONENT_DEF(W25X, LOG_SEVERITY_DEBUG);

static const uint16_t PAGE_SIZE = 256;

static const uint8_t MFRID = 0xEF;
static const uint8_t DEVID = 0x13;
static const uint8_t DEVID_NEW = 0x16;

enum {
    CMD_ENABLE_WR = 0x06,
    CMD_ENABLE_WR_VSR = 0x50,
    CMD_DISABLE_WR = 0x04,
    CMD_RD_STATUS_REG = 0x05,
    CMD_WR_STATUS_REG = 0x01,
    CMD_RD_DATA = 0x03,
    CMD_RD_FAST = 0x0b,
    CMD_RD_FAST_D_O = 0x3b,
    CMD_RD_FAST_D_IO = 0xbb,
    CMD_PAGE_PROGRAM = 0x02,
    CMD_SECTOR_ERASE = 0x20,
    CMD_BLOCK32_ERASE = 0x52,
    CMD_BLOCK64_ERASE = 0xd8,
    CMD_CHIP_ERASE = 0xc7,
    CMD_CHIP_ERASE2 = 0x60,
    CMD_PWR_DOWN = 0xb9,
    CMD_PWR_DOWN_REL = 0xab,
    CMD_MFRID_DEVID = 0x90,
    CMD_MFRID_DEVID_D = 0x92,
    CMD_JEDEC_ID = 0x9f,
    CMD_RD_UID = 0x4b,
};

typedef enum {
    W25X_STATUS_BUSY = 0x01,
    W25X_STATUS_WEL = 0x02,
    W25X_STATUS_BP0 = 0x04,
    W25X_STATUS_BP1 = 0x08,
    W25X_STATUS_TB = 0x20,
    W25X_STATUS_SRP = 0x80,
} w25x_status_t;

static int w25x_mfrid_devid(uint8_t *devid);
static void w25x_wait_busy(void);

static uint8_t device_id;

bool w25x_init(bool init_dma) {

    if (!w25x_communication_init(init_dma))
        return false;

    w25x_wait_busy();

    if (!w25x_mfrid_devid(&device_id))
        return false;

    return true;
}

uint32_t w25x_get_sector_count() {
    if (device_id == DEVID) {
        return 256;
    } else if (device_id == DEVID_NEW) {
        return 2048;
    } else {
        abort();
    }
}

static void w25x_enable_wr(void) {
    w25x_cs_low();
    w25x_send_byte(CMD_ENABLE_WR); // send command 0x06
    w25x_cs_high();
}

static w25x_status_t w25x_rd_status_reg() {
    w25x_cs_low();
    w25x_send_byte(CMD_RD_STATUS_REG);          // send command 0x90
    w25x_status_t status = w25x_receive_byte(); // receive value
    w25x_cs_high();
    return status;
}

static void w25x_wait_busy(void) {
    while (w25x_rd_status_reg() & W25X_STATUS_BUSY)
        ;
}

void w25x_rd_data(uint32_t addr, uint8_t *data, uint16_t cnt) {
    w25x_cs_low();
    w25x_send_byte(CMD_RD_DATA);           // send command 0x03
    w25x_send_byte(((uint8_t *)&addr)[2]); // send addr bits 16..23
    w25x_send_byte(((uint8_t *)&addr)[1]); // send addr bits 8..15
    w25x_send_byte(((uint8_t *)&addr)[0]); // send addr bits 0..7
    w25x_receive(data, cnt);
    w25x_cs_high();
}

void w25x_page_program_single(uint32_t addr, const uint8_t *data, uint16_t cnt) {
    w25x_enable_wr();
    w25x_cs_low();
    w25x_send_byte(CMD_PAGE_PROGRAM);      // send command 0x02
    w25x_send_byte(((uint8_t *)&addr)[2]); // send addr bits 16..23
    w25x_send_byte(((uint8_t *)&addr)[1]); // send addr bits 8..15
    w25x_send_byte(((uint8_t *)&addr)[0]); // send addr bits 0..7
    w25x_send(data, cnt);                  // send data
    w25x_cs_high();
    w25x_wait_busy();
}

void w25x_page_program(uint32_t addr, const uint8_t *data, uint16_t cnt) {
    // The Page Program instruction allows from one byte
    // to 256 bytes (a page) of data to be programmed

    // Write unaligned part first
    uint32_t addr_align = addr % PAGE_SIZE;
    if (addr_align != 0) {
        int cnt_align = PAGE_SIZE - addr_align;
        if (cnt_align >= cnt) {
            w25x_page_program_single(addr, data, cnt);
            return;
        }
        w25x_page_program_single(addr, data, cnt_align);
        addr += cnt_align;
        data += cnt_align;
        cnt -= cnt_align;
    }

    // Write all full pages
    while (cnt >= PAGE_SIZE) {
        w25x_page_program_single(addr, data, PAGE_SIZE);
        addr += PAGE_SIZE;
        data += PAGE_SIZE;
        cnt -= PAGE_SIZE;
    }

    // Write the remaining data
    if (cnt > 0) {
        w25x_page_program_single(addr, data, cnt);
    }
}

void w25x_erase(uint8_t cmd, uint32_t addr) {
    w25x_enable_wr();
    w25x_cs_low();
    w25x_send_byte(cmd);                   // send command 0x20
    w25x_send_byte(((uint8_t *)&addr)[2]); // send addr bits 16..23
    w25x_send_byte(((uint8_t *)&addr)[1]); // send addr bits 8..15
    w25x_send_byte(((uint8_t *)&addr)[0]); // send addr bits 0..7
    w25x_cs_high();
    w25x_wait_busy();
}

void w25x_sector_erase(uint32_t addr) {
    w25x_erase(CMD_SECTOR_ERASE, addr);
}

void w25x_block32_erase(uint32_t addr) {
    w25x_erase(CMD_BLOCK32_ERASE, addr);
}

void w25x_block64_erase(uint32_t addr) {
    w25x_erase(CMD_BLOCK64_ERASE, addr);
}

void w25x_chip_erase(void) {
    w25x_enable_wr();
    w25x_cs_low();
    w25x_send_byte(CMD_CHIP_ERASE); // send command 0xc7
    w25x_cs_high();
    w25x_wait_busy();
}

void w25x_rd_uid(uint8_t *uid) {
    w25x_cs_low();
    w25x_send_byte(CMD_RD_UID); // send command 0x4b
    uint8_t cnt = 4;            // 4 dummy bytes
    while (cnt--)               // receive dummy bytes
        w25x_receive_byte();
    cnt = 8;      // 8 bytes UID
    while (cnt--) // receive UID
        uid[7 - cnt] = w25x_receive_byte();
    w25x_cs_high();
}

int w25x_mfrid_devid(uint8_t *devid) {
    w25x_cs_low();
    w25x_send_byte(CMD_MFRID_DEVID); // send command 0x90
    uint8_t cnt = 3;                 // 3 address bytes
    while (cnt--)                    // send address bytes
        w25x_send_byte(0x00);
    uint8_t w25x_mfrid = w25x_receive_byte(); // receive mfrid
    uint8_t w25x_devid = w25x_receive_byte(); // receive devid
    w25x_cs_high();
    if (devid)
        *devid = w25x_devid;
    return ((w25x_mfrid == MFRID) && ((w25x_devid == DEVID) || (w25x_devid == DEVID_NEW)));
}
