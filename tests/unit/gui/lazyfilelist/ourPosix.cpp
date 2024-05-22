#include "ourPosix.hpp"

std::vector<FileEntry> testFiles0;

extern "C" {

size_t strlcpy(char *, const char *, size_t);
}

void MakeLFNSFN(dirent *fno, const char *lfn, size_t fileindex) {
    if (strlen(lfn) >= 13) {
        int charsWritten = snprintf(fno->d_name, sizeof(fno->d_name), "%-.6s~%02u.GCO", lfn, (unsigned)fileindex);
        charsWritten = std::min<int>(charsWritten, sizeof(fno->d_name) - 1);
        strlcpy(fno->d_name + charsWritten + 1, lfn, sizeof(fno->d_name) - charsWritten - 1);
        fno->lfn = fno->d_name + charsWritten + 1;
    } else {
        strlcpy(fno->d_name, lfn, sizeof(fno->d_name));
        fno->lfn = fno->d_name;
    }
}
DIR *opendir(const char *path) {
    DIR *dp = new DIR;
    dp->obj = 0;
    return dp;
}

int closedir(DIR *dp) {
    delete dp;
    return 1;
}

dirent *readdir(DIR *dp) {
    if (dp->obj >= testFiles0.size()) {
        return nullptr;
    } else {
        MakeLFNSFN(&(dp->dirStruct), testFiles0[dp->obj].lfn.c_str(), dp->obj);
        dp->dirStruct.d_type = testFiles0[dp->obj].dir ? DT_DIR : DT_REG;
        dp->dirStruct.time = testFiles0[dp->obj].time;
        ++dp->obj;
    }
    return &(dp->dirStruct);
}
