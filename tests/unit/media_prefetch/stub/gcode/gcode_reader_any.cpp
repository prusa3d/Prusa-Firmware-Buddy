#include "gcode_reader_any.hpp"

#include <catch2/catch.hpp>

#include <test_tools/gcode_provider.hpp>
#include <freertos/mutex.hpp>

static void check_prefetch_mutex_not_locked() {
    // Check that we're not doing anything to the reader while the media prefetch mutex is locked.
    // We're testing everything synchronously, so there is no risk of race conditions
    CHECK(freertos::Mutex::locked_mutex_count == 0);
}

AnyGcodeFormatReader::AnyGcodeFormatReader(const char *filename) {
    check_prefetch_mutex_not_locked();

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

AnyGcodeFormatReader &AnyGcodeFormatReader::operator=(AnyGcodeFormatReader &&o) {
    if (provider) {
        // We would be "destroying" provider here - check that we're not locked
        check_prefetch_mutex_not_locked();
    }

    provider = o.provider;
    o.provider = nullptr;
    return *this;
}

AnyGcodeFormatReader::~AnyGcodeFormatReader() {
    if (provider) {
        check_prefetch_mutex_not_locked();
    }
}
