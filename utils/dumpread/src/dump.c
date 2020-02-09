// dump.c

#include "dump.h"
#include <stdlib.h>
#include <string.h>


dump_t* dump_alloc(void)
{
	dump_t* pd = (dump_t*)malloc(sizeof(dump_t));
	pd->ram = (uint8_t*)malloc(DUMP_RAM_SIZE);
	pd->ccram = (uint8_t*)malloc(DUMP_CCRAM_SIZE);
	pd->otp = (uint8_t*)malloc(DUMP_OTP_SIZE);
	pd->flash = (uint8_t*)malloc(DUMP_FLASH_SIZE);
	if (pd->ram && pd->ccram && pd->otp && pd->flash)
		return pd;
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

dump_t* dump_load(const char* fn)
{
	dump_t* pd;
	if ((pd = dump_alloc()) == 0) return 0;
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
			return pd;
	}
	dump_free(pd);
	return 0;
}

int dump_save_all_sections(dump_t* pd, const char* dir)
{
	char path[MAX_PATH];
	strcpy(path, dir);
	strcpy(path + strlen(dir), "dump_ram.bin");
	dump_save_bin_to_file(pd->ram, DUMP_RAM_SIZE, path);
	strcpy(path + strlen(dir), "dump_ccram.bin");
	dump_save_bin_to_file(pd->ccram, DUMP_CCRAM_SIZE, path);
	strcpy(path + strlen(dir), "dump_otp.bin");
	dump_save_bin_to_file(pd->otp, DUMP_OTP_SIZE, path);
	strcpy(path + strlen(dir), "dump_flash.bin");
	dump_save_bin_to_file(pd->flash, DUMP_FLASH_SIZE, path);
	return 1;
}

uint8_t* dump_get_data_ptr(dump_t* pd, uint32_t addr)
{
	uint8_t* p = 0;
	if ((addr >= DUMP_RAM_ADDR) && (addr < (DUMP_RAM_ADDR + DUMP_RAM_SIZE))) p = pd->ram + addr - DUMP_RAM_ADDR;
	else if ((addr >= DUMP_CCRAM_ADDR) && (addr < (DUMP_CCRAM_ADDR + DUMP_CCRAM_SIZE))) p = pd->ccram + addr - DUMP_CCRAM_ADDR;
	else if ((addr >= DUMP_OTP_ADDR) && (addr < (DUMP_OTP_ADDR + DUMP_OTP_SIZE))) p = pd->otp + addr - DUMP_OTP_ADDR;
	else if ((addr >= DUMP_FLASH_ADDR) && (addr < (DUMP_FLASH_ADDR + DUMP_FLASH_SIZE))) p = pd->flash + addr - DUMP_FLASH_ADDR;
	return p;
}

void dump_get_data(dump_t* pd, uint32_t addr, uint32_t size, uint8_t* data)
{
	uint32_t i;
	uint8_t* p;
	for (i = 0; i < size; i++)
	{
		p = dump_get_data_ptr(pd, addr + i);
		data[i] = p?*p:0xff;
	}
}

uint32_t dump_get_ui32(dump_t* pd, uint32_t addr)
{
	uint32_t value;
	dump_get_data(pd, addr, 4, (uint8_t*)&value);
	return value;
}

int dump_load_bin_from_file(void* data, int size, const char* fn)
{
	FILE* fbin = fopen(fn, "rb");
	if (fbin)
	{
		int rb = fread(data, 1, size, fbin);
		fclose(fbin);
		return (rb == size)?1:0;
	}
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

void dump_print_hardfault_stackframe(uint32_t* gen_regs)
{
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
}

void dump_print_hardfault_simple(uint32_t* gen_regs, uint32_t* scb_regs)
{
	printf("HARDFAULT\n");
	dump_print_hardfault_stackframe(gen_regs);
	printf("SCB:\n");
	printf("CFSR = 0x%08x\n", scb_regs[0x28/4]);
	printf("HFSR = 0x%08x\n", scb_regs[0x2c/4]);
	printf("DFSR = 0x%08x\n", scb_regs[0x30/4]);
	printf("AFSR = 0x%08x\n", scb_regs[0x3c/4]);
	printf("BFAR = 0x%08x\n", scb_regs[0x38/4]);
	printf("Misc:\n");
	printf("LREXC     = 0x%08x\n", gen_regs[0x5c/4]);
}

void dump_print_hardfault_detail(uint32_t* gen_regs, uint32_t* scb_regs)
{
	printf("HARDFAULT\n");
	dump_print_hardfault_stackframe(gen_regs);
	printf("registers:\n");
	printf("R4  = 0x%08x\n", gen_regs[0x10/4]);
	printf("R5  = 0x%08x\n", gen_regs[0x14/4]);
	printf("R6  = 0x%08x\n", gen_regs[0x18/4]);
	printf("R7  = 0x%08x\n", gen_regs[0x1c/4]);
	printf("R8  = 0x%08x\n", gen_regs[0x10/4]);
	printf("R9  = 0x%08x\n", gen_regs[0x14/4]);
	printf("R10 = 0x%08x\n", gen_regs[0x18/4]);
	printf("R11 = 0x%08x\n", gen_regs[0x1c/4]);
	printf("PRIMASK   = 0x%08x\n", gen_regs[0x44/4]);
	printf("BASEPRI   = 0x%08x\n", gen_regs[0x48/4]);
	printf("FAULTMASK = 0x%08x\n", gen_regs[0x4c/4]);
	printf("CONTROL   = 0x%08x\n", gen_regs[0x50/4]);
	printf("MSP       = 0x%08x\n", gen_regs[0x54/4]);
	printf("PSP       = 0x%08x\n", gen_regs[0x58/4]);
	printf("LREXC     = 0x%08x\n", gen_regs[0x5c/4]);
	printf("SCB:\n");
	printf("CFSR = 0x%08x\n", scb_regs[0x28/4]);
	printf("HFSR = 0x%08x\n", scb_regs[0x2c/4]);
	printf("DFSR = 0x%08x\n", scb_regs[0x30/4]);
	printf("AFSR = 0x%08x\n", scb_regs[0x3c/4]);
	printf("BFAR = 0x%08x\n", scb_regs[0x38/4]);
}
