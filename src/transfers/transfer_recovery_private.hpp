#pragma once

// On-disk layout of transfer recovery backup files, exposed only so unit
// tests can craft and tamper with backup files. Not part of the public
// transfer API.

#include "transfer.hpp"

#include <option/websocket.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>

namespace transfers {

struct SerializedString {
    long offset = 0;
    size_t size = 0;

    static SerializedString serialize(const char *str, FILE *file, bool &error) {
        const size_t len = strlen(str) + 1;
        if (fwrite(str, len, 1, file) != 1) {
            error = true;
        }
        return SerializedString { ftell(file) - static_cast<long>(len), len };
    }

    template <typename T>
    const char *deserialize(T get_data_ptr) const {
        const char *ptr = static_cast<const char *>(get_data_ptr(offset, size));
        // size must bound a NUL-terminated string, not just an in-range pointer.
        if (ptr == nullptr || size == 0 || ptr[size - 1] != '\0') {
            return nullptr;
        }
        return ptr;
    }
};

struct SerializedTransfer {
    // what data are valid
    PartialFile::State partial_file_state;

    // crc of the partial file state only
    uint32_t partial_file_state_crc;

    // for compatibility check
    SerializedString fw_version;

    // basic url info
    SerializedString host;
    uint16_t port = 0;
    SerializedString url_path_or_hash;
    // Is this an inline transfer (running inside a websocket?)
    //
    // In such case, we use hash instead of url path, but we add the team_id below.
    bool is_inline;
    uint64_t team_id = 0;
    uint32_t orig_size = 0;

    TransferId transfer_id;

#if !WEBSOCKET()
    // encryption info
    std::optional<Download::EncryptionInfo> encryption_info;
#endif

    // raw data referenced by the above structure continue after this
    // at the end of the raw data, there is a crc32 of the range [fw_version, end_of_file)
};

} // namespace transfers
