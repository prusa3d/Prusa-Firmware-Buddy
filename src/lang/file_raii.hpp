#pragma once

#include <stdio.h>

class FileRAII {
public:
    FileRAII(FILE *file)
        : m_File(file) {}

    ~FileRAII() {
        if (m_File) {
            fclose(m_File);
        }
    }

    void Release() {
        m_File = nullptr;
    }

private:
    FILE *m_File;
};
