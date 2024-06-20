#include "gcode_reader_any.hpp"

#include <catch2/catch.hpp>

#include <test_tools/gcode_provider.hpp>

AnyGcodeFormatReader::AnyGcodeFormatReader(const char *filename) {
    provider = StubGcodeProviderBase::from_filename(filename);
}

AnyGcodeFormatReader::Result_t AnyGcodeFormatReader::stream_gcode_start(uint32_t offset) {
    REQUIRE(provider);
    return provider->stream_gcode_start(offset);
}

AnyGcodeFormatReader::Result_t AnyGcodeFormatReader::stream_getc(char &ch) {
    REQUIRE(provider);
    return provider->stream_getc(ch);
}
