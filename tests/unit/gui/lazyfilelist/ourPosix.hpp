#pragma once
#include "../../../../src/gui/file_list_defs.h"
#include <string.h>
#include <string>
#include <vector>
#include <cstdint>

// needed to not include dirent from system libraries
// if there is problem with multiple definitions, check dirent.h for its define
#ifndef _dirent_h_
    #ifndef _DIRENT_H
        #define _DIRENT_H 1
        #define _dirent_h_

struct FileEntry {
    std::string lfn;
    uint64_t time;
    bool dir;
};

extern std::vector<FileEntry> testFiles0;

        #define DT_UNKNOWN 0
        #define DT_FIFO    1
        #define DT_CHR     2
        #define DT_DIR     4
        #define DT_BLK     6
        #define DT_REG     8
        #define DT_LNK     10
        #define DT_SOCK    12
        #define DT_WHT     14

        #define FF_USE_LFN 1
        #define FF_SFN_BUF 13
        #define FF_LFN_BUF 96

struct dirent {
    unsigned char d_type;
    char d_name[NAME_MAX + 1];
    char *lfn;
    uint64_t time;
};

struct DIR {
    dirent dirStruct;
    size_t obj;
};

void MakeLFNSFN(dirent *fno, const char *lfn, size_t fileindex);

int closedir(DIR *dp);

dirent *readdir(DIR *dp);

DIR *opendir(const char *path);

    #endif
#endif
