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
        {
            pd->regs_gen = (dump_regs_gen_t*)dump_get_data_ptr(pd, DUMP_REGS_GEN);
            pd->regs_scb = dump_get_data_ptr(pd, DUMP_REGS_SCB);
            return pd;
        }
    }
    dump_free(pd);
    return 0;
}

int dump_load_all_sections(dump_t* pd, const char* dir)
{
    char path[MAX_PATH];
    strcpy(path, dir);
    strcpy(path + strlen(dir), "dump_ram.bin");
    dump_load_bin_from_file(pd->ram, DUMP_RAM_SIZE, path);
    strcpy(path + strlen(dir), "dump_ccram.bin");
    dump_load_bin_from_file(pd->ccram, DUMP_CCRAM_SIZE, path);
    strcpy(path + strlen(dir), "dump_otp.bin");
    dump_load_bin_from_file(pd->otp, DUMP_OTP_SIZE, path);
    strcpy(path + strlen(dir), "dump_flash.bin");
    dump_load_bin_from_file(pd->flash, DUMP_FLASH_SIZE, path);
    return 1;
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

void dump_print_hardfault_stackframe(dump_t* pd)
{
    printf("Stack Frame:\n");
    printf("R0  = 0x%08x\n", pd->regs_gen->R0);
    printf("R1  = 0x%08x\n", pd->regs_gen->R1);
    printf("R2  = 0x%08x\n", pd->regs_gen->R2);
    printf("R3  = 0x%08x\n", pd->regs_gen->R3);
    printf("R12 = 0x%08x\n", pd->regs_gen->R12);
    printf("SP  = 0x%08x\n", pd->regs_gen->SP);
    printf("LR  = 0x%08x\n", pd->regs_gen->LR);
    printf("PC  = 0x%08x\n", pd->regs_gen->PC);
    printf("PSR = 0x%08x\n", pd->regs_gen->PSR);
}

void dump_print_hardfault_simple(dump_t* pd)
{
    printf("HARDFAULT\n");
    dump_print_hardfault_stackframe(pd);
    printf("SCB:\n");
    printf("CFSR = 0x%08x\n", pd->regs_scb[0x28/4]);
    printf("HFSR = 0x%08x\n", pd->regs_scb[0x2c/4]);
    printf("DFSR = 0x%08x\n", pd->regs_scb[0x30/4]);
    printf("AFSR = 0x%08x\n", pd->regs_scb[0x3c/4]);
    printf("BFAR = 0x%08x\n", pd->regs_scb[0x38/4]);
    printf("Misc:\n");
    printf("LREXC     = 0x%08x\n", pd->regs_gen->PSR);
}

void dump_print_hardfault_detail(dump_t* pd)
{
    printf("HARDFAULT\n");
    dump_print_hardfault_stackframe(pd);

    uint32_t* scb_regs = (uint32_t*)(pd->ccram + 0xff60);

    printf("registers:\n");
    printf("R4  = 0x%08x\n", pd->regs_gen->R4);
    printf("R5  = 0x%08x\n", pd->regs_gen->R5);
    printf("R6  = 0x%08x\n", pd->regs_gen->R6);
    printf("R7  = 0x%08x\n", pd->regs_gen->R7);
    printf("R8  = 0x%08x\n", pd->regs_gen->R8);
    printf("R9  = 0x%08x\n", pd->regs_gen->R9);
    printf("R10 = 0x%08x\n", pd->regs_gen->R10);
    printf("R11 = 0x%08x\n", pd->regs_gen->R11);
    printf("PRIMASK   = 0x%08x\n", pd->regs_gen->PRIMASK);
    printf("BASEPRI   = 0x%08x\n", pd->regs_gen->BASEPRI);
    printf("FAULTMASK = 0x%08x\n", pd->regs_gen->FAULTMASK);
    printf("CONTROL   = 0x%08x\n", pd->regs_gen->CONTROL);
    printf("MSP       = 0x%08x\n", pd->regs_gen->MSP);
    printf("PSP       = 0x%08x\n", pd->regs_gen->PSP);
    printf("LREXC     = 0x%08x\n", pd->regs_gen->LREXC);
    printf("SCB:\n");
    printf("CFSR = 0x%08x\n", scb_regs[0x28/4]);
    printf("HFSR = 0x%08x\n", scb_regs[0x2c/4]);
    printf("DFSR = 0x%08x\n", scb_regs[0x30/4]);
    printf("AFSR = 0x%08x\n", scb_regs[0x3c/4]);
    printf("BFAR = 0x%08x\n", scb_regs[0x38/4]);
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
