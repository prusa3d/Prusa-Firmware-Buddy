/// @file
#include <resources/tarball.hpp>
#include <resources/tarball_internal.hpp>

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <logging/log.hpp>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(Resources);

namespace buddy::resources::tarball {

[[nodiscard]] static bool log_io_error(const char *op) {
    log_error(Resources, "%s() failed: %i", op, errno);
    return false;
}

[[nodiscard]] static bool log_malformed_tar() {
    log_error(Resources, "Malformed tar");
    return false;
}

// Parse a space/NUL padded octal field. Returns false on a stray character.
bool parse_octal(const uint8_t *field, size_t len, uint32_t &out) {
    uint32_t value = 0;
    size_t i = 0;
    while (i < len && (field[i] == ' ' || field[i] == '\0')) {
        i++; // leading padding
    }
    for (; i < len; i++) {
        uint8_t c = field[i];
        if (c == ' ' || c == '\0') {
            break; // trailing padding
        }
        if (c < '0' || c > '7') {
            return false;
        }
        value = (value << 3) | (c - '0');
    }
    out = value;
    return true;
}

bool checksum_ok(const uint8_t *block) {
    // Sum of all header bytes with the checksum field taken as spaces.
    uint32_t sum = 0;
    for (size_t i = 0; i < block_size; i++) {
        sum += (i >= field_offset_chksum && i < field_offset_chksum + field_length_chksum) ? ' ' : block[i];
    }
    uint32_t stored;
    if (!parse_octal(block + field_offset_chksum, field_length_chksum, stored)) {
        return false;
    }
    return sum == stored;
}

// Round `n` up to a whole tar block. The block size is a power of two, so this is a bitmask.
size_t padded_to_block(size_t n) {
    return (n + block_size - 1) & ~(block_size - 1);
}

// Validate the member name and build its absolute destination path, rewritten in place in the
// header block. The name must fit the 100-byte name field (empty ustar prefix), be rooted
// ('/'), and have no ".." component; it is NUL-terminated and `root` is prepended. For a
// directory the ustar trailing slash is dropped. Returns the path, or nullptr if the name is
// unsafe.
const char *build_path(std::span<uint8_t> header, const char *root, bool is_dir) {
    // `name` points into the header block (not NUL-terminated when the name fills the
    // 100-byte field); `name_len` is its length within that field.
    char *const name = reinterpret_cast<char *>(header.data() + field_offset_name);
    const size_t name_len = strnlen(name, field_length_name);

    // NUL-terminate the name in place. The following byte is a NUL or the unused mode field.
    name[name_len] = '\0';

    // Validate the name before it is trusted -- this runs before the payload's digest is
    // verified. A non-empty prefix field means the name spilled past the 100-byte name field
    // (the build keeps names <= 100 bytes), and the name must be rooted with a leading '/'
    // that `root` is concatenated onto.
    if (header[field_offset_prefix] != '\0' || name[0] != '/') {
        return nullptr;
    }

    // Reject any ".." component so a hostile archive can't escape root.
    for (const char *p = name; *p != '\0';) {
        const char *slash = strchr(p, '/');
        const size_t len = slash ? static_cast<size_t>(slash - p) : strlen(p);
        if (len == 2 && p[0] == '.' && p[1] == '.') {
            return nullptr;
        }
        if (slash == nullptr) {
            break;
        }
        p = slash + 1;
    }

    // Prepend `root` in place; the name's leading '/' is the separator. All header fields are
    // already consumed by now, so the whole 512-byte block is free scratch -- the result only
    // has to fit within it (root_len + name_len + NUL <= block_size).
    const size_t root_len = strlen(root);
    memmove(name + root_len, name, name_len);
    memcpy(name, root, root_len);
    name[root_len + name_len] = '\0';

    if (is_dir) {
        // Drop the ustar trailing slash so the path names the directory itself.
        name[root_len + name_len - 1] = '\0';
    }
    return name;
}

struct TarballExtractor {
    // Inputs.
    int fd;
    uint32_t length;
    std::span<uint8_t> scratch_buffer;
    const ExtractDataHook &on_data;

    // State.
    uint32_t remaining; //< payload bytes not yet read
    int out_fd = -1; //< file to be written, when -1, we process the header.
    uint32_t out_remaining = 0; //< remaining bytes to be written to out_fd
    std::span<uint8_t> input_buffer; //< not-yet-consumed tail of the current batch in scratch_buffer

    static constexpr char root[] = "/internal/res";

    TarballExtractor(int fd, uint32_t length, std::span<uint8_t> scratch_buffer, const ExtractDataHook &on_data)
        : fd(fd)
        , length(length)
        , scratch_buffer(scratch_buffer)
        , on_data(on_data)
        , remaining(length) {}

    [[nodiscard]] bool extract() {
        if (length % block_size != 0) {
            return log_malformed_tar();
        }
        if (scratch_buffer.empty() || scratch_buffer.size() % block_size != 0) {
            log_error(Resources, "Tar scratch buffer size %lu is not a non-zero multiple of the block size", static_cast<unsigned long>(scratch_buffer.size()));
            return false;
        }

        if (mkdir(root, 0700) != 0) {
            return log_io_error("mkdir");
        }

        const bool success = iterate_blocks();

        // A file still open here means the loop broke mid-copy: a failure.
        if (out_fd != -1) {
            ::close(out_fd);
            return false;
        }

        return success;
    }

private:
    [[nodiscard]] bool iterate_blocks() {
        for (;;) {
            if (input_buffer.empty()) {
                if (remaining == 0) {
                    return true;
                }
                if (!refill()) {
                    return false;
                }
            }
            if (out_fd == -1) {
                if (!process_header()) {
                    return false;
                }
            } else {
                if (!process_content()) {
                    return false;
                }
            }
        }
    }

    [[nodiscard]] bool refill() {
        debug_assert(input_buffer.empty());
        debug_assert(remaining);

        const size_t batch = std::min<size_t>(remaining, scratch_buffer.size());
        const ssize_t r = ::read(fd, scratch_buffer.data(), batch);
        if (r != static_cast<ssize_t>(batch)) {
            return log_io_error("read");
        }
        input_buffer = scratch_buffer.first(batch);
        remaining -= static_cast<uint32_t>(batch);

        on_data(input_buffer);
        return true;
    }

    [[nodiscard]] bool process_header() {
        debug_assert(!input_buffer.empty());
        debug_assert(out_fd == -1);

        // consume header
        const std::span<uint8_t> header = input_buffer.first(block_size);
        input_buffer = input_buffer.subspan(block_size);

        // A block that isn't a valid ustar header -- the zero-filled end-of-archive terminator,
        // trailing zeros, or a bad checksum -- is skipped; keep hashing until the payload runs out.
        const uint8_t *const block = header.data();
        const uint8_t typeflag = block[field_offset_typeflag];
        // Match the "ustar" prefix only (5 chars), tolerating both the POSIX NUL and GNU space
        // that follow it within the field.
        if (memcmp(block + field_offset_magic, "ustar", 5) != 0
            || !checksum_ok(block)) {
            return true;
        }

        switch (typeflag) {
        case '0':
            return process_file(header);
        case '5':
            return process_directory(header);
        default:
            return log_malformed_tar();
        }
    }

    [[nodiscard]] bool process_directory(std::span<uint8_t> header) {
        const char *const path = build_path(header, root, /*is_dir=*/true);
        if (path == nullptr) {
            return log_malformed_tar();
        }

        log_info(Resources, "Creating directory %s", path);
        if (mkdir(path, 0700) != 0) {
            return log_io_error("mkdir");
        } else {
            return true;
        }
    }

    [[nodiscard]] bool process_file(std::span<uint8_t> header) {
        // Read the size before build_path() rewrites the header buffer for the name.
        uint32_t size = 0;
        if (!parse_octal(header.data() + field_offset_size, field_length_size, size)) {
            return log_malformed_tar();
        }
        const char *const path = build_path(header, root, /*is_dir=*/false);
        if (path == nullptr) {
            return log_malformed_tar();
        }

        log_info(Resources, "Creating file %s", path);
        out_fd = ::open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (out_fd == -1) {
            return log_io_error("open");
        } else {
            out_remaining = size;
            return true;
        }
    }

    [[nodiscard]] bool process_content() {
        debug_assert(!input_buffer.empty());
        debug_assert(out_fd != -1);

        const size_t n = std::min<size_t>(out_remaining, input_buffer.size());
        const ssize_t w = ::write(out_fd, input_buffer.data(), n);
        if (w != static_cast<ssize_t>(n)) {
            return log_io_error("write");
        }
        out_remaining -= n;
        input_buffer = input_buffer.subspan(padded_to_block(n));
        if (out_remaining == 0) {
            if (::close(std::exchange(out_fd, -1)) != 0) {
                return log_io_error("close");
            }
        }
        return true;
    }
};

bool extract(int fd, uint32_t length, std::span<uint8_t> scratch_buffer, const ExtractDataHook &on_data) {
    return TarballExtractor { fd, length, scratch_buffer, on_data }.extract();
}

} // namespace buddy::resources::tarball
