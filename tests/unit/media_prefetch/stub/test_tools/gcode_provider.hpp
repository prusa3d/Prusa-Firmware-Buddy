#pragma once

#include <array>
#include <string>
#include <queue>
#include <string>
#include <cstdint>

#include <gcode/gcode_reader_result.hpp>

class StubGcodeProviderBase {

public:
    StubGcodeProviderBase();

    static StubGcodeProviderBase *from_filename(const char *filename);

    const char *filename() {
        return filename_.data();
    }

public:
    virtual GcodeReaderResult stream_gcode_start(uint32_t offset) = 0;

    virtual GcodeReaderResult stream_getc(char &ch) = 0;

private:
    std::array<char, 10> filename_;
};

class StubGcodeProviderMemory final : public StubGcodeProviderBase {

public:
    void add_line(const std::string &text);

    /// Makes the reader return the given result at the current position
    void add_result(GcodeReaderResult result);

    bool has_read_all() const {
        return pos == data.size();
    }

public:
    GcodeReaderResult stream_gcode_start(uint32_t offset) override;

    GcodeReaderResult stream_getc(char &ch) override;

private:
    std::string data;
    std::queue<std::pair<size_t, GcodeReaderResult>> results;
    size_t pos = 0;
};
