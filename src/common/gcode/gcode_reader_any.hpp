#pragma once

#include "gcode_reader_binary.hpp"
#include "gcode_reader_closed.hpp"
#include "gcode_reader_plaintext.hpp"
#include <variant>

/**
 * @brief Container that can open and read any gcode regardless of what type it is.
 *        Stores Plain/PrusaPack reader inside, so dynamic alocation is not needed.
 *        Also handles destruction & closing of files.
 */
class AnyGcodeFormatReader {
public:
    AnyGcodeFormatReader();
    AnyGcodeFormatReader(const char *filename);
    AnyGcodeFormatReader(const AnyGcodeFormatReader &) = delete;
    AnyGcodeFormatReader &operator=(const AnyGcodeFormatReader &) = delete;
    AnyGcodeFormatReader(AnyGcodeFormatReader &&);
    AnyGcodeFormatReader &operator=(AnyGcodeFormatReader &&);
    ~AnyGcodeFormatReader();

    IGcodeReader *get(); // never returns nullptr
    IGcodeReader &operator*() { return *get(); }
    IGcodeReader *operator->() { return get(); }

    /**
     * @brief Return true if openning was successfull
     *
     * @return true
     * @return false
     */
    bool is_open() const;

private:
    typedef std::variant<ClosedReader, PlainGcodeReader, PrusaPackGcodeReader> storage_type_t;
    storage_type_t storage;
};
