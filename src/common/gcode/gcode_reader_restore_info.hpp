#pragma once

#include <variant>

struct GCodeReaderStreamRestoreInfo {

public:
    struct __attribute__((packed)) PrusaPackRec {
        uint32_t block_file_pos = 0; //< Of block header in file
        uint32_t block_start_offset = 0; //< Offset of decompressed data on start of the block
    };
    using PrusaPack = std::array<PrusaPackRec, 2>;

public:
    std::variant<std::monostate, PrusaPack> data;
};

// Ugly workardoung to allow making power panic flash data packed
struct __attribute__((packed)) GCodeReaderStreamRestoreInfoSerialized {
    GCodeReaderStreamRestoreInfoSerialized() = default;
    GCodeReaderStreamRestoreInfoSerialized(const GCodeReaderStreamRestoreInfo &data) {
        *this = data;
    }

    GCodeReaderStreamRestoreInfoSerialized &operator=(const GCodeReaderStreamRestoreInfoSerialized &) = default;
    GCodeReaderStreamRestoreInfoSerialized &operator=(const GCodeReaderStreamRestoreInfo &data) {
        memcpy(this->data, &data, sizeof(GCodeReaderStreamRestoreInfo));
        return *this;
    }

    GCodeReaderStreamRestoreInfo deserialize() const {
        GCodeReaderStreamRestoreInfo r;
        memcpy(&r, data, sizeof(GCodeReaderStreamRestoreInfo));
        return r;
    }

    uint8_t data[sizeof(GCodeReaderStreamRestoreInfo)];
};
