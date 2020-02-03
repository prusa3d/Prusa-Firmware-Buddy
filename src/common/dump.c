// dump.c

#include "dump.h"
#include "ff.h"
#include "w25x.h"


#define DUMP_OFFSET      0x00000000

#define DUMP_PAGE_SIZE   0x100

#define DUMP_BUFF_SIZE   0x100

#define DUMP_FILE_SIZE   (DUMP_RAM_SIZE + DUMP_CCRAM_SIZE)


void dump_to_xflash(void)
{
//	uint8_t buff[DUMP_BUFF_SIZE];
	uint32_t addr;
	if (w25x_init())
	{
		for (addr = 0; addr < DUMP_FILE_SIZE; addr += 0x10000)
		{
			w25x_wait_busy();
			w25x_enable_wr();
			w25x_block64_erase(DUMP_OFFSET + addr);
//			w25x_wait_busy();
		}
		for (addr = 0; addr < DUMP_RAM_SIZE; addr += DUMP_PAGE_SIZE)
		{
			w25x_wait_busy();
			w25x_enable_wr();
			w25x_page_program(DUMP_OFFSET + addr, (uint8_t*)(DUMP_RAM_ADDR + addr), DUMP_PAGE_SIZE);
//			w25x_wait_busy();
//			w25x_rd_data(DUMP_OFFSET + addr, buff, DUMP_BUFF_SIZE);
		}
		for (addr = 0; addr < DUMP_CCRAM_SIZE; addr += DUMP_PAGE_SIZE)
		{
			w25x_wait_busy();
			w25x_enable_wr();
			w25x_page_program(DUMP_OFFSET + DUMP_RAM_SIZE + addr, (uint8_t*)(DUMP_CCRAM_ADDR + addr), DUMP_PAGE_SIZE);
//			w25x_wait_busy();
//			w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + addr, buff, DUMP_BUFF_SIZE);
		}
		w25x_wait_busy();
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
				memset(buff, 0, DUMP_BUFF_SIZE);
				w25x_rd_data(addr, buff, DUMP_BUFF_SIZE);
				f_write(&fil, buff, DUMP_BUFF_SIZE, &bw);
			}
			f_close(&fil);
			return 1;
		}
	}
	return 0;
}

void dump_hardfault_test(void)
{
}
