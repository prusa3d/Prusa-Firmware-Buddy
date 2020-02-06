// mapfile.h
#ifndef _MAPFILE_H
#define _MAPFILE_H

#include <inttypes.h>
#include <stdio.h>



typedef struct _mapfile_t
{
} mapfile_t;

typedef struct _mapfile_mem_entry_t
{
	uint32_t addr;
	uint32_t size;
	void* ptr;
} mapfile_mem_entry_t;

typedef struct _mapfile_entry_t
{
	char* name;
	uint32_t addr;
	uint32_t size;
} mapfile_entry_t;


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void mapfile_free(mapfile_t* pm);

extern mapfile_t* mapfile_load(const char* fn);

//extern uint32_t mapfile_get_symbol(mapfile_t* pm, const char* sn);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MAPFILE_H
