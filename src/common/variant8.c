// variant8.c

#include "variant8.h"
#include <stdlib.h>
#include <string.h>


variant8_t variant8_init(uint8_t type, uint16_t count, void* pdata)
{
	variant8_t var8;
	uint16_t size;
	if ((count == 1) && !(type & VARIANT8_PTR))
	{
		var8 = (variant8_t){ type, 0, { 0 }, { 0 } };
		if (pdata)
			switch (type) {
			case VARIANT8_I8:   var8.i8   = *((int8_t*)pdata); break;
			case VARIANT8_UI8:  var8.ui8  = *((uint8_t*)pdata); break;
			case VARIANT8_I16:  var8.i16  = *((int16_t*)pdata); break;
			case VARIANT8_UI16: var8.ui16 = *((uint16_t*)pdata); break;
			case VARIANT8_I32:  var8.i32  = *((int32_t*)pdata); break;
			case VARIANT8_UI32: var8.ui32 = *((uint32_t*)pdata); break;
			case VARIANT8_FLT:  var8.flt  = *((float*)pdata); break;
			}
	}
	else if ((count > 1) && (type & VARIANT8_PTR))
	{
		size = variant8_type_size(type & ~VARIANT8_PTR) * count;
		var8 = (variant8_t){ type, 0, { .size = size }, { .ptr = 0 } };
		if (size)
		{
			var8.ptr = variant8_malloc(size);
			if (pdata)
				memcpy(var8.ptr, pdata, size);
		}
	}
	else
	{
		var8 = (variant8_t){ VARIANT8_EMPTY, 0, { 0 }, { 0 } };
		//TODO: error
	}
	return var8;
}

void variant8_done(variant8_t* pvar8) {
	if (pvar8) {
		if (pvar8->type & VARIANT8_PTR) {
			if (pvar8->size && pvar8->ptr)
				variant8_free(pvar8->ptr);
		}
		*pvar8 = (variant8_t){ VARIANT8_EMPTY, 0, { 0 }, { 0 } };
	}
}

variant8_t variant8_copy(variant8_t* pvar8)
{
	variant8_t var8 = (variant8_t){ VARIANT8_EMPTY, 0, { 0 }, { 0 } };
/*	if ((var8.type & VARIANT8_PTR) && var8.size && var8.ptr) {
		void* ptr = var8.ptr;
		var8.ptr = variant8_malloc(var8.size);
		memcpy(var8.ptr, ptr, var8.size);
	}*/
	return var8;
}


variant8_t variant8_empty(void) { return (variant8_t){ VARIANT8_EMPTY, 0, { 0 }, { 0 } }; }
variant8_t variant8_i8(int8_t i8) { return (variant8_t){ VARIANT8_I8, 0, { 0 }, { .i8 = i8 } }; }
variant8_t variant8_ui8(uint8_t ui8) { return (variant8_t){ VARIANT8_UI8, 0, { 0 }, { .ui8 = ui8 } }; }
variant8_t variant8_i16(int16_t i16) { return (variant8_t){ VARIANT8_I16, 0, { 0 }, { .i16 = i16 } }; }
variant8_t variant8_ui16(uint16_t ui16) { return (variant8_t){ VARIANT8_UI16, 0, { 0 }, { .ui16 = ui16 } }; }
variant8_t variant8_i32(int32_t i32) { return (variant8_t){ VARIANT8_I32, 0, { 0 }, { .i32 = i32 } }; }
variant8_t variant8_ui32(uint32_t ui32) { return (variant8_t){ VARIANT8_UI32, 0, { 0 }, { .ui32 = ui32 } }; }
variant8_t variant8_flt(float flt) { return (variant8_t){ VARIANT8_FLT, 0, { 0 }, { .flt = flt} }; }

variant8_t variant8_pchar(const char* pch) { return (variant8_t){ VARIANT8_PCHAR, 0, { 0 }, { .pch = (char*)pch } }; }

variant8_t variant8_user(uint32_t usr32, uint16_t usr16, uint8_t usr8) {
	return (variant8_t){ VARIANT8_USER, usr8, { .usr16 = usr16 }, { .usr32 = usr32 } };
}

uint16_t variant8_type_size(uint8_t type)
{
	switch (type) {
	case VARIANT8_I8:
	case VARIANT8_UI8:
	case VARIANT8_CHAR:
		return 1;
	case VARIANT8_I16:
	case VARIANT8_UI16:
		return 2;
	case VARIANT8_I32:
	case VARIANT8_UI32:
	case VARIANT8_FLT:
		return 4;
	case VARIANT8_USER:
		return 7;
	}
	return 0;
}

uint16_t variant8_data_size(variant8_t* pvar8)
{
	if (pvar8) {
        if (pvar8->type & VARIANT8_PTR)
        	return pvar8->size;
        else
        	return variant8_type_size(pvar8->type);
	}
	return 0;
}

void* variant8_data_ptr(variant8_t* pvar8)
{
	if (pvar8) {
        if (pvar8->type & VARIANT8_PTR)
        	return pvar8->ptr;
        else
			switch (pvar8->type) {
			case VARIANT8_I8:   return &(pvar8->i8);
			case VARIANT8_UI8:  return &(pvar8->ui8);
			case VARIANT8_CHAR: return &(pvar8->ch);
			case VARIANT8_I16:  return &(pvar8->i16);
			case VARIANT8_UI16: return &(pvar8->ui16);
			case VARIANT8_I32:  return &(pvar8->i32);
			case VARIANT8_UI32: return &(pvar8->ui32);
			case VARIANT8_FLT:  return &(pvar8->flt);
			case VARIANT8_USER: return &(pvar8->usr8);
			}
	}
	return 0;
}

void* variant8_malloc(uint16_t size) { return size?malloc(size):0; }

void variant8_free(void *ptr) { if (ptr) free(ptr); }
