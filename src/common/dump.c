// dump.c

#include "dump.h"
#include "ff.h"
#include "w25x.h"


#define RAM_ADDR    0x20000000
#define RAM_SIZE    0x00020000

#define CCRAM_ADDR  0x10000000
#define CCRAM_SIZE  0x00010000


void dump_to_xflash(void)
{
}

int dump_save_xflash_to_usb(const char* fn)
{
	return 1;
}
