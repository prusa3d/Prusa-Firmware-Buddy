#pragma once

#include <variant>

struct GCodeReaderStreamRestoreInfo {

public:
    struct PrusaPackRec {
        uint32_t block_file_pos = 0; //< Of block header in file
        uint32_t block_start_offset = 0; //< Offset of decompressed data on start of the block

        bool operator==(const PrusaPackRec &) const = default;
        bool operator!=(const PrusaPackRec &) const = default;
    };
    using PrusaPack = std::array<PrusaPackRec, 2>;

public:
    bool operator==(const GCodeReaderStreamRestoreInfo &) const = default;
    bool operator!=(const GCodeReaderStreamRestoreInfo &) const = default;

public:
    std::variant<std::monostate, PrusaPack> data;
};

/// Struct containing complete information for resuming streams
struct GCodeReaderPosition {
    GCodeReaderStreamRestoreInfo restore_info;

    /// Position in the file, to be passed into stream_XX_start
    uint32_t offset = 0;
};
