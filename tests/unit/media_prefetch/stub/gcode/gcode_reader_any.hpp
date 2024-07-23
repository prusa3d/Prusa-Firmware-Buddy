// Unittest stub

#pragma once

#include <cstdint>
#include <stddef.h>

#include "gcode_reader_result.hpp"

class StubGcodeProviderBase;

static constexpr size_t MAX_CMD_SIZE = 96;
static constexpr size_t FILE_PATH_BUFFER_LEN = 64;

struct GCodeReaderStreamRestoreInfo {
public:
    bool operator==(const GCodeReaderStreamRestoreInfo &) const = default;
    bool operator!=(const GCodeReaderStreamRestoreInfo &) const = default;
};

/// Struct containing complete information for resuming streams
struct GCodeReaderPosition {
    GCodeReaderStreamRestoreInfo restore_info;

    /// Position in the file, to be passed into stream_XX_start
    uint32_t offset = 0;
};

class AnyGcodeFormatReader;
using IGcodeReader = AnyGcodeFormatReader;

class AnyGcodeFormatReader {
public:
    using Result_t = GCodeReaderResult;

    AnyGcodeFormatReader() = default;
    AnyGcodeFormatReader(const char *filename);
    AnyGcodeFormatReader(const AnyGcodeFormatReader &) = default;
    AnyGcodeFormatReader &operator=(const AnyGcodeFormatReader &) = default;

    IGcodeReader *operator->() { return this; }

    bool is_open() const {
        return provider;
    }

    void set_restore_info(const GCodeReaderStreamRestoreInfo &) {}
    GCodeReaderStreamRestoreInfo get_restore_info() { return {}; }

    Result_t stream_gcode_start(uint32_t offset);

    Result_t stream_getc(char &ch);

    uint32_t get_gcode_stream_size_estimate() const { return 0; }

    void update_validity(const char *) {}

private:
    StubGcodeProviderBase *provider = nullptr;
};
