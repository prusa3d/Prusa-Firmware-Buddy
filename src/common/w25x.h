///
/// \file
///
/// Driver for the W25xxx family of SPI flash memories.
///
/// The driver is split into two parts.
/// The `w25x_communication.h/c` handles communication with the chip and this part is considered private to the w25x module.
/// The `w25x.h/c` handles the high-level operations with the chip (read, erase, write, etc)
///
#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

/// Initialize the w25x module
///
/// This has to be called after the underlying SPI has been initialized
/// and assigned using w25x_spi_assign.
///
/// @param init_dma Dynamically allocate resources for DMA transfers
///        DMA and RTOS facilities are not used if false,
///        it has no effect if it was already allocated by previous call
/// @retval true on success
/// @retval false otherwise.
extern bool w25x_init(bool init_dma);

/// Return the number of available sectors
extern uint32_t w25x_get_sector_count();

/// Read data from the flash.
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_rd_data(uint32_t addr, uint8_t *data, uint16_t cnt);

/// Write data to the flash (the sector has to be erased first)
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_page_program(uint32_t addr, const uint8_t *data, uint16_t cnt);

/// Erase single sector of the flash
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_sector_erase(uint32_t addr);

/// Erase block of 32 kB of the flash
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_block32_erase(uint32_t addr);

/// Erase block of 64 kB of the flash
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_block64_erase(uint32_t addr);

/// Erase the whole flash memory
/// Errors can be checked (and cleared) using w25x_fetch_error()
extern void w25x_chip_erase(void);

/// Fetch and clear error of a previous operation.
/// Returns 0 if there hasn't been any error
extern int w25x_fetch_error(void);

/// Assign handle of configured and ready-to-use SPI handle
extern void w25x_spi_assign(SPI_HandleTypeDef *spi_handle);

/// This should be called when the underlying SPI's DMA finishes DMA transfer (send)
extern void w25x_spi_transfer_complete_callback(void);

/// This should be called when the underlying SPI's DMA finishes DMA transfer (receive)
extern void w25x_spi_receive_complete_callback(void);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
