/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <buddy/filename_defs.hpp>

/// A file's name(s): the canonical name (stable, used for filesystem operations
/// and protocols) and the display name (shown in the UI). On FAT these are the
/// short (SFN) and long (LFN) names. On a single-name filesystem they are
/// identical and share storage. Mirrors the packing of struct dirent.
class Filename {
public:
    Filename() = default;

    // If display is nullptr or equal to canonical, it aliases canonical (no second copy is stored).
    Filename(const char *canonical, const char *display = nullptr) {
        assign(canonical, display);
    }

    /// Neither pointer may point into this Filename's own storage: the
    /// canonical copy overwrites the buffer before display is read.
    void assign(const char *canonical, const char *display = nullptr) {
        if (!canonical) {
            canonical = "";
        }
        if (!display || display == canonical || strcmp(display, canonical) == 0) {
            copy_field(buf, canonical, sizeof(buf));
            display_off = 0;
            return;
        }

        // Cap the canonical field so the display always gets its full budget;
        // an overlong canonical would otherwise silently truncate the display
        // (and change how the entry sorts).
        size_t clen = copy_field(buf, canonical, filename_defs::short_filename_buffer_size);
        size_t off = clen + 1;
        copy_field(buf + off, display, sizeof(buf) - off);
        display_off = static_cast<uint8_t>(off);
    }

    const char *canonical() const { return buf; }
    const char *display() const { return buf + display_off; }
    bool empty() const { return buf[0] == '\0'; }

private:
    static size_t copy_field(char *dst, const char *src, size_t cap) {
        size_t n = strlen(src);
        if (n > cap - 1) {
            n = cap - 1;
        }
        memcpy(dst, src, n);
        dst[n] = '\0';
        return n;
    }

    // Holds "canonical\0display\0", or just "canonical\0" when display aliases it.
    static constexpr size_t buffer_size = filename_defs::short_filename_buffer_size + filename_defs::filename_buffer_size;
    static_assert(buffer_size <= 256, "display_off is uint8_t");
    char buf[buffer_size] = { '\0' };
    uint8_t display_off = 0;
};
