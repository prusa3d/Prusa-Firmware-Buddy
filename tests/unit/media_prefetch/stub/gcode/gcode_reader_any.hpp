// Unittest stub

#pragma once

#include <cstdint>
#include <stddef.h>

#include <option/has_e2ee_support.h>
#if HAS_E2EE_SUPPORT()
    #include <e2ee/identity_check_levels.hpp>
#endif

#include "gcode_reader_result.hpp"

class StubGcodeProviderBase;

static constexpr size_t MAX_CMD_SIZE = 96;

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
    AnyGcodeFormatReader(const char *filename, bool allow_decryption = false
#if HAS_E2EE_SUPPORT()
        ,
        e2ee::IdentityCheckLevel identity_check_lvl = e2ee::IdentityCheckLevel::AnyIdentity
#endif
    );
    AnyGcodeFormatReader(const AnyGcodeFormatReader &) = delete;
    AnyGcodeFormatReader &operator=(AnyGcodeFormatReader &&);
    ~AnyGcodeFormatReader();

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

    virtual bool valid_for_print(bool);

    bool has_error() { return false; }
    virtual const char *error_str() const { return nullptr; }

private:
    StubGcodeProviderBase *provider = nullptr;
};
