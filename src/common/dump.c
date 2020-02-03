// dump.c

#include "dump.h"
#include "ff.h"
#include "w25x.h"


#define DUMP_RAM_ADDR    0x20000000
#define DUMP_RAM_SIZE    0x00020000

#define DUMP_CCRAM_ADDR  0x10000000
#define DUMP_CCRAM_SIZE  0x00010000

#define DUMP_OFFSET      0x00000000

#define DUMP_PAGE_SIZE   0x100

#define DUMP_BUFF_SIZE   0x100

#define DUMP_FILE_SIZE   (DUMP_RAM_SIZE + DUMP_CCRAM_SIZE)


void dump_to_xflash(void)
{
	uint32_t addr;
	if (w25x_init())
	{
		w25x_wait_busy();
		w25x_enable_wr();
		for (addr = 0; addr < DUMP_FILE_SIZE; addr += 0x10000)
		{
			w25x_block64_erase(DUMP_OFFSET + addr);
			w25x_wait_busy();
		}
		for (addr = 0; addr < DUMP_RAM_SIZE; addr += DUMP_PAGE_SIZE)
		{
			w25x_page_program(DUMP_OFFSET + addr, (uint8_t*)(DUMP_RAM_ADDR + addr), DUMP_PAGE_SIZE);
			w25x_wait_busy();
		}
		for (addr = 0; addr < DUMP_CCRAM_SIZE; addr += DUMP_PAGE_SIZE)
		{
			w25x_page_program(DUMP_OFFSET + addr, (uint8_t*)(DUMP_CCRAM_ADDR + addr), DUMP_PAGE_SIZE);
			w25x_wait_busy();
		}
		w25x_disable_wr();
	}
}

int dump_save_xflash_to_usb(const char* fn)
{
	FIL fil;
	uint32_t addr;
	uint8_t buff[DUMP_BUFF_SIZE];
	UINT bw;
	if (w25x_init())
	{
		if (f_open(&fil, fn, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
		{
			for (addr = 0; addr < DUMP_FILE_SIZE; addr += DUMP_BUFF_SIZE)
			{
				w25x_rd_data(addr, buff, DUMP_BUFF_SIZE);
				f_write(&fil, buff, DUMP_BUFF_SIZE, &bw);
			}
			f_close(&fil);
			return 1;
		}
	}
	return 0;
}
