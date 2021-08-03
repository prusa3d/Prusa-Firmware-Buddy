#pragma once

#include <stdio.h>

class FileRaii {
public:
    FileRaii(FILE *file)
        : m_File(file) {}

    ~FileRaii() {
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
