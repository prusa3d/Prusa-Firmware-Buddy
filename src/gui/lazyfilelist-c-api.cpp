#include "lazyfilelist-c-api.h"
#include "lazyfilelist.h"

using LDV9 = LazyDirView<9>;

extern "C" bool LDV_ChangeDir(void *LDV, bool sortByName, const char *path) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    ldv->ChangeDirectory(path,
        sortByName ? LDV9::SortPolicy::BY_NAME : LDV9::SortPolicy::BY_CRMOD_DATETIME,
        nullptr);
    return true;
}

extern "C" uint32_t LDV_TotalFilesCount(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->TotalFilesCount();
}

extern "C" uint32_t LDV_VisibleFilesCount(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->VisibleFilesCount();
}

extern "C" bool LDV_MoveUp(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->MoveUp();
}

extern "C" bool LDV_MoveDown(void *LDV) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    return ldv->MoveDown();
}

extern "C" const char *LDV_FileAt(void *LDV, int index, bool *isFile) {
    LDV9 *ldv = reinterpret_cast<LDV9 *>(LDV);
    auto i = ldv->FileNameAt(index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

extern "C" void *LDV_Get(void) {
    static LDV9 ldv;
    return &ldv;
}
