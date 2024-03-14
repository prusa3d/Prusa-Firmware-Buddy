#pragma once

#include <cstdlib>

// Hostname compression. We don't want to allocate bigger chunk of eeprom, but
// still need to store some testing domains that are longer. Therefore, we
// compress the "known suffix" of it. There's a possibility to have more such
// known suffixes in the future.
//
// (It is still possible to have non-compressed custom ones as before, they
// still need to fit into the eeprom variable though).

namespace connect_client {

// Copies the host into the provided buffer, applying compression if possible.
//
// Returns false if it doesn't fit into the buffer (after any applicable
// compression).
//
// The resulting value is \0-terminated.
bool compress_host(const char *host, char *buffer, size_t buffer_len);

// The reverse of compress_host. Expands it in-place.
//
// In case the value is not compressed, is invalid or the expanded value
// wouldn't fit, it leaves the original intact.
void decompress_host(char *host, size_t host_buffer);

} // namespace connect_client
