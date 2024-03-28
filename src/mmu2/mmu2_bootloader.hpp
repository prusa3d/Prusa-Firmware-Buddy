#pragma once

#include <array>
#include <initializer_list>

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_serial.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_bootloader_result.h"
#include "mmu2_bootloader_coroutine.hpp"
#include "mmu2_bootloader_memory_resource.hpp"

namespace MMU2 {

/// Class that communicated with the MMU2 bootloader.
/// Checks for firmware version, does firmware updates, and such.
class MMU2BootloaderManager final {
    template <typename T>
    using Task = bootloader::Task<T>;

public:
    MMU2BootloaderManager(MMU2Serial &serial);
    ~MMU2BootloaderManager();

    // Disable copy/move constructors, we're storing coroutine stacks in this class, cannot be moved
    MMU2BootloaderManager(const MMU2BootloaderManager &) = delete;
    MMU2BootloaderManager &operator=(const MMU2BootloaderManager &) = delete;

public:
    /// \returns if the manager is active and is trying to communicate with the MMU bootloader
    [[nodiscard]] bool is_active() const {
        return coroutine_.is_active();
    }

    /// Initializes the manager and attempts to start talking with the MMU.
    /// This is to be called after MMU reset.
    void start();

    void stop();

    /// Force firmware update on the following bootloader run.
    /// If called after bootloader sequence, you gotta restart the MMU to apply.
    void force_fw_update() {
        force_fw_update_ = true;
    }

    /// Returns result of the bootloader run
    inline MMU2BootloaderResult result() const {
        return result_;
    }

    /// Function to be executed periodically, advances main_coroutine
    void loop();

public:
    /// Memory resource (allocator/deallocator) for coroutine frames
    /// This is to prevent coroutines dynamic allocation
    bootloader::StaticStackMemoryResource<320> co_memory_resource;

private:
    /// Handle of the coroutine handling the comm
    Task<void> coroutine_;

    /// Handle of the resume point for the next loop (NextLoopAwaitable)
    std::coroutine_handle<> coroutine_resume_point_;

    /// Function handling the logic and everything,
    /// runs as a coroutine in the Marlin thread (gets continuation every loop)
    [[nodiscard]] Task<void> main_coroutine();

private:
    struct NextLoopAwaitable;

    /// Returns awaitable that resumes the execution in the next loop
    /// Resumable returns false if is_stopping_
    NextLoopAwaitable next_loop();

    /// Sends a primitive command to the MMU bootloader
    void send_command(std::initializer_list<char> cmd);

    /// Sets address for reading/writing operationrs
    Task<bool> set_address(uint16_t address);

    /// Asynchronously reads \param size bytes from the serial.
    /// \returns false on timeout
    [[nodiscard]] Task<bool> receive(uint8_t size, int32_t timeout_ms = 500);

private:
    /// Buffer for transmitting/receiving data
    alignas(4) std::array<uint8_t, 8> rtx_buffer;
    MMU2Serial &serial;

private:
    bool force_fw_update_ : 1 = false;
    bool is_stopping_ : 1 = false;

    MMU2BootloaderResult result_ = MMU2BootloaderResult::not_detected;
};

} // namespace MMU2
