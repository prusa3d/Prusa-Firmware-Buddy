#include <puppies/cyphal_flash_host.hpp>

#include <bit>
#include <cerrno>
#include <cinttypes>
#include <logging/log.hpp>
#include <puppies/cyphal_flash_files.hpp>
#include <span>
#include <sys/unistd.h>

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using FileId = xbuddy_extension::FileId;

void CyphalBridgeFlashHost::set_requests(
    xbuddy_extension::modbus::ChunkRequest chunk,
    xbuddy_extension::modbus::DigestRequest digest) {

    current_chunk_request = chunk;
    current_digest_request = digest;
    // Publish validity only after the request fields, never a stale value.
    valid = true;
}

uint8_t CyphalBridgeFlashHost::progress_percent() const {
    if (flash_file_size == 0) {
        return 0;
    }

    const uint32_t flash_file_offset = static_cast<uint32_t>(last_chunk_request.offset_hi << 16) | static_cast<uint32_t>(last_chunk_request.offset_lo);
    return 100 * flash_file_offset / flash_file_size;
}

void CyphalBridgeFlashHost::close_flash_file() {
    if (flash_fd != -1) {
        close(flash_fd);
        flash_fd = -1;
        flash_file_size = 0;
    }
}

CommunicationStatus CyphalBridgeFlashHost::write_chunk(PuppyModbus &bus, modbus::ServerAddress server) {
    if (!valid) {
        // We need an up-to-date request. Otherwise, we don't try anything.
        return CommunicationStatus::ERROR;
    }

    const xbuddy_extension::modbus::ChunkRequest current_request = current_chunk_request;

    if (flash_fd != -1 && last_chunk_request.file_id != current_request.file_id) {
        // The current file is outdated (or maybe no request any more).
        // Get rid of this one, maybe create a new one later.
        close_flash_file();
    }

    const FileId file_id = xbuddy_extension::modbus::parse_file_id(current_request.file_id);
    if (file_id == FileId::none) {
        // No request -> we are done.
        return CommunicationStatus::SKIPPED;
    }

    if (flash_fd != -1) {
        if (last_chunk_request == current_request) {
            // We didn't get a newer request yet, this one was already sent, wait for newer one.
            return CommunicationStatus::SKIPPED;
        }
    } else {
        flash_fd = open_firmware_file(file_id);
        if (flash_fd == -1) {
            return CommunicationStatus::SKIPPED;
        }
        // Cache the file size when opening
        const off_t lseek_result = lseek(flash_fd, 0, SEEK_END);
        if (lseek_result == -1) {
            log_error(Puppies, "lseek() failed %d", errno);
            close_flash_file();
            return CommunicationStatus::SKIPPED;
        }
        flash_file_size = lseek_result;
        last_chunk_request.file_id = current_request.file_id;
    }

    const uint32_t chunk_offset = static_cast<uint32_t>(current_request.offset_hi << 16) | static_cast<uint32_t>(current_request.offset_lo);
    if (lseek(flash_fd, chunk_offset, SEEK_SET) == -1) {
        log_error(Puppies, "lseek() failed %d", errno);
        close_flash_file();
        return CommunicationStatus::ERROR;
    }

    xbuddy_extension::modbus::Chunk modbus_chunk;
    modbus_chunk.request = current_request;

    // we defined Chunk::data as little endian => no byte swapping needed
    // we also read the chunk in-place and save some stack space
    static_assert(std::endian::native == std::endian::little);
    const auto chunk_buffer = std::as_writable_bytes(std::span { modbus_chunk.data });
    const size_t chunk_size = chunk_buffer.size();

    size_t cummulative_read = 0;
    // Deal with read being able to do short reads - we promise the other side
    // we'll give it full-sized chunks (unless it's the last one).
    while (cummulative_read < chunk_size) {
        const ssize_t nread = read(flash_fd, chunk_buffer.data() + cummulative_read, chunk_size - cummulative_read);
        if (nread == 0) {
            // EOF -> terminate, send whatever we have.
            break;
        } else if (nread == -1) {
            log_error(Puppies, "read() failed %d", errno);
            close_flash_file();
            return CommunicationStatus::ERROR;
        } else {
            cummulative_read += nread;
        }
    }

    modbus_chunk.size = cummulative_read;

    if (bus.write_holding_registers(server, modbus_chunk)) {
        log_debug(Puppies, "sent chunk offset %" PRIu32 " size %zu", chunk_offset, cummulative_read);
        last_chunk_request = current_request;
        return CommunicationStatus::OK;
    }
    return CommunicationStatus::ERROR;
}

CommunicationStatus CyphalBridgeFlashHost::write_digest(PuppyModbus &bus, modbus::ServerAddress server, DigestComputeFn compute) {
    const xbuddy_extension::modbus::DigestRequest current_request = current_digest_request;

    if (current_request == last_digest_request) {
        return CommunicationStatus::SKIPPED;
    }

    const FileId file_id = xbuddy_extension::modbus::parse_file_id(current_request.file_id);
    if (file_id == FileId::none) {
        // Nothing was requested
        last_digest_request = current_request;
        return CommunicationStatus::SKIPPED;
    }

    // Callback runs the slow work with the mutex released and reacquires
    // before returning.
    xbuddy_extension::modbus::Digest modbus_digest;
    compute(current_request, file_id, modbus_digest);

    if (bus.write_holding_registers(server, modbus_digest)) {
        last_digest_request = current_request;
        return CommunicationStatus::OK;
    } else {
        // Best effort — retry on next cycle (last_digest_request not updated,
        // so dedup won't suppress it).
        return CommunicationStatus::SKIPPED;
    }
}

} // namespace buddy::puppies
