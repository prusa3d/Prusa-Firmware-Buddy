//w25x.h
#pragma once

#include <inttypes.h>

static const uint8_t W25X_STATUS_BUSY = 0x01;
static const uint8_t W25X_STATUS_WEL = 0x02;
static const uint8_t W25X_STATUS_BP0 = 0x04;
static const uint8_t W25X_STATUS_BP1 = 0x08;
static const uint8_t W25X_STATUS_TB = 0x20;
static const uint8_t W25X_STATUS_SRP = 0x80;

#define W25X_SPI_ENTER() // spi_setup(W25X20CL_SPCR, W25X20CL_SPSR)

#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)

extern int8_t w25x_init(void);
extern void w25x_enable_wr(void);
extern void w25x_disable_wr(void);
extern uint8_t w25x_rd_status_reg(void);
extern void w25x_wr_status_reg(uint8_t val);
extern void w25x_rd_data(uint32_t addr, uint8_t *data, uint16_t cnt);
extern void w25x_page_program(uint32_t addr, const uint8_t *data, uint16_t cnt);
extern void w25x_sector_erase(uint32_t addr);
extern void w25x_block32_erase(uint32_t addr);
extern void w25x_block64_erase(uint32_t addr);
extern void w25x_chip_erase(void);
extern void w25x_rd_uid(uint8_t *uid);
extern void w25x_wait_busy(void);

#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
