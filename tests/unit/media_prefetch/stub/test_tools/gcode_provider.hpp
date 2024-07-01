#pragma once

#include <array>
#include <string>
#include <queue>
#include <string>
#include <cstdint>
#include <optional>

#include <gcode/gcode_reader_result.hpp>

class StubGcodeProviderBase {

public:
    StubGcodeProviderBase();

    static StubGcodeProviderBase *from_filename(const char *filename);

    const char *filename() {
        return filename_.c_str();
    }

public:
    virtual GCodeReaderResult stream_gcode_start(uint32_t offset) = 0;

    virtual GCodeReaderResult stream_getc(char &ch) = 0;

private:
    std::string filename_;
};

class StubGcodeProviderMemory final : public StubGcodeProviderBase {

public:
    bool has_read_all() const {
        return pos == data.size();
    }

    /// \returns replay position of the added gcode
    uint32_t add_gcode(const std::string &text);

    /// Makes the reader return the given result at the given position
    void add_breakpoint(GCodeReaderResult result, std::optional<uint32_t> pos = std::nullopt);

    size_t breakpoint_count() const {
        return breakpoints.size();
    }

public:
    GCodeReaderResult stream_gcode_start(uint32_t offset) override;

    GCodeReaderResult stream_getc(char &ch) override;

private:
    std::string data;
    std::queue<std::pair<size_t, GCodeReaderResult>> breakpoints;
    size_t pos = 0;
};
