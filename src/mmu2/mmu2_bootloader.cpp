#include "mmu2_bootloader.hpp"

#include <memory>
#include <cstring>

#include <scope_guard.hpp>
#include <timing.h>
#include <log.h>
#include <wiring_time.h>
#include <mmu2/mmu_fw_attached_version.hpp>

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_supported_version.h"

using namespace MMU2;

LOG_COMPONENT_REF(MMU2);

// Check that the MMU firmware file version matches the version the code is written for
static_assert(mmu_fw_attached_version[0] == mmuVersionMajor);
static_assert(mmu_fw_attached_version[1] == mmuVersionMinor);
static_assert(mmu_fw_attached_version[2] == mmuVersionPatch);

struct MMU2BootloaderManager::NextLoopAwaitable {
    bool await_ready() const noexcept {
        return mgr.is_stopping_;
    }
    bool await_resume() const noexcept {
        mgr.coroutine_resume_point_ = {};
        return !mgr.is_stopping_;
    }
    void await_suspend(std::coroutine_handle<> h) {
        assert(!mgr.coroutine_resume_point_);
        mgr.coroutine_resume_point_ = h;
    }

    MMU2BootloaderManager &mgr;
};

MMU2BootloaderManager::NextLoopAwaitable MMU2BootloaderManager::next_loop() {
    return NextLoopAwaitable { *this };
}

MMU2BootloaderManager::MMU2BootloaderManager(MMU2Serial &serial)
    : serial(serial) {}

MMU2BootloaderManager::~MMU2BootloaderManager() {
    stop();

    // Make sure we clear the coroutine before the co_stack is destroyed
    coroutine_.clear();
}

void MMU2BootloaderManager::start() {
    stop();

    // Reinitialize the coroutine
    coroutine_ = main_coroutine();
}

void MMU2BootloaderManager::stop() {
    if (!is_active()) {
        return;
    }

    is_stopping_ = true;
    while (is_active()) {
        loop();

        // We should be stopped after a single resume actually.
        assert(!is_active());
    }
    is_stopping_ = false;
}

void MMU2BootloaderManager::loop() {
    if (is_active()) {
        assert(coroutine_resume_point_);
        coroutine_resume_point_();
    }
}

MMU2BootloaderManager::Task<void> MMU2BootloaderManager::main_coroutine() {
    result_ = MMU2BootloaderResult::not_detected;

    // Initial suspend - we've created the coroutine in activate(), wait for the loop() to proceed
    if (!co_await next_loop()) {
        co_return;
    }

    // Try asking for the bootloader identifier a few times - the MMU might take a bit to boot up
    {
        const auto start_ms = millis();
        while (true) {
            // S - return software identifier (7 bytes string response)
            send_command({ 'S' });
            if (co_await receive(7)) {
                break;
            }

            if (is_stopping_) {
                co_return;
            }

            // Timeout -> exit the bootloader
            if (ticks_diff(millis(), start_ms) >= 10000) {
                log_info(MMU2, "Bootloader handshake timeout");
                goto exit;
            }
        }

        // If we've got after this point and return early, it's a comm error.
        result_ = MMU2BootloaderResult::comm_error;

        // Append \0 after the identifier so that we can work with as string
        rtx_buffer[7] = '\0';

        log_info(MMU2, "Software identifier: %s", rtx_buffer.data());

        if (strcmp(reinterpret_cast<const char *>(rtx_buffer.data()), "MMCTL20")) {
            log_info(MMU2, "Unsupported bootloader");
            goto exit;
        }
    }

    // Enter programming mode - necessary for flash reading/writing
    // We have extra constant "MMCTL20" at the end of the programming command - this is for extra safety
    // so that the MMU logic doesn't acidentally reflash the firmware
    {
        send_command({ 'P', 'M', 'M', 'C', 'T', 'L', '2', '0' });
        if (!co_await receive(1) || rtx_buffer[0] != '\r') {
            log_info(MMU2, "Failed to enter programming mode");
            goto exit;
        }
    }

    // Obtain the firmware version information - should always be on the same position in the firmware progmem
    {
        struct __attribute__((packed)) FWVersionData {
            uint8_t project_major;
            uint8_t project_minor;
            uint16_t project_revision;
            uint16_t project_build_number;
        };
        static_assert(sizeof(FWVersionData) == 6);

        static constexpr uint16_t fwVersionDataAddress = 0xAC;
        static_assert((fwVersionDataAddress & 1) == 0);

        // Set read address
        if (!co_await set_address(fwVersionDataAddress)) {
            log_info(MMU2, "Failed to set read address");
            goto exit;
        }

        // Read the FWVersionData
        send_command({ 'g', 0, sizeof(FWVersionData), 'F' });
        if (!co_await receive(sizeof(FWVersionData))) {
            goto exit;
        }

        const FWVersionData *mmu_version = reinterpret_cast<const FWVersionData *>(rtx_buffer.data());
        log_info(MMU2, "FW version: %i.%i.%i", mmu_version->project_major, mmu_version->project_minor, mmu_version->project_revision);

        if (!force_fw_update_ && mmu_version->project_major == mmuVersionMajor && mmu_version->project_minor == mmuVersionMinor && mmu_version->project_revision == mmuVersionPatch) {
            log_info(MMU2, "Firmware is up-to-date.");
            result_ = MMU2BootloaderResult::fw_up_to_date;
            goto exit;
        }
    }

    // Execute the FW update
    {
        log_info(MMU2, "Starting firmware update...");

        // If we get after this point and return early, it's a firmware update error
        result_ = MMU2BootloaderResult::flashing_error;

        auto file = fopen("/internal/res/mmu/fw.bin", "rb");
        if (!file) {
            log_critical(MMU2, "Failed to open fw.bin");
            goto exit;
        }
        ScopeGuard _fg([&] {
            fclose(file);
        });

        // Dynamically allocate buffer for firmware update
        // Use 128 bytes, as that is the page size of the At32U8 MMU MCU.
        // We cannot use less, because the writes have to be page aligned (and the second write would get misaligned).
        // We don't want to go any further probably because the bootloader has a 256B RX buffer,
        // so sending 256 bytes + 4 bytes header could make it overflow.
        auto bufferPtr = std::make_unique<std::array<uint8_t, 128>>();
        auto &buffer = *bufferPtr;
        size_t total_bytes_written = 0;

        const auto write_addr = 0;
        if (!co_await set_address(write_addr)) {
            log_info(MMU2, "Failed to set write address %u %i", write_addr, rtx_buffer[0]);
            goto exit;
        }

        while (!feof(file)) {
            auto bytes_to_write = fread(buffer.data(), 1, buffer.size(), file);

            if (auto e = ferror(file)) {
                log_critical(MMU2, "FW file read error: %i", e);
                goto exit;
            }

            // The programming is done in 2-byte words, enforce it
            if (bytes_to_write % 1 != 0) {
                buffer[bytes_to_write++] = '\0';
            }

            // Start Block Flash Load
            send_command({ 'B', static_cast<char>((bytes_to_write >> 8) & 0xff), static_cast<char>(bytes_to_write & 0xff), 'F' });

            // Write the file data to the UART
            serial.write(buffer.data(), bytes_to_write);

            log_info(MMU2, "Flashing firmware: %u B", total_bytes_written);

            if (!co_await receive(1) || rtx_buffer[0] != '\r') {
                log_critical(MMU2, "Firmware write error: %i", rtx_buffer[0]);
                goto exit;
            }

            total_bytes_written += bytes_to_write;

            // Give some space for doing work
            if (!co_await next_loop()) {
                co_return;
            }
        }

        log_info(MMU2, "Firmware update finished (%u B)", total_bytes_written);
        result_ = MMU2BootloaderResult::fw_updated;
        force_fw_update_ = false;
    }

exit:
    // We have jumped here if receive returned false - that could have been because we were stopping
    if (is_stopping_) {
        co_return;
    }

    log_info(MMU2, "Exiting the bootloader");

    // Exit bootloader - multiple attempts to make sure we've exit
    for (uint8_t attempt = 0; attempt < 4; attempt++) {
        send_command({ 'E' });

        if (co_await receive(1) && rtx_buffer[0] == '\r') {
            break;
        }

        if (is_stopping_) {
            co_return;
        }
    }

    // Wait until the actual bootloader exit - this can take up to a second
    // during which the bootloader is still accepting commands
    // and sending anything could prolong the exit
    for (const auto start_ms = millis(); !is_stopping_ && ticks_diff(millis(), start_ms) < 1000;) {
        co_await next_loop();
    }
}

void MMU2BootloaderManager::send_command(std::initializer_list<char> cmd) {
    assert(rtx_buffer.size() >= cmd.size());

    // UART is using DMA, which cannot read from the CCMRAM (=stack)
    // -> we have to put the command in the tx buffer
    std::copy(cmd.begin(), cmd.end(), rtx_buffer.begin());

    // Write is blocking. If it ever stops, we need to make this co_waitable
    serial.write(reinterpret_cast<const uint8_t *>(rtx_buffer.data()), cmd.size());
}

MMU2BootloaderManager::Task<bool> MMU2BootloaderManager::set_address(uint16_t address) {
    // For whatever reason, the bootloader takes the address divided by two
    send_command({ 'A', static_cast<char>((address >> 9) & 0xff), static_cast<char>((address >> 1) & 0xff) });
    co_return (co_await receive(1)) && rtx_buffer[0] == '\r';
}

MMU2BootloaderManager::Task<bool> MMU2BootloaderManager::receive(uint8_t size, int32_t timeout_ms) {
    auto last_activity_ms = millis();
    uint8_t rcvd_bytes = 0;

    while (rcvd_bytes < size) {
        // No activity from the MMU for the specified timeout -> exit
        if (ticks_diff(millis(), last_activity_ms) >= timeout_ms) {
            log_critical(MMU2, "Receive timed out");
            co_return false;
        }

        // Try reading one byte from the serial
        const auto ch = serial.read();

        // No char received -> try again in the next loop() call
        if (ch < 0) {
            if (!co_await next_loop()) {
                co_return false;
            }
            continue;
        }

        last_activity_ms = millis();
        rtx_buffer[rcvd_bytes] = static_cast<uint8_t>(ch);
        rcvd_bytes++;
    }

    co_return true;
}
