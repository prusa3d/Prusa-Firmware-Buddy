#include <puppies/cyphal_flash_files.hpp>

#include <bsod/bsod.h>
#include <buddy/digest.hpp>
#include <bit>
#include <cerrno>
#include <logging/log.hpp>
#include <span>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <utils/uncopyable.hpp>

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using xbuddy_extension::DigestStatus;
using xbuddy_extension::FileId;
using xbuddy_extension::modbus::Digest;
using xbuddy_extension::modbus::DigestRequest;
using xbuddy_extension::modbus::serialize_digest_status;

namespace {

    const char *firmware_file_path(FileId file_id) {
        switch (file_id) {
        case FileId::none:
            break;
        case FileId::firmware_ac_controller:
            return "/internal/res/puppies/fw-ac_controller.bin";
        case FileId::firmware_anfc:
            return "/internal/res/puppies/fw-anfc.bin";
        case FileId::firmware_tool_offset_sensor:
            return "/internal/res/puppies/fw-tool_offset_sensor.bin";
        }
        bsod_unreachable();
    }

    struct ClosingFileDescriptor : Uncopyable {
        int fd;
        explicit ClosingFileDescriptor(int fd)
            : fd(fd) {}
        ~ClosingFileDescriptor() {
            if (fd != -1) {
                (void)close(fd);
            }
        }
    };

} // namespace

int open_firmware_file(FileId file_id) {
    const char *path = firmware_file_path(file_id);
    const int fd = ::open(path, O_RDONLY);
    if (fd == -1) {
        log_error(Puppies, "open(%s) failed %d", path, errno);
    }
    return fd;
}

void compute_digest_response(DigestRequest request, FileId file_id, Digest &modbus_digest) {
    const ClosingFileDescriptor fd { open_firmware_file(file_id) };

    const uint32_t salt = static_cast<uint32_t>(request.salt_hi << 16) | static_cast<uint32_t>(request.salt_lo);
    modbus_digest.request = request;

    DigestStatus digest_status;
    if (fd.fd == -1) {
        modbus_digest.data = {};
        digest_status = DigestStatus::unavailable;
    } else {
        // we defined Digest::data as little endian => no byte swapping needed
        // we also compute the digest in-place and save some stack space
        static_assert(std::endian::native == std::endian::little);
        const auto buddy_digest = std::as_writable_bytes(std::span { modbus_digest.data });
        if (buddy::compute_file_digest(fd.fd, salt, buddy_digest)) {
            digest_status = DigestStatus::ok;
        } else {
            log_error(Puppies, "buddy::compute_file_digest() failed");
            modbus_digest.data = {};
            digest_status = DigestStatus::retry;
        }
    }
    modbus_digest.status = serialize_digest_status(digest_status);
}

} // namespace buddy::puppies
