// dump.c

#include "dump.h"
#include <stdlib.h>
#include <string.h>

#define DUMP_RTOS_MAX_TASKS      16 // actually 8 tasks used
#define DUMP_RTOS_MAX_PRIORITIES 7  // configMAX_PRIORITIES = 7

#define DUMP_RTOS_TASK_STATE_SUSPENDED 0
#define DUMP_RTOS_TASK_STATE_DELAYED   1
#define DUMP_RTOS_TASK_STATE_READY     2
#define DUMP_RTOS_TASK_STATE_RUNNING   3

const char *dump_rtos_task_state_text[4] = {
    "SUSPENDED",
    "DELAYED",
    "READY",
    "RUNNING",
};

dump_t *dump_alloc(void) {
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
            pd->regs_scb = (uint32_t)dump_get_data_ptr(pd, DUMP_REGS_SCB);
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

int dump_resolve_addr(dump_t *pd, mapfile_t *pm, uint32_t addr, char *name, uint32_t *offs) {
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
}

mapfile_mem_entry_t *dump_print_var(dump_t *pd, mapfile_t *pm, const char *name) {
    mapfile_mem_entry_t *e;
    if ((e = mapfile_find_mem_entry_by_name(pm, name)) != NULL) {
        if (dump_get_data_ptr(pd, e->addr)) {
            printf("%s @0x%08x\n", name, e->addr);
        } else
            printf("%s address 0x%08x is invalid !\n", name, e->addr);
    } else
        printf("%s not found in mapfile!\n", name);
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

uint32_t dump_rtos_task_stacksize(dump_t *pd, uint32_t task_handle) {
    dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
    if (strcmp(ptcb->pcTaskName, "defaultTask") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "displayTask") == 0)
        return 2048 * 4;
    else if (strcmp(ptcb->pcTaskName, "webServerTask") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "measurementTask") == 0)
        return 512 * 4;
    else if (strcmp(ptcb->pcTaskName, "IDLE") == 0)
        return 128 * 4;
    else if (strcmp(ptcb->pcTaskName, "USBH_Thread") == 0)
        return 128 * 4;
    else if (strcmp(ptcb->pcTaskName, "tcpip_thread") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "EthIf") == 0)
        return 350 * 4;
    return 0;
}

uint32_t dump_rtos_list_task_handles(dump_t *pd, uint32_t list_addr, uint32_t *task_handles) {
    dump_list_t *plist = (dump_list_t *)dump_get_data_ptr(pd, list_addr);
    uint32_t index = 0;
    dump_listitem_t *plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plist->pxIndex);
    if (plistitem->pvOwner == 0)
        plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plistitem->pxNext);
    while (index < plist->uxNumberOfItems) {
        task_handles[index] = plistitem->pvOwner;
        plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plistitem->pxNext);
        index++;
    }
    return index;
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

void dump_print_rtos(dump_t *pd, mapfile_t *pm) {
    if (!pm)
        return;
    printf("\nRTOS\n");
    mapfile_mem_entry_t *e;
    uint32_t uxCurrentNumberOfTasks = 0;
    uint32_t xIdleTaskHandle = 0;
    uint32_t defaultTaskHandle = 0;
    uint32_t displayTaskHandle = 0;
    uint32_t webServerTaskHandle = 0;
    uint32_t pxCurrentTCB = 0;
    dump_print_var_ui32(pd, pm, "xFreeBytesRemaining");
    dump_print_var_ui32(pd, pm, "xMinimumEverFreeBytesRemaining");
    if ((e = dump_print_var_ui32(pd, pm, "uxCurrentNumberOfTasks")) != NULL)
        uxCurrentNumberOfTasks = dump_get_ui32(pd, e->addr);
    dump_print_var_ui32(pd, pm, "xTickCount");
    dump_print_var_ui32(pd, pm, "uxTopReadyPriority");
    dump_print_var_ui32(pd, pm, "xSchedulerRunning");
    dump_print_var_ui32(pd, pm, "uxPendedTicks");
    dump_print_var_ui32(pd, pm, "xYieldPending");
    dump_print_var_ui32(pd, pm, "xNumOfOverflows");
    dump_print_var_ui32(pd, pm, "uxTaskNumber");
    dump_print_var_ui32(pd, pm, "xNextTaskUnblockTime");
    if ((e = dump_print_var_ui32(pd, pm, "xIdleTaskHandle")) != NULL)
        xIdleTaskHandle = dump_get_ui32(pd, e->addr);

    if ((e = dump_print_var_ui32(pd, pm, "defaultTaskHandle")) != NULL)
        defaultTaskHandle = dump_get_ui32(pd, e->addr);
    if ((e = dump_print_var_ui32(pd, pm, "displayTaskHandle")) != NULL)
        displayTaskHandle = dump_get_ui32(pd, e->addr);
    if ((e = dump_print_var_ui32(pd, pm, "webServerTaskHandle")) != NULL)
        webServerTaskHandle = dump_get_ui32(pd, e->addr);

    if ((e = dump_print_var_ui32(pd, pm, "pxCurrentTCB")) != NULL)
        pxCurrentTCB = dump_get_ui32(pd, e->addr);

    // all currently created tasks sorted by tasknumber
    uint32_t task_handles[DUMP_RTOS_MAX_TASKS];
    memset(task_handles, 0, sizeof(task_handles));
    uint32_t task_states[DUMP_RTOS_MAX_TASKS];
    uint32_t task_count = 0;

    // we need enumerate all task lists in RTOS to retreive list of all created tasks
    uint32_t task_handles_ready[DUMP_RTOS_MAX_PRIORITIES][DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_ready_count[DUMP_RTOS_MAX_PRIORITIES] = { 0, 0, 0, 0, 0, 0, 0 };
    if ((e = dump_print_var(pd, pm, "pxReadyTasksLists")) != NULL) {
        for (int pri = 0; pri < DUMP_RTOS_MAX_PRIORITIES; pri++) {
            printf("priority %d\n", pri);
            task_handles_ready_count[pri] = dump_rtos_list_task_handles(pd, e->addr + pri * sizeof(dump_list_t), task_handles_ready[pri]);
            for (int task = 0; task < task_handles_ready_count[pri]; task++) {
                uint32_t task_handle = task_handles_ready[pri][task];
                dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
                if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                    task_count++;
                task_handles[ptcb->uxTCBNumber - 1] = task_handle;
                if (task_handle == pxCurrentTCB)
                    task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_RUNNING;
                else
                    task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_READY;
                printf(" handle 0x%08x number %u name %s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
            }
        }
    }

    uint32_t task_handles_delayed[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_delayed_count = 0;
    if ((e = dump_print_var(pd, pm, "pxDelayedTaskList")) != NULL) {
        task_handles_delayed_count = dump_rtos_list_task_handles(pd, dump_get_ui32(pd, e->addr), task_handles_delayed);
        for (int task = 0; task < task_handles_delayed_count; task++) {
            uint32_t task_handle = task_handles_delayed[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_DELAYED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    uint32_t task_handles_overflow[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_overflow_count = 0;
    if ((e = dump_print_var(pd, pm, "pxOverflowDelayedTaskList")) != NULL) {
        task_handles_overflow_count = dump_rtos_list_task_handles(pd, dump_get_ui32(pd, e->addr), task_handles_overflow);
        for (int task = 0; task < task_handles_overflow_count; task++) {
            uint32_t task_handle = task_handles_overflow[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_DELAYED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    uint32_t task_handles_suspended[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_suspended_count = 0;
    if ((e = dump_print_var(pd, pm, "xSuspendedTaskList")) != NULL) {
        task_handles_suspended_count = dump_rtos_list_task_handles(pd, e->addr, task_handles_suspended);
        for (int task = 0; task < task_handles_suspended_count; task++) {
            uint32_t task_handle = task_handles_suspended[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_SUSPENDED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    printf("RTOS TASKs (%u)\n", task_count);
    printf(" num handle     state     pri stack      stacktop   name\n");
    for (int task = 0; task < task_count; task++) {
        uint32_t task_handle = task_handles[task];
        uint32_t task_state = task_states[task];
        dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
        printf(" %-3u 0x%08x %-9s %u/%u 0x%08x 0x%08x %s\n", ptcb->uxTCBNumber, task_handle, dump_rtos_task_state_text[task_state], ptcb->uxBasePriority, ptcb->uxPriority, ptcb->pxStack, ptcb->pxTopOfStack, ptcb->pcTaskName);
    }
    printf("RTOS TASK STACKs\n");
    printf(" num stack      stacktop   size   depth maxdepth name\n");
    for (int task = 0; task < task_count; task++) {
        uint32_t task_handle = task_handles[task];
        uint32_t stacksize = dump_rtos_task_stacksize(pd, task_handle);
        dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
        uint32_t stackmax = ptcb->pxStack + stacksize;
        float depth = 100.0F * (stackmax - ptcb->pxTopOfStack) / stacksize;
        uint8_t *pstack = dump_get_data_ptr(pd, ptcb->pxStack);
        uint32_t o = 0;
        for (; o < stackmax; o++)
            if (pstack[o] != 0xa5)
                break;
        float maxdepth = 100.0F * (stacksize - o) / stacksize;
        printf(" %-3u 0x%08x 0x%08x 0x%04x %5.1f%% %5.1f%% %s\n", ptcb->uxTCBNumber, ptcb->pxStack, ptcb->pxTopOfStack, stacksize, depth, maxdepth, ptcb->pcTaskName);
    }
    if (1)
        for (int task = 0; task < task_count; task++) {
            uint32_t task_handle = task_handles[task];
            uint32_t stacksize = dump_rtos_task_stacksize(pd, task_handle);
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            uint32_t stacktop = ptcb->pxTopOfStack;
            uint32_t stackmax = ptcb->pxStack + stacksize;
            printf("\nTASK STACK (%s)\n", ptcb->pcTaskName);
            dump_print_stack(pd, pm, stacktop, (stackmax - stacktop) / 4);
            //    if (xIdleTaskHandle)
            //    	dump_rtos_task_stacksize(pd, xIdleTaskHandle);
        }
}

void dump_print_hardfault(dump_t *pd, mapfile_t *pm) {
    printf("\nHARDFAULT DUMP\n");
    dump_print_stackframe(pd, pm);
    dump_print_registers(pd, pm);
    dump_print_system(pd, pm);
    dump_print_rtos(pd, pm);
}

void dump_print_watchdog(dump_t *pd, mapfile_t *pm) {
    printf("\nWATCHDOG RESET DUMP\n");
    dump_print_stackframe(pd, pm);
    dump_print_registers(pd, pm);
    dump_print_system(pd, pm);
    dump_print_rtos(pd, pm);
}

void dump_print(dump_t *pd, mapfile_t *pm) {
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
