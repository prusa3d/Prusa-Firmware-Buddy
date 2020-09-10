// mapfile.h
#pragma once

#include <inttypes.h>
#include <stdio.h>

typedef enum {
    mem_type_fill = 0,
    mem_type_text = 1,
    mem_type_data = 2,
    mem_type_rodata = 3,
    mem_type_bss = 4,
    mem_type_common = 5,
    mem_type_other = 6,
} mapfile_mem_type_t;

typedef struct _mapfile_t {
} mapfile_t;

typedef struct _mapfile_mem_entry_t {
    mapfile_mem_type_t type;
    uint32_t addr;
    uint32_t size;
    char *name;
} mapfile_mem_entry_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void mapfile_free(mapfile_t *pm);

extern mapfile_t *mapfile_load(const char *fn);

extern mapfile_mem_entry_t *mapfile_find_mem_entry_by_name(mapfile_t *pm, const char *name);

extern mapfile_mem_entry_t *mapfile_find_mem_entry_by_addr(mapfile_t *pm, uint32_t addr);

#ifdef __cplusplus
}
#endif //__cplusplus
