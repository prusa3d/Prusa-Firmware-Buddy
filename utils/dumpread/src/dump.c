// dump.c

#include "dump.h"
#include <stdlib.h>
#include <string.h>


int dump_alloc(dump_t* pd)
{
	pd->ram = (uint8_t*)malloc(DUMP_RAM_SIZE);
	pd->ccram = (uint8_t*)malloc(DUMP_CCRAM_SIZE);
	pd->otp = (uint8_t*)malloc(DUMP_OTP_SIZE);
	pd->flash = (uint8_t*)malloc(DUMP_FLASH_SIZE);
	if (pd->ram && pd->ccram && pd->otp && pd->flash)
		return 1;
	dump_free(pd);
	return 0;
}

void dump_free(dump_t* pd)
{
	if (pd->ram) free(pd->ram);
	if (pd->ccram) free(pd->ccram);
	if (pd->otp) free(pd->otp);
	if (pd->flash) free(pd->flash);
	pd->ram = 0;
	pd->ccram = 0;
	pd->otp = 0;
	pd->flash = 0;
}

int dump_load(dump_t* pd, const char* fn)
{
	if (dump_alloc(pd) == 0) return 0;
	FILE* fdump_bin = fopen(fn, "rb");
	if (fdump_bin)
	{
		int rd_ram = fread(pd->ram, 1, DUMP_RAM_SIZE, fdump_bin);
		int rd_ccram = fread(pd->ccram, 1, DUMP_CCRAM_SIZE, fdump_bin);
		int rd_otp = fread(pd->otp, 1, DUMP_OTP_SIZE, fdump_bin);
		int rd_flash = fread(pd->flash, 1, DUMP_FLASH_SIZE, fdump_bin);
		fclose(fdump_bin);
		if ((rd_ram == DUMP_RAM_SIZE) &&
			(rd_ccram == DUMP_CCRAM_SIZE) &&
			(rd_otp == DUMP_OTP_SIZE) &&
			(rd_flash == DUMP_FLASH_SIZE))
			return 1;
	}
	dump_free(pd);
	return 0;
}

int dump_save_bin_to_file(void* data, int size, const char* fn)
{
	FILE* fbin = fopen(fn, "wb");
	if (fbin)
	{
		int wb = fwrite(data, 1, size, fbin);
		fclose(fbin);
		return (wb == size)?1:0;
	}
	return 0;
}

void dump_print_hardfault_simple(uint32_t* gen_regs, uint32_t* scb_regs)
{
	printf("HARDFAULT\n");
	printf("Stack Frame:\n");
	printf("R0  = 0x%08x\n", gen_regs[0x00/4]);
	printf("R1  = 0x%08x\n", gen_regs[0x04/4]);
	printf("R2  = 0x%08x\n", gen_regs[0x08/4]);
	printf("R3  = 0x%08x\n", gen_regs[0x0c/4]);
	printf("R12 = 0x%08x\n", gen_regs[0x30/4]);
	printf("SP  = 0x%08x\n", gen_regs[0x34/4]);
	printf("LR  = 0x%08x\n", gen_regs[0x38/4]);
	printf("PC  = 0x%08x\n", gen_regs[0x3c/4]);
	printf("PSR = 0x%08x\n", gen_regs[0x40/4]);
	printf("FSR/FAR:\n");
	printf("CFSR = 0x%08x\n", scb_regs[0x28/4]);
	printf("HFSR = 0x%08x\n", scb_regs[0x2c/4]);
	printf("DFSR = 0x%08x\n", scb_regs[0x30/4]);
	printf("AFSR = 0x%08x\n", scb_regs[0x3c/4]);
	printf("BFAR = 0x%08x\n", scb_regs[0x38/4]);
	printf("Misc:\n");
	printf("LREXC= 0x%08x\n", gen_regs[0x5c/4]);
}
