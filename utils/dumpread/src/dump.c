// dump.c

#include <crash_dump/dump.h>
#include <stdlib.h>
#include <string.h>
#include "dump_rtos.h"
#include "dump_marlinapi.h"

#define DUMP_MAX_SYMBOL_CHARS       32
#define DUMP_MAX_ADDITIONAL_SYMBOLS 100

uint32_t additional_symbol_addr[DUMP_MAX_ADDITIONAL_SYMBOLS];
uint32_t additional_symbol_size[DUMP_MAX_ADDITIONAL_SYMBOLS];
char additional_symbol_name[DUMP_MAX_ADDITIONAL_SYMBOLS][DUMP_MAX_SYMBOL_CHARS];
uint32_t additional_symbol_count = 0;

dump_t *dump_alloc(void) {
    _Static_assert(sizeof(dump_regs_gen_t) == 96, "invalid sizeof(dump_regs_gen_t)");
    _Static_assert(sizeof(dump_tcb_t) == 68, "invalid sizeof(dump_tcb_t)");
    _Static_assert(sizeof(dump_t) == 24, "invalid sizeof(dump_t)");

    dump_t *pd = (dump_t *)malloc(sizeof(dump_t));
    pd->ram = (uint8_t *)malloc(DUMP_RAM_SIZE);
    pd->ccram = (uint8_t *)malloc(DUMP_CCRAM_SIZE);
    pd->otp = (uint8_t *)malloc(DUMP_OTP_SIZE);
    pd->flash = (uint8_t *)malloc(DUMP_FLASH_SIZE);
    if (pd->ram && pd->ccram && pd->otp && pd->flash)
        return pd;
    dump_free(pd);
    return 0;
}

void dump_free(dump_t *pd) {
    if (pd->ram)
        free(pd->ram);
    if (pd->ccram)
        free(pd->ccram);
    if (pd->otp)
        free(pd->otp);
    if (pd->flash)
        free(pd->flash);
    pd->ram = 0;
    pd->ccram = 0;
    pd->otp = 0;
    pd->flash = 0;
}

dump_t *dump_load(const char *fn) {
    dump_t *pd;
    if ((pd = dump_alloc()) == 0)
        return 0;
    FILE *fdump_bin = fopen(fn, "rb");
    if (fdump_bin) {
        int rd_ram = fread(pd->ram, 1, DUMP_RAM_SIZE, fdump_bin);
        int rd_ccram = fread(pd->ccram, 1, DUMP_CCRAM_SIZE, fdump_bin);
        int rd_otp = fread(pd->otp, 1, DUMP_OTP_SIZE, fdump_bin);
        int rd_flash = fread(pd->flash, 1, DUMP_FLASH_SIZE, fdump_bin);
        fclose(fdump_bin);
        if ((rd_ram == DUMP_RAM_SIZE) && (rd_ccram == DUMP_CCRAM_SIZE) && (rd_otp == DUMP_OTP_SIZE) && (rd_flash == DUMP_FLASH_SIZE)) {
            pd->regs_gen = (dump_regs_gen_t *)dump_get_data_ptr(pd, DUMP_REGS_GEN);
            pd->regs_scb = (uint32_t *)dump_get_data_ptr(pd, DUMP_REGS_SCB);
            pd->info = (dump_info_t *)dump_get_data_ptr(pd, DUMP_INFO);
            return pd;
        }
    }
    dump_free(pd);
    return 0;
}

int dump_load_all_sections(dump_t *pd, const char *dir) {
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

int dump_save_all_sections(dump_t *pd, const char *dir) {
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

uint8_t *dump_get_data_ptr(dump_t *pd, uint32_t addr) {
    uint8_t *p = 0;
    if ((addr >= DUMP_RAM_ADDR) && (addr < (DUMP_RAM_ADDR + DUMP_RAM_SIZE)))
        p = pd->ram + addr - DUMP_RAM_ADDR;
    else if ((addr >= DUMP_CCRAM_ADDR) && (addr < (DUMP_CCRAM_ADDR + DUMP_CCRAM_SIZE)))
        p = pd->ccram + addr - DUMP_CCRAM_ADDR;
    else if ((addr >= DUMP_OTP_ADDR) && (addr < (DUMP_OTP_ADDR + DUMP_OTP_SIZE)))
        p = pd->otp + addr - DUMP_OTP_ADDR;
    else if ((addr >= DUMP_FLASH_ADDR) && (addr < (DUMP_FLASH_ADDR + DUMP_FLASH_SIZE)))
        p = pd->flash + addr - DUMP_FLASH_ADDR;
    return p;
}

void dump_get_data(dump_t *pd, uint32_t addr, uint32_t size, uint8_t *data) {
    uint32_t i;
    uint8_t *p;
    for (i = 0; i < size; i++) {
        p = dump_get_data_ptr(pd, addr + i);
        data[i] = p ? *p : 0xff;
    }
}

uint32_t dump_get_ui32(dump_t *pd, uint32_t addr) {
    uint32_t value;
    dump_get_data(pd, addr, 4, (uint8_t *)&value);
    return value;
}

int dump_load_bin_from_file(void *data, int size, const char *fn) {
    FILE *fbin = fopen(fn, "rb");
    if (fbin) {
        int rb = fread(data, 1, size, fbin);
        fclose(fbin);
        return rb;
    }
    return 0;
}

int dump_save_bin_to_file(void *data, int size, const char *fn) {
    FILE *fbin = fopen(fn, "wb");
    if (fbin) {
        int wb = fwrite(data, 1, size, fbin);
        fclose(fbin);
        return wb;
    }
    return 0;
}

uint32_t dump_find_in_flash(dump_t *pd, uint8_t *pdata, uint16_t size, uint32_t start_addr, uint32_t end_addr) {
    if (start_addr < DUMP_FLASH_ADDR)
        start_addr = DUMP_FLASH_ADDR;
    if (end_addr > (DUMP_FLASH_ADDR + DUMP_FLASH_SIZE - size))
        end_addr = (DUMP_FLASH_ADDR + DUMP_FLASH_SIZE - size);
    if (start_addr < end_addr)
        for (uint32_t addr = start_addr; addr <= end_addr; addr++)
            if (memcmp(pdata, pd->flash + addr - DUMP_FLASH_ADDR, size) == 0)
                return addr;
    return 0xffffffff;
}

uint32_t dump_find_in_ram(dump_t *pd, uint8_t *pdata, uint16_t size, uint32_t start_addr, uint32_t end_addr) {
    if (start_addr < DUMP_RAM_ADDR)
        start_addr = DUMP_RAM_ADDR;
    if (end_addr > (DUMP_RAM_ADDR + DUMP_RAM_SIZE - size))
        end_addr = (DUMP_RAM_ADDR + DUMP_RAM_SIZE - size);
    if (start_addr < end_addr)
        for (uint32_t addr = start_addr; addr <= end_addr; addr++)
            if (memcmp(pdata, pd->ram + addr - DUMP_RAM_ADDR, size) == 0)
                return addr;
    return 0xffffffff;
}

int dump_add_symbol(uint32_t addr, uint32_t size, const char *name) {
    additional_symbol_addr[additional_symbol_count] = addr;
    additional_symbol_size[additional_symbol_count] = size;
    strcpy(additional_symbol_name[additional_symbol_count], name);
    additional_symbol_count++;
    return additional_symbol_count - 1;
}

int dump_find_symbol_by_addr(uint32_t addr, char *name, uint32_t *offs) {
    int i;
    for (i = 0; i < additional_symbol_count; i++)
        if ((additional_symbol_addr[i] <= addr) && (additional_symbol_addr[i] + additional_symbol_size[i] > addr))
            break;
    if (i < additional_symbol_count) {
        if (name)
            strcpy(name, additional_symbol_name[i]);
        if (offs)
            *offs = addr - additional_symbol_addr[i];
        return i;
    }
    return -1;
}

int dump_resolve_addr(dump_t *pd, mapfile_t *pm, uint32_t addr, char *name, uint32_t *offs) {
    if (dump_find_symbol_by_addr(addr, name, offs) >= 0)
        return 1;
    if (pm) {
        mapfile_mem_entry_t *e = mapfile_find_mem_entry_by_addr(pm, addr);
        if (e) {
            if (name && e->name)
                strcpy(name, e->name);
            if (offs)
                *offs = addr - e->addr;
            return 1;
        }
    }
    if (name)
        *name = 0;
    return 0;
}

void dump_print_reg(dump_t *pd, mapfile_t *pm, const char *name, uint32_t val) {
    char resolved_name[128];
    uint32_t resolved_offs;
    char resolved_offs_str[16];
    int resolved = dump_resolve_addr(pd, pm, val, resolved_name, &resolved_offs);
    if (resolved)
        sprintf(resolved_offs_str, "%u", resolved_offs);
    else
        *resolved_offs_str = 0;
    printf(" %-4s = 0x%08x %s%s%s%s%s\n", name, val, resolved ? " (" : "", resolved_name, resolved ? " + " : "", resolved_offs_str, resolved ? ")" : "");
    fflush(stdout);
}

mapfile_mem_entry_t *dump_print_var(dump_t *pd, mapfile_t *pm, const char *name) {
    mapfile_mem_entry_t *e;
    if ((e = mapfile_find_mem_entry_by_name(pm, name)) != NULL) {
        if (dump_get_data_ptr(pd, e->addr)) {
            printf("%s @0x%08x, size = 0x%04x (%u)\n", name, e->addr, e->size, e->size);
        } else
            printf("%s address 0x%08x is invalid !\n", name, e->addr);
    } else
        printf("%s not found in mapfile!\n", name);
    fflush(stdout);
    return e;
}

mapfile_mem_entry_t *dump_print_var_ui32(dump_t *pd, mapfile_t *pm, const char *name) {
    mapfile_mem_entry_t *e;
    if ((e = mapfile_find_mem_entry_by_name(pm, name)) != NULL) {
        if (dump_get_data_ptr(pd, e->addr)) {
            uint32_t value = dump_get_ui32(pd, e->addr);
            printf("%s=0x%08x (%u), @0x%08x\n", name, value, value, e->addr);
        } else
            printf("%s address 0x%08x is invalid !\n", name, e->addr);
    } else
        printf("%s not found in mapfile!\n", name);
    fflush(stdout);
    return e;
}

mapfile_mem_entry_t *dump_print_var_pchar(dump_t *pd, mapfile_t *pm, const char *name) {
    mapfile_mem_entry_t *e;
    if ((e = mapfile_find_mem_entry_by_name(pm, name)) != NULL) {
        char *str;
        if ((str = (char *)dump_get_data_ptr(pd, e->addr)) != NULL) {
            printf("%s='%s', @0x%08x\n", name, str, e->addr);
        } else
            printf("%s address 0x%08x is invalid !\n", name, e->addr);
    } else
        printf("%s not found in mapfile!\n", name);
    fflush(stdout);
    return e;
}

void dump_print_stackframe(dump_t *pd, mapfile_t *pm) {
    printf("Stack Frame:\n");
    dump_print_reg(pd, pm, "R0", pd->regs_gen->R0);
    dump_print_reg(pd, pm, "R1", pd->regs_gen->R1);
    dump_print_reg(pd, pm, "R2", pd->regs_gen->R2);
    dump_print_reg(pd, pm, "R3", pd->regs_gen->R3);
    dump_print_reg(pd, pm, "R12", pd->regs_gen->R12);
    dump_print_reg(pd, pm, "SP", pd->regs_gen->SP);
    dump_print_reg(pd, pm, "LR", pd->regs_gen->LR);
    dump_print_reg(pd, pm, "PC", pd->regs_gen->PC);
    dump_print_reg(pd, pm, "PSR", pd->regs_gen->PSR);
}

void dump_print_registers(dump_t *pd, mapfile_t *pm) {
    printf("General registers:\n");
    dump_print_reg(pd, pm, "R4", pd->regs_gen->R4);
    dump_print_reg(pd, pm, "R5", pd->regs_gen->R5);
    dump_print_reg(pd, pm, "R6", pd->regs_gen->R6);
    dump_print_reg(pd, pm, "R7", pd->regs_gen->R7);
    dump_print_reg(pd, pm, "R8", pd->regs_gen->R8);
    dump_print_reg(pd, pm, "R9", pd->regs_gen->R9);
    dump_print_reg(pd, pm, "R10", pd->regs_gen->R10);
    dump_print_reg(pd, pm, "R11", pd->regs_gen->R11);

    dump_print_reg(pd, pm, "PRIMASK", pd->regs_gen->PRIMASK);
    dump_print_reg(pd, pm, "BASEPRI", pd->regs_gen->BASEPRI);
    dump_print_reg(pd, pm, "FAULTMASK", pd->regs_gen->FAULTMASK);
    dump_print_reg(pd, pm, "CONTROL", pd->regs_gen->CONTROL);
    dump_print_reg(pd, pm, "MSP", pd->regs_gen->MSP);
    dump_print_reg(pd, pm, "PSP", pd->regs_gen->PSP);
    dump_print_reg(pd, pm, "LREXC", pd->regs_gen->LREXC);

    printf("SCB registers:\n");
    dump_print_reg(pd, pm, "CFSR", pd->regs_scb[0x28 / 4]);
    dump_print_reg(pd, pm, "HFSR", pd->regs_scb[0x2c / 4]);
    dump_print_reg(pd, pm, "DFSR", pd->regs_scb[0x30 / 4]);
    dump_print_reg(pd, pm, "AFSR", pd->regs_scb[0x3c / 4]);
    dump_print_reg(pd, pm, "BFAR", pd->regs_scb[0x38 / 4]);
}

void dump_print_system(dump_t *pd, mapfile_t *pm) {
    if (!pm)
        return;
    printf("\nSYSTEM\n");
    mapfile_mem_entry_t *e;
    dump_print_var_pchar(pd, pm, "project_version_full");
    if ((e = dump_print_var_ui32(pd, pm, "uwTick")) != NULL) {
        uint32_t uwTick = dump_get_ui32(pd, e->addr);
        int days = uwTick / (1000 * 60 * 60 * 24);
        int hours = (uwTick - (days * 1000 * 60 * 60 * 24)) / (1000 * 60 * 60);
        int mins = (uwTick - (days * 1000 * 60 * 60 * 24) - (hours * 1000 * 60 * 60)) / (1000 * 60);
        int secs = (uwTick - (days * 1000 * 60 * 60 * 24) - (hours * 1000 * 60 * 60) - (mins * 1000 * 60)) / 1000;
        int msecs = (uwTick - (days * 1000 * 60 * 60 * 24) - (hours * 1000 * 60 * 60) - (mins * 1000 * 60) - (secs * 1000));
        printf(" system runtime = %u days %u hours %u minutes %u seconds %u miliseconds\n", days, hours, mins, secs, msecs);
    }
    dump_print_var(pd, pm, "__malloc_av_");
    dump_print_var_ui32(pd, pm, "__malloc_sbrk_base");
    dump_print_var_ui32(pd, pm, "__malloc_trim_threshold");
    if ((e = dump_print_var(pd, pm, "__malloc_current_mallinfo")) != NULL) {
        dump_mallinfo_t *pmallinfo = (dump_mallinfo_t *)dump_get_data_ptr(pd, e->addr);
        printf(" arena    = 0x%08x  /* total space allocated from system */\n", pmallinfo->arena);
        printf(" ordblks  = 0x%08x  /* number of non-inuse chunks */\n", pmallinfo->ordblks);
        printf(" smblks   = 0x%08x  /* unused -- always zero */\n", pmallinfo->smblks);
        printf(" hblks    = 0x%08x  /* number of mmapped regions */\n", pmallinfo->hblks);
        printf(" hblkhd   = 0x%08x  /* total space in mmapped regions */\n", pmallinfo->hblkhd);
        printf(" usmblks  = 0x%08x  /* unused -- always zero */\n", pmallinfo->usmblks);
        printf(" fsmblks  = 0x%08x  /* unused -- always zero */\n", pmallinfo->fsmblks);
        printf(" uordblks = 0x%08x  /* total allocated space */\n", pmallinfo->uordblks);
        printf(" fordblks = 0x%08x  /* total non-inuse space */\n", pmallinfo->fordblks);
        printf(" keepcost = 0x%08x  /* top-most, releasable (via malloc_trim) space */\n", pmallinfo->keepcost);
    }
    dump_print_var_ui32(pd, pm, "__malloc_max_sbrked_mem");
    dump_print_var_ui32(pd, pm, "__malloc_max_total_mem");
    dump_print_var_ui32(pd, pm, "__malloc_top_pad");
    dump_print_var_ui32(pd, pm, "variant8_total_malloc_size");
}

void dump_print_stack(dump_t *pd, mapfile_t *pm, uint32_t addr, uint32_t depth) {
    char resolved_name[256];
    uint32_t resolved_offs;
    char resolved_offs_str[16];
    for (uint32_t i = 0; i < depth; i++) {
        uint32_t data = dump_get_ui32(pd, addr + 4 * i);
        int resolved = dump_resolve_addr(pd, pm, data, resolved_name, &resolved_offs);
        if (resolved)
            sprintf(resolved_offs_str, "%u", resolved_offs);
        else
            *resolved_offs_str = 0;
        printf("%-3u 0x%08x 0x%08x %s%s%s%s%s\n", i, addr + 4 * i, data, resolved ? " (" : "", resolved_name, resolved ? " + " : "", resolved_offs_str, resolved ? ")" : "");
    }
}

void dump_print_common(dump_t *pd, mapfile_t *pm) {
    dump_print_stackframe(pd, pm);
    dump_print_registers(pd, pm);
    dump_print_system(pd, pm);
    dump_rtos_print(pd, pm);
    dump_marlinapi_print(pd, pm);
}

void dump_print_hardfault(dump_t *pd, mapfile_t *pm) {
    printf("\nHARDFAULT DUMP\n");
    dump_print_common(pd, pm);
}

void dump_print_watchdog(dump_t *pd, mapfile_t *pm) {
    printf("\nWATCHDOG RESET DUMP\n");
    dump_print_common(pd, pm);
}

void dump_print_all(dump_t *pd, mapfile_t *pm) {
    switch (pd->info->type_flags & 3) {
    case 1:
        dump_print_hardfault(pd, pm);
        break;
    case 2:
        dump_print_watchdog(pd, pm);
        break;
    default:
        printf("DUMP NOT VALID!\n");
    }
}
