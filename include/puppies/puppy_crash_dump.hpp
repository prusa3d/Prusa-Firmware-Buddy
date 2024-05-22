#pragma once
#include <cstdint>
#include <puppies/BootloaderProtocol.hpp>
#include <puppies/crash_dump_shared.hpp>
#include <crash_dump/crash_dump_distribute.hpp>
#include <span>

namespace buddy::puppies::crash_dump {
/**
 * @brief Fetches the fw descriptor of the puppy
 *
 * @param buffer Buffer to be used for the communication
 * @param flasher Bootloader protocol instance with correct address set
 */
std::optional<puppy_crash_dump::FWDescriptor>
fetch_fw_descriptor(std::span<uint8_t> buffer, BootloaderProtocol &flasher, const char *puppy_name);

/**
 * @brief Downloads crash dump on the given offset (if present) and stores it into a file
 *
 * @param buffer Buffer to be used for the communication
 * @param flasher Bootloader protocol instance with correct address set
 * @param dump_offset App offset where the dump starts
 * @param file_path Path to the file
 */
bool download_dump_into_file(std::span<uint8_t> buffer, BootloaderProtocol &flasher,
    const char *puppy_name, const char *file_path);

/**
 * @brief Query whether a crash dump from puppies is stored in filesystem
 *
 */
[[nodiscard]] bool is_a_dump_in_filesystem();

/**
 * @brief Uploads crash dumps from filesystem to usb
 * @return true if at least one dump was uploaded
 */
bool save_dumps_to_usb();

/**
 * @brief Uploads crash dumps from filesystem to a server
 * @return true if at least one dump was uploaded
 */
bool upload_dumps_to_server();

/**
 * @brief Removes crash dumps from filesystem
 * @return true if at least one dump was removed
 */
bool remove_dumps_from_filesystem();
}; // namespace buddy::puppies::crash_dump
