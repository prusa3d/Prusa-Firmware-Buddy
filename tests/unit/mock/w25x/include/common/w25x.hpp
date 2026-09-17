/// @file Test double of the W25xFlash driver, shadowing src/common/w25x.hpp.
#pragma once

#include <cstdint>
#include <mock/flash_sim.hpp>
#include <utils/uncopyable.hpp>

class W25xFlash : public Uncopyable {
public:
    static constexpr uint32_t block_size = 4096;

    // BUDDY address layout, the board the unit tests build for
    static constexpr uint32_t dump_start_address = 0;
    static constexpr uint32_t error_start_address = 49 * block_size;
    static constexpr uint32_t pp_start_address = 50 * block_size;
    static constexpr uint32_t fs_start_address = 51 * block_size;
    static constexpr uint32_t pp_size = fs_start_address - pp_start_address;

    static constexpr uint32_t simulated_capacity = 1024 * 1024;

    static W25xFlash &instance() {
        static W25xFlash flash;
        return flash;
    }

    void read(uint32_t addr, uint8_t *data, uint32_t len) {
        record(sim_flash.read(addr, data, len));
    }

    void program(uint32_t addr, const uint8_t *data, uint32_t len) {
        record(sim_flash.program(addr, data, len));
    }

    void erase_block(uint32_t addr) {
        record(sim_flash.erase_block(addr));
    }

    /// Fetch and clear error of a previous operation; 0 means no error
    int fetch_error() {
        const int error = current_error;
        current_error = 0;
        return error;
    }

    /// Erased chip, no pending error
    void reset() {
        sim_flash.reset();
        current_error = 0;
    }

    mock::FlashSim<simulated_capacity, block_size> sim_flash;

private:
    W25xFlash() = default;

    void record(bool ok) {
        if (!ok) {
            current_error = 1;
        }
    }

    int current_error = 0;
};
