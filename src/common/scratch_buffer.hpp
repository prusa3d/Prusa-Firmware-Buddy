#pragma once
#include <inttypes.h>
#include <stddef.h>

namespace buddy::scratch_buffer {

/// Single place for definition of large RAM space for various tasks.
///
/// This is used by:
/// - PNG decoder
/// - QR code generator
/// - Bootloader Update
///
/// TODO: use it for
/// - SHA256 encoding

struct ScratchBuffer {
    uint8_t buffer[49152];

    size_t size() const {
        return sizeof(buffer);
    }
};

class Ownership {
private:
    bool acquired;

public:
    Ownership();

    bool acquire(bool wait);
    ScratchBuffer &get();
    void release();

    ~Ownership();
};

};
