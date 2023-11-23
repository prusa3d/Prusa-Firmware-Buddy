#pragma once

#include "../common/filename_type.hpp"
#include <stdio.h>
#include "dirent.h"
#include <strings.h>

enum class ResType {
    OK,
    NOK,
    NO_FILE
};

/// RAII iterator structure over a directory with file/entry matching
/// tailored for our purposes
struct F_DIR_RAII_Iterator {
    DIR *dp;
    dirent *fno;
    ResType result;
    char *m_Path;
    F_DIR_RAII_Iterator(char *path)
        : m_Path(path) {
        dp = opendir(path);
        if (!dp) {
            result = ResType::NOK;
        } else {
            result = ResType::OK;
        }
    }

    /// @return true if a file/entry matching the requirements was found
    ///         false if there are no more files or an error iterating over a dir occured
    bool FindNext() {
        // and it only does pattern matching, which I don't need here - I have my own
        while ((fno = readdir(dp)) != nullptr) {
            if (EntryAccepted()) {
                return true; // found and accepted
            }
        }
        return false; // an error iterating over the dir
    }

    bool EntryAccepted() const {
        if (fno->lfn[0] == '.') { // ignore hidden files/directories
            return false;
        }
        if ((fno->d_type & DT_DIR) != 0) {
            return true; // all normal directories are accepted
        }
        // files are being filtered by their extension
        return filename_is_printable(fno->lfn);
    }

    ~F_DIR_RAII_Iterator() {
        closedir(dp);
    }
};
/// This is just a simple RAII struct for finding one particular file/dir name
/// and closing the control structures accordingly
struct F_DIR_RAII_Find_One {
    DIR *dp;
    dirent *fno;
    ResType result;
    F_DIR_RAII_Find_One(char *sfnPath, const char *sfn) {
        result = ResType::NO_FILE;
        dp = opendir(sfnPath);
        if (dp) {
            for (;;) {
                fno = readdir(dp);
                if (!fno) {
                    result = ResType::NO_FILE;
                    break;
                }
                if (!strcmp(sfn, fno->d_name)) {
                    result = ResType::OK;
                    break;
                }
            }
        }
    }
    ~F_DIR_RAII_Find_One() {
        closedir(dp);
    }
};
