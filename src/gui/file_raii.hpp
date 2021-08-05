#pragma once

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

    static bool FnameMatchesPattern(const char *fname) {
        size_t len = strlen(fname);
        static const char gcode[] = ".gcode";
        static const char gc[] = ".gc";
        static const char g[] = ".g";
        static const char gco[] = ".gco";

        if (!strcasecmp(fname + len - sizeof(gcode) + 1, gcode))
            return true;
        if (!strcasecmp(fname + len - sizeof(gc) + 1, gc))
            return true;
        if (!strcasecmp(fname + len - sizeof(g) + 1, g))
            return true;
        if (!strcasecmp(fname + len - sizeof(gco) + 1, gco))
            return true;
        return false;
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
        if ((fno->d_type & DT_DIR) != 0) {
            return true; // all normal directories are accepted
        }
        // files are being filtered by their extension
        return FnameMatchesPattern(fno->d_name);
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
