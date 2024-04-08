#pragma once

#include "gcode_reader_binary.hpp"
#include "gcode_reader_plaintext.hpp"
#include <variant>

/**
 * @brief Container that can open and read any gcode regardless of what type it is.
 *        Stores Plain/PrusaPack reader inside, so dynamic alocation is not needed.
 *        Also handles destruction & closing of files.
 */
class AnyGcodeFormatReader {
public:
    AnyGcodeFormatReader()
        : ptr(nullptr) {}
    AnyGcodeFormatReader(const char *filename)
        : ptr(nullptr) {
        open(filename);
    }
    AnyGcodeFormatReader(AnyGcodeFormatReader &&other);
    AnyGcodeFormatReader &operator=(AnyGcodeFormatReader &&other);
    ~AnyGcodeFormatReader();

    /**
     * @brief Open specified file
     *
     * @return nullptr if file cannot be opened
     */
    IGcodeReader *open(const char *filename);

    /**
     * @brief Get IGcodeReader that will read data from this gcode
     *
     * @return IGcodeReader*
     */
    inline IGcodeReader *get() {
        assert(ptr);
        return ptr;
    }
    inline IGcodeReader *operator->() {
        assert(ptr);
        return ptr;
    }

    /**
     * @brief Return true if openning was successfull
     *
     * @return true
     * @return false
     */
    bool is_open() const {
        return ptr != nullptr;
    }

    /**
     * @brief Close the file
     */
    void close();

private:
    typedef std::variant<std::monostate, PlainGcodeReader, PrusaPackGcodeReader> storage_type_t;
    storage_type_t storage;
    IGcodeReader *ptr;

    AnyGcodeFormatReader &operator=(const AnyGcodeFormatReader &) = delete;
    AnyGcodeFormatReader(const AnyGcodeFormatReader &) = delete;
};
