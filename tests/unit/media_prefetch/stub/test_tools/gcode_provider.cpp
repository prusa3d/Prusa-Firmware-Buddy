#include "gcode_provider.hpp"

#include <catch2/catch.hpp>
#include <sstream>
#include <numeric>

StubGcodeProviderBase::StubGcodeProviderBase() {
    filename_ = (std::stringstream() << std::hex << std::bit_cast<uintptr_t>(this)).str();
}

StubGcodeProviderBase *StubGcodeProviderBase::from_filename(const char *filename) {
    uintptr_t ptr;
    std::stringstream(filename) >> std::hex >> ptr;
    return std::bit_cast<StubGcodeProviderBase *>(ptr);
}

void StubGcodeProviderMemory::add_line(const std::string &text) {
    data.append(text);
    data.push_back('\n');
}

void StubGcodeProviderMemory::add_breakpoint(GcodeReaderResult result, uint32_t pos) {
    if (pos == static_cast<uint32_t>(-1)) {
        pos = data.size();
    }

    breakpoints.push({ pos, result });
}

GcodeReaderResult StubGcodeProviderMemory::stream_gcode_start(uint32_t offset) {
    pos = offset;
    return GcodeReaderResult::RESULT_OK;
}

GcodeReaderResult StubGcodeProviderMemory::stream_getc(char &ch) {
    if (!breakpoints.empty() && breakpoints.front().first <= pos) {
        const auto r = breakpoints.front().second;
        breakpoints.pop();
        return r;
    }

    if (pos >= data.size()) {
        return GcodeReaderResult::RESULT_EOF;
    }

    ch = data[pos++];
    return GcodeReaderResult::RESULT_OK;
}
