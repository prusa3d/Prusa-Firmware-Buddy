// temporary C API for the lazy dir view
#pragma once
#include <stdint.h>
#include <stdbool.h>

bool LDV_ChangeDir(void *LDV, bool sortByName, const char *path, const char *fname);
uint32_t LDV_TotalFilesCount(void *LDV);
uint32_t LDV_WindowSize(void *LDV);
uint32_t LDV_VisibleFilesCount(void *LDV);
bool LDV_MoveUp(void *LDV);
bool LDV_MoveDown(void *LDV);
const char *LDV_LongFileNameAt(void *LDV, int index, bool *isFile);
const char *LDV_ShortFileNameAt(void *LDV, int index, bool *isFile);
void *LDV_Get(void);
