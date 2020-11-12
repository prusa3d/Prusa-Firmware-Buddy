// scratch_buffer.h

#include <inttypes.h>

/// Single place for definition of large RAM space for various tasks.
/// Currently, there's no locking mechanism so before you use it, make sure
/// there's nobody else who uses it.
///
/// This is used by:
/// - PNG decoder
/// - QR code generator
///
/// TODO: use it for
/// - SHA256 encoding

#define SCRATCH_BUFFER_SIZE 49152 ///< ugly but constexpr int does not work

extern uint8_t scratch_buffer[SCRATCH_BUFFER_SIZE];
