/// @file
/// Driver for the MT29F series SPI NAND flash memories.
///
/// Presents a flat byte-address interface over the NAND page/block
/// structure. On-die ECC is used; uncorrectable errors are reported
/// via fetch_error().
#pragma once

#include "spi_flash_bus.hpp"
#include <cstdint>

namespace buddy::hw {
class OutputPin;
}

class Mt29fFlash {
public:
    static constexpr uint32_t page_size = 4096;
    /// A page must be written in a single program op: PROGRAM LOAD resets the chip
    /// cache to all-FF, so a second program of the same page corrupts its ECC
    static constexpr uint32_t prog_size = page_size;
    static constexpr uint32_t pages_per_block = 64;
    static constexpr uint32_t blocks_per_die = 2048;
    static constexpr uint32_t num_dies = 2;
    static constexpr uint32_t block_size = page_size * pages_per_block; // 256KB

    static Mt29fFlash &instance();

    /// Initialize the device. Returns true on success.
    [[nodiscard]] bool init();

    /// True after a successful init()
    [[nodiscard]] bool is_initialized() const { return initialized; }

    void read(uint32_t addr, uint8_t *data, uint32_t len);
    void program(uint32_t addr, const uint8_t *data, uint32_t len);
    void erase_block(uint32_t addr);
    void chip_erase();

    [[nodiscard]] uint32_t erase_block_size() const { return block_size; }
    [[nodiscard]] uint32_t block_count() const { return blocks_per_die * num_dies; }
    [[nodiscard]] uint32_t capacity() const { return erase_block_size() * block_count(); }

    void set_error(int error);
    [[nodiscard]] int fetch_error();

private:
    SpiFlashBus &bus;
    const buddy::hw::OutputPin &cs;

    uint8_t current_die = 0;
    int current_error = 0;
    bool initialized = false;

    struct Address {
        uint8_t die;
        uint32_t block;
        uint32_t page;
        uint16_t col;
    };

    Mt29fFlash(SpiFlashBus &bus, const buddy::hw::OutputPin &cs);

    void unlock_bus();
    void select_die(uint8_t die);
    void write_enable();
    [[nodiscard]] uint8_t get_feature(uint8_t addr);
    void set_feature(uint8_t addr, uint8_t value);
    [[nodiscard]] bool wait_busy();
    [[nodiscard]] bool check_ecc();

    [[nodiscard]] static Address decompose(uint32_t addr);

    void read_page(const Address &a, uint8_t *data, uint16_t len);
    void program_page(const Address &a, const uint8_t *data, uint16_t len);
};
