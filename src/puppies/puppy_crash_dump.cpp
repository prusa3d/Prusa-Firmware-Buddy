#include <puppies/puppy_crash_dump.hpp>
#include <unique_file_ptr.hpp>
#include <logging/log.hpp>
#include <path_utils.h>
#include <sys/stat.h>
#include <cstring>
#include <ranges>

#include <puppies/puppy_constants.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies::crash_dump {
std::optional<puppy_crash_dump::FWDescriptor>
fetch_fw_descriptor(std::span<uint8_t> buffer, BootloaderProtocol &flasher, const char *puppy_name) {
    debug_assert(buffer.size() >= puppy_crash_dump::APP_DESCRIPTOR_LENGTH);

    if (auto st = flasher.read_flash_cmd(puppy_crash_dump::APP_DESCRIPTOR_OFFSET, buffer.data(), puppy_crash_dump::APP_DESCRIPTOR_LENGTH);
        st != BootloaderProtocol::COMMAND_OK) {
        log_error(Puppies, "Failed getting descriptor from %s", puppy_name);
        return std::nullopt;
    }

    return *reinterpret_cast<puppy_crash_dump::FWDescriptor *>(buffer.data());
}

bool download_dump_into_file(std::span<uint8_t> buffer,
    BootloaderProtocol &flasher, const char *puppy_name, const char *file_path) {

    const auto res = fetch_fw_descriptor(buffer, flasher, puppy_name);
    if (!res.has_value()) {
        return false;
    }
    const puppy_crash_dump::FWDescriptor &fw_descriptor = res.value();

    if (fw_descriptor.stored_type != puppy_crash_dump::FWDescriptor::StoredType::crash_dump) {
        log_info(Puppies, "%s doesn't contain crash_dump", puppy_name); // not an error
        return false;
    }

    unique_file_ptr dump_file(fopen(file_path, "wb"));
    if (!dump_file) {
        log_error(Puppies, "Unable to open file to save %s crash_dump", puppy_name);
        return false;
    }

    for (uint32_t cur_offset = 0; cur_offset < fw_descriptor.dump_size; cur_offset += buffer.size()) {
        const uint32_t read_sz = std::min(fw_descriptor.dump_size - cur_offset,
            std::min(static_cast<uint32_t>(buffer.size()), static_cast<uint32_t>(BootloaderProtocol::MAX_RESPONSE_DATA_LEN)));
        if (auto st = flasher.read_flash_cmd(fw_descriptor.dump_offset + cur_offset, buffer.data(), read_sz);
            st != BootloaderProtocol::COMMAND_OK) {
            log_error(Puppies, "Failed reading crash_dump from %s", puppy_name);
            return false;
        }
        if (auto rc = fwrite(buffer.data(), 1, read_sz, dump_file.get()); rc != read_sz) {
            log_error(Puppies, "File saving failed during %s crash_dump download rc:%d", puppy_name, rc);
            return false;
        }
    }
    log_info(Puppies, "Successfully downloaded crash_dump for %s", puppy_name);
    return true;
}

bool is_a_dump_in_filesystem() {
    for (auto crash_dump_path : buddy::puppies::DOCKS | std::views::transform(buddy::puppies::get_crash_dump_path)) {
        if (crash_dump_path && file_exists(*crash_dump_path)) {
            return true;
        }
    }
    return false;
}

bool save_dumps_to_usb() {
    bool rc { false };
    for (auto crash_dump_path : buddy::puppies::DOCKS | std::views::transform(buddy::puppies::get_crash_dump_path)) {
        if (!crash_dump_path || !file_exists(*crash_dump_path)) {
            continue;
        }
        unique_file_ptr dump_file(fopen(*crash_dump_path, "rb"));
        if (!dump_file) {
            continue; // unable to open
        }

        static constexpr size_t buffer_size { 128 };

        static constexpr size_t max_dump_path_length { []() {
            size_t max_size { 0 };
            for (auto crash_dump_path : buddy::puppies::DOCKS | std::views::transform(buddy::puppies::get_crash_dump_path)) {
                auto size = crash_dump_path ? std::char_traits<char>::length(*crash_dump_path) : 0;
                max_size = std::max(max_size, size);
            }
            return max_size;
        }() };
        // make sure buffer fits the longest path
        static_assert(buffer_size + sizeof("/internal/") >= max_dump_path_length + sizeof("/usb/") + 1);

        char buffer[buffer_size] { "/usb/" };
        strcat(buffer, *crash_dump_path + strlen("/internal/")); // concatenate the path after /internal/

        unique_file_ptr usb_file(fopen(buffer, "wb"));
        if (!usb_file) {
            continue; // unable to open
        }

        bool errored { false };
        while (!feof(dump_file.get())) {
            auto read = fread(buffer, 1, buffer_size, dump_file.get());
            if (ferror(dump_file.get())) {
                errored = true;
                break;
            }
            fwrite(buffer, 1, read, usb_file.get());
            if (ferror(usb_file.get())) {
                errored = true;
                break;
            }
        }
        if (errored) {
            continue;
        }
        rc = true;
    }
    return rc;
}

bool remove_dumps_from_filesystem() {
    bool rc { false };
    for (auto crash_dump_path : buddy::puppies::DOCKS | std::views::transform(buddy::puppies::get_crash_dump_path)) {
        if (crash_dump_path && remove(*crash_dump_path) == 0) {
            rc = true;
        }
    }
    return rc;
}
}; // namespace buddy::puppies::crash_dump
