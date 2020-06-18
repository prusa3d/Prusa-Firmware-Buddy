#include "lazyfilelist-c-api.h"
#include "lazyfilelist.h"

using LDV9 = LazyDirView<9>;

bool LDV_ChangeDir(void *LDV, bool sortByName, const char *path, const char *fname) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    ldv->ChangeDirectory(path,
        sortByName ? LDV9::SortPolicy::BY_NAME : LDV9::SortPolicy::BY_CRMOD_DATETIME,
        fname);
    return true;
}

uint32_t LDV_TotalFilesCount(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->TotalFilesCount();
}

uint32_t LDV_WindowSize(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->WindowSize();
}

uint32_t LDV_VisibleFilesCount(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->VisibleFilesCount();
}

bool LDV_MoveUp(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->MoveUp();
}

bool LDV_MoveDown(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->MoveDown();
}

const char *LDV_LongFileNameAt(void *LDV, int index, bool *isFile) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    auto i = ldv->LongFileNameAt(index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

const char *LDV_ShortFileNameAt(void *LDV, int index, bool *isFile) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    auto i = ldv->ShortFileNameAt(index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

void *LDV_Get(void) {
    static LDV9 ldv;
    return &ldv;
}
