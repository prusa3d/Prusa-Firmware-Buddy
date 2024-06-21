#pragma once

enum class GCodeReaderResult {
    RESULT_OK,
    RESULT_EOF,
    RESULT_TIMEOUT, // low level USB function might return timeout in case they can't get mutex in time
    RESULT_ERROR,
    RESULT_OUT_OF_RANGE, // Outside of the validity range
    RESULT_CORRUPT, // Corruption / CRC mismatch / ...

    _RESULT_LAST = RESULT_CORRUPT,
};
