/*
 * w25x20cl.c
 *
 *  Created on: Jun 20, 2019
 *      Author: ponyxis
 */

//w25x20cl.c

#include <w25x.h>
#include "main.h"

#define _MFRID     0xEF
#define _DEVID     0x13
#define _DEVID_NEW 0x16

#define _CMD_ENABLE_WR     0x06
#define _CMD_ENABLE_WR_VSR 0x50
#define _CMD_DISABLE_WR    0x04
#define _CMD_RD_STATUS_REG 0x05
#define _CMD_WR_STATUS_REG 0x01
#define _CMD_RD_DATA       0x03
#define _CMD_RD_FAST       0x0b
#define _CMD_RD_FAST_D_O   0x3b
#define _CMD_RD_FAST_D_IO  0xbb
#define _CMD_PAGE_PROGRAM  0x02
#define _CMD_SECTOR_ERASE  0x20
#define _CMD_BLOCK32_ERASE 0x52
#define _CMD_BLOCK64_ERASE 0xd8
#define _CMD_CHIP_ERASE    0xc7
#define _CMD_CHIP_ERASE2   0x60
#define _CMD_PWR_DOWN      0xb9
#define _CMD_PWR_DOWN_REL  0xab
#define _CMD_MFRID_DEVID   0x90
#define _CMD_MFRID_DEVID_D 0x92
#define _CMD_JEDEC_ID      0x9f
#define _CMD_RD_UID        0x4b

#define _CS_LOW()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, 0)
#define _CS_HIGH() HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, 1)

static inline uint8_t spi_txrx(uint8_t tx);

#define _SPI_TX(b) spi_txrx(b)
#define _SPI_RX()  spi_txrx(0xff)

extern SPI_HandleTypeDef hspi3;

static inline uint8_t spi_txrx(uint8_t tx) {
    uint8_t _in = 0;
    HAL_SPI_TransmitReceive(&hspi3, &tx, &_in, 1, HAL_MAX_DELAY);
    return _in;
}

int w25x_mfrid_devid(void);

int8_t w25x_init(void) {
    //PIN_OUT(W25X20CL_PIN_CS);
    _CS_HIGH();
    W25X_SPI_ENTER();
    if (!w25x_mfrid_devid())
        return 0;
    return 1;
}

void w25x_enable_wr(void) {
    _CS_LOW();
    _SPI_TX(_CMD_ENABLE_WR); // send command 0x06
    _CS_HIGH();
}

void w25x_disable_wr(void) {
    _CS_LOW();
    _SPI_TX(_CMD_DISABLE_WR); // send command 0x04
    _CS_HIGH();
}

uint8_t w25x_rd_status_reg(void) {
    _CS_LOW();
    _SPI_TX(_CMD_RD_STATUS_REG); // send command 0x90
    uint8_t val = _SPI_RX();     // receive value
    _CS_HIGH();
    return val;
}

void w25x_wr_status_reg(uint8_t val) {
    _CS_LOW();
    _SPI_TX(_CMD_WR_STATUS_REG); // send command 0x90
    _SPI_TX(val);                // send value
    _CS_HIGH();
}

void w25x_rd_data(uint32_t addr, uint8_t *data, uint16_t cnt) {
    _CS_LOW();
    _SPI_TX(_CMD_RD_DATA);          // send command 0x03
    _SPI_TX(((uint8_t *)&addr)[2]); // send addr bits 16..23
    _SPI_TX(((uint8_t *)&addr)[1]); // send addr bits 8..15
    _SPI_TX(((uint8_t *)&addr)[0]); // send addr bits 0..7
    while (cnt--)                   // receive data
        *(data++) = _SPI_RX();
    _CS_HIGH();
}

void w25x_page_program(uint32_t addr, uint8_t *data, uint16_t cnt) {
    _CS_LOW();
    _SPI_TX(_CMD_PAGE_PROGRAM);     // send command 0x02
    _SPI_TX(((uint8_t *)&addr)[2]); // send addr bits 16..23
    _SPI_TX(((uint8_t *)&addr)[1]); // send addr bits 8..15
    _SPI_TX(((uint8_t *)&addr)[0]); // send addr bits 0..7
    while (cnt--)                   // send data
        _SPI_TX(*(data++));
    _CS_HIGH();
}

void w25x_page_program_P(uint32_t addr, uint8_t *data, uint16_t cnt) {
    _CS_LOW();
    _SPI_TX(_CMD_PAGE_PROGRAM);     // send command 0x02
    _SPI_TX(((uint8_t *)&addr)[2]); // send addr bits 16..23
    _SPI_TX(((uint8_t *)&addr)[1]); // send addr bits 8..15
    _SPI_TX(((uint8_t *)&addr)[0]); // send addr bits 0..7
    while (cnt--)                   // send data
        _SPI_TX(*(data++));
    _CS_HIGH();
}

void w25x_erase(uint8_t cmd, uint32_t addr) {
    _CS_LOW();
    _SPI_TX(cmd);                   // send command 0x20
    _SPI_TX(((uint8_t *)&addr)[2]); // send addr bits 16..23
    _SPI_TX(((uint8_t *)&addr)[1]); // send addr bits 8..15
    _SPI_TX(((uint8_t *)&addr)[0]); // send addr bits 0..7
    _CS_HIGH();
}

void w25x_sector_erase(uint32_t addr) {
    return w25x_erase(_CMD_SECTOR_ERASE, addr);
}

void w25x_block32_erase(uint32_t addr) {
    return w25x_erase(_CMD_BLOCK32_ERASE, addr);
}

void w25x_block64_erase(uint32_t addr) {
    return w25x_erase(_CMD_BLOCK64_ERASE, addr);
}

void w25x_chip_erase(void) {
    _CS_LOW();
    _SPI_TX(_CMD_CHIP_ERASE); // send command 0xc7
    _CS_HIGH();
}

void w25x_rd_uid(uint8_t *uid) {
    _CS_LOW();
    _SPI_TX(_CMD_RD_UID); // send command 0x4b
    uint8_t cnt = 4;      // 4 dummy bytes
    while (cnt--)         // receive dummy bytes
        _SPI_RX();
    cnt = 8;      // 8 bytes UID
    while (cnt--) // receive UID
        uid[7 - cnt] = _SPI_RX();
    _CS_HIGH();
}

int w25x_mfrid_devid(void) {
    _CS_LOW();
    _SPI_TX(_CMD_MFRID_DEVID); // send command 0x90
    uint8_t cnt = 3;           // 3 address bytes
    while (cnt--)              // send address bytes
        _SPI_TX(0x00);
    uint8_t w25x_mfrid = _SPI_RX(); // receive mfrid
    uint8_t w25x_devid = _SPI_RX(); // receive devid
    _CS_HIGH();
    return ((w25x_mfrid == _MFRID) && ((w25x_devid == _DEVID) || (w25x_devid == _DEVID_NEW)));
}

void w25x_wait_busy(void) {
    while (w25x_rd_status_reg() & W25X_STATUS_BUSY)
        ;
}
