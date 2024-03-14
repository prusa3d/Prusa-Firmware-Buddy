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
