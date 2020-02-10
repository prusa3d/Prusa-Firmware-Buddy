// mapfile.h
#ifndef _MAPFILE_H
#define _MAPFILE_H

#include <inttypes.h>
#include <stdio.h>


typedef enum
{
    mem_type_fill = 0,
    mem_type_text = 1,
    mem_type_data = 2,
    mem_type_bss = 3,
    mem_type_common = 4,
    mem_type_other = 5,
} mapfile_mem_type_t;

typedef struct _mapfile_t
{
} mapfile_t;

typedef struct _mapfile_mem_entry_t
{
    mapfile_mem_type_t type;
    uint32_t addr;
    uint32_t size;
    char* name;
} mapfile_mem_entry_t;


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void mapfile_free(mapfile_t* pm);

extern mapfile_t* mapfile_load(const char* fn);

extern mapfile_mem_entry_t* mapfile_find_mem_entry(mapfile_t* pm, const char* name);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MAPFILE_H
