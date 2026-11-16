/**
 * @file unique_file_ptr.hpp
 * @brief file RAII
 */

#pragma once

#include <memory> // std::unique_ptr
#include <stdio.h> // FILE, fclose

class FileDeleter {
public:
    void operator()(FILE *f) {
        fclose(f);
    }
};

using unique_file_ptr = std::unique_ptr<FILE, FileDeleter>;

/**
 * @brief Close the file and report whether the final flush succeeded.
 *
 * FileDeleter throws away the result of fclose, so a stream closed by RAII
 * cannot tell anyone that its last flush failed. For a file small enough to fit
 * in the stdio buffer that flush is the only write which ever reaches the
 * filesystem, and then fwrite/ferror checks alone prove nothing at all.
 *
 * Use this instead of letting the pointer go out of scope whenever a failed
 * write has to be reported to the caller. The file is closed either way; the
 * pointer is consumed.
 */
[[nodiscard]] inline bool fclose_checked(unique_file_ptr file) {
    return file && fclose(file.release()) == 0;
}
