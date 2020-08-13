//dumpread - main.c

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dump.h"
#include "mapfile.h"

int main(int argc, char **argv) {
    int ret = 0;

    char dump_fn[MAX_PATH] = "dump.bin";
    char map_fn[MAX_PATH] = "firmware.map";
    char bin_fn[MAX_PATH] = "firmware.bin";
    char out_dir[MAX_PATH] = "";
    dump_t *pdump = 0;
    mapfile_t *pmap = 0;
    uint8_t *pbin = 0;

    int argn = 0;
    char *arg = 0;

    //parse args
    while (++argn < argc) {
        arg = argv[argn];
        if (sscanf(arg, "-dump=%s", dump_fn) == 1)
            continue;
        if (sscanf(arg, "-map=%s", map_fn) == 1)
            continue;
        if (sscanf(arg, "-bin=%s", bin_fn) == 1)
            continue;
        if (sscanf(arg, "-out=%s", out_dir) == 1)
            continue;
    }
    // check valid dump_fn
    if ((ret == 0) && (strlen(dump_fn) == 0)) {
        fputs("DUMP_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    // check if dump_fn file exists
    if ((ret == 0) && (access(dump_fn, F_OK) != 0)) {
        fprintf(stderr, "file '%s' not found!\n", dump_fn);
        ret = 1;
    }
    // check valid map_fn
    if ((ret == 0) && (strlen(map_fn) == 0)) {
        fputs("MAP_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    // check if map_fn file exists
    if ((ret == 0) && (access(map_fn, F_OK) != 0)) {
        fprintf(stderr, "file '%s' not found!\n", map_fn);
        ret = 1;
    }
    if (ret == 0) {

        // load dump file
        if (strlen(dump_fn))
            pdump = dump_load(dump_fn);

        // load map file
        if (strlen(map_fn))
            pmap = mapfile_load(map_fn);

        // load bin file if exists
        if (strlen(bin_fn)) {
            pbin = (uint8_t *)malloc(DUMP_FLASH_SIZE);
            int bin_size = dump_load_bin_from_file(pbin, DUMP_FLASH_SIZE, bin_fn);
            if (memcmp(pbin, pdump->flash + 0x20200, bin_size) == 0)
                printf("Binary image is identical - OK.\n");
            else {
                printf("Binary image is different - NG!\n");
            }
        }

        if (pdump) {
            dump_print_hardfault_detail(pdump);

            puts("");

            if (pmap) {
                mapfile_mem_entry_t *e;

                if ((e = mapfile_find_mem_entry(pmap, "project_version_full")) != NULL) {
                    char *project_version_full = (char *)(pdump->flash + (e->addr - DUMP_FLASH_ADDR));
                    printf("&project_version_full=0x%08x \n", e->addr);
                    printf("project_version_full=%s \n", project_version_full);
                } else
                    printf("project_version_full not found!\n");

                puts("");

                if ((e = mapfile_find_mem_entry(pmap, "xTickCount")) != NULL) {
                    uint32_t xTickCount = dump_get_ui32(pdump, e->addr);
                    printf("xTickCount=%u ms (%u hours)\n", xTickCount, xTickCount / (1000 * 60 * 60));
                }

                if ((e = mapfile_find_mem_entry(pmap, "uwTick")) != NULL) {
                    uint32_t uwTick = dump_get_ui32(pdump, e->addr);
                    printf("uwTick=%u ms (%u hours)\n", uwTick, uwTick / (1000 * 60 * 60));
                }

                puts("");

                // defaultTask
                /*				uint32_t defaultTask_txt_addr = dump_find_in_flash(pdump, (uint8_t*)"defaultTask", 11, 0, -1);
				printf("defaultTask_txt_addr=0x%08x\n", defaultTask_txt_addr);
				uint32_t defaultTask_txt_addr_ref = dump_find_in_flash(pdump, (uint8_t*)&defaultTask_txt_addr, 4, 0, -1);
				printf("defaultTask_txt_addr_ref=0x%08x\n", defaultTask_txt_addr_ref);*/
                if ((e = mapfile_find_mem_entry(pmap, "StartDefaultTask")) != NULL) {
                    printf("StartDefaultTask=0x%08x\n", e->addr);
                    //					uint32_t rStartDefaultTask = dump_find_ui32_in_flash(pdump, e->addr, 0, -1);
                    //					printf("rStartDefaultTask=0x%08x\n", rStartDefaultTask);
                }

                uint32_t defaultTask_StackSize = 1024 * 4;
                if ((e = mapfile_find_mem_entry(pmap, "defaultTaskHandle")) != NULL) {
                    printf("&defaultTaskHandle=0x%08x\n", e->addr);
                    uint32_t defaultTaskHandle = dump_get_ui32(pdump, e->addr);
                    printf("defaultTaskHandle=0x%08x\n", defaultTaskHandle);
                    dump_tcb_t *pdefaultTask_TCB = (dump_tcb_t *)(pdump->ram + (defaultTaskHandle - DUMP_RAM_ADDR));
                    printf(" TCB.pcTaskName=%s\n", pdefaultTask_TCB->pcTaskName);
                    printf(" TCB.pxTopOfStack=0x%08x\n", pdefaultTask_TCB->pxTopOfStack);
                    printf(" TCB.pxStack=0x%08x\n", pdefaultTask_TCB->pxStack);
                    uint32_t defaultTask_StackDepth = pdefaultTask_TCB->pxTopOfStack - pdefaultTask_TCB->pxStack;
                    printf(" StackDepth=0x%08x (%d%%)\n", defaultTask_StackDepth, 100 * (defaultTask_StackSize - defaultTask_StackDepth) / defaultTask_StackSize);
                    uint32_t addr;
                    for (addr = pdefaultTask_TCB->pxStack; addr < (pdefaultTask_TCB->pxStack + defaultTask_StackSize); addr++)
                        if (pdump->ram[addr - DUMP_RAM_ADDR] != 0xa5)
                            break;
                    uint32_t defaultTask_StackMaxDepth = addr - pdefaultTask_TCB->pxStack;
                    printf(" StackMaxDepth=0x%08x (%d%%)\n", defaultTask_StackMaxDepth, 100 * (defaultTask_StackSize - defaultTask_StackMaxDepth) / defaultTask_StackSize);
                }

                puts("");

                // displayTask
                uint32_t displayTask_StackSize = 2048 * 4;
                if ((e = mapfile_find_mem_entry(pmap, "displayTaskHandle")) != NULL) {
                    printf("&displayTaskHandle=0x%08x\n", e->addr);
                    uint32_t displayTaskHandle = dump_get_ui32(pdump, e->addr);
                    printf("displayTaskHandle=0x%08x\n", displayTaskHandle);
                    dump_tcb_t *pdisplayTask_TCB = (dump_tcb_t *)(pdump->ram + (displayTaskHandle - DUMP_RAM_ADDR));
                    printf(" TCB.pcTaskName=%s\n", pdisplayTask_TCB->pcTaskName);
                    printf(" TCB.pxTopOfStack=0x%08x\n", pdisplayTask_TCB->pxTopOfStack);
                    printf(" TCB.pxStack=0x%08x\n", pdisplayTask_TCB->pxStack);
                    uint32_t displayTask_StackDepth = pdisplayTask_TCB->pxTopOfStack - pdisplayTask_TCB->pxStack;
                    printf(" StackDepth=0x%08x (%d%%)\n", displayTask_StackDepth, 100 * (displayTask_StackSize - displayTask_StackDepth) / displayTask_StackSize);
                    uint32_t addr;
                    for (addr = pdisplayTask_TCB->pxStack; addr < (pdisplayTask_TCB->pxStack + displayTask_StackSize); addr++)
                        if (pdump->ram[addr - DUMP_RAM_ADDR] != 0xa5)
                            break;
                    uint32_t displayTask_StackMaxDepth = addr - pdisplayTask_TCB->pxStack;
                    printf(" StackMaxDepth=0x%08x (%d%%)\n", displayTask_StackMaxDepth, 100 * (displayTask_StackSize - displayTask_StackMaxDepth) / displayTask_StackSize);
                }

                puts("");

                // webServerTask
                uint32_t webServerTask_StackSize = 1024 * 4;
                if ((e = mapfile_find_mem_entry(pmap, "webServerTaskHandle")) != NULL) {
                    printf("&webServerTaskHandle=0x%08x\n", e->addr);
                    uint32_t webServerTaskHandle = dump_get_ui32(pdump, e->addr);
                    printf("webServerTaskHandle=0x%08x\n", webServerTaskHandle);
                    dump_tcb_t *pwebServerTask_TCB = (dump_tcb_t *)(pdump->ram + (webServerTaskHandle - DUMP_RAM_ADDR));
                    printf(" TCB.pcTaskName=%s\n", pwebServerTask_TCB->pcTaskName);
                    printf(" TCB.pxTopOfStack=0x%08x\n", pwebServerTask_TCB->pxTopOfStack);
                    printf(" TCB.pxStack=0x%08x\n", pwebServerTask_TCB->pxStack);
                    uint32_t webServerTask_StackDepth = pwebServerTask_TCB->pxTopOfStack - pwebServerTask_TCB->pxStack;
                    printf(" StackDepth=0x%08x (%d%%)\n", webServerTask_StackDepth, 100 * (webServerTask_StackSize - webServerTask_StackDepth) / webServerTask_StackSize);
                    uint32_t addr;
                    for (addr = pwebServerTask_TCB->pxStack; addr < (pwebServerTask_TCB->pxStack + webServerTask_StackSize); addr++)
                        if (pdump->ram[addr - DUMP_RAM_ADDR] != 0xa5)
                            break;
                    uint32_t webServerTask_StackMaxDepth = addr - pwebServerTask_TCB->pxStack;
                    printf(" StackMaxDepth=0x%08x (%d%%)\n", webServerTask_StackMaxDepth, 100 * (webServerTask_StackSize - webServerTask_StackMaxDepth) / webServerTask_StackSize);
                }

                // measurementTask
                uint32_t measurementTask_StackSize = 512 * 4;
                if ((e = mapfile_find_mem_entry(pmap, "measurementTaskHandle")) != NULL) {
                    printf("&measurementTaskHandle=0x%08x\n", e->addr);
                    uint32_t measurementTaskHandle = dump_get_ui32(pdump, e->addr);
                    printf("measurementTaskHandle=0x%08x\n", measurementTaskHandle);
                    dump_tcb_t *pmeasurementTask_TCB = (dump_tcb_t *)(pdump->ram + (measurementTaskHandle - DUMP_RAM_ADDR));
                    printf(" TCB.pcTaskName=%s\n", pmeasurementTask_TCB->pcTaskName);
                    printf(" TCB.pxTopOfStack=0x%08x\n", pmeasurementTask_TCB->pxTopOfStack);
                    printf(" TCB.pxStack=0x%08x\n", pmeasurementTask_TCB->pxStack);
                    uint32_t measurementTask_StackDepth = pmeasurementTask_TCB->pxTopOfStack - pmeasurementTask_TCB->pxStack;
                    printf(" StackDepth=0x%08x (%d%%)\n", measurementTask_StackDepth, 100 * (measurementTask_StackSize - measurementTask_StackDepth) / measurementTask_StackSize);
                    uint32_t addr;
                    for (addr = pmeasurementTask_TCB->pxStack; addr < (pmeasurementTask_TCB->pxStack + measurementTask_StackSize); addr++)
                        if (pdump->ram[addr - DUMP_RAM_ADDR] != 0xa5)
                            break;
                    uint32_t measurementTask_StackMaxDepth = addr - pmeasurementTask_TCB->pxStack;
                    printf(" StackMaxDepth=0x%08x (%d%%)\n", measurementTask_StackMaxDepth, 100 * (measurementTask_StackSize - measurementTask_StackMaxDepth) / measurementTask_StackSize);
                }
            }
        }

        if (pmap)
            mapfile_free(pmap);

        if (pdump)
            dump_free(pdump);
    } else { // print help in case of some error
        printf("dumpread - utility for working with dump.bin file\n");
        printf(" arguments:\n");
        printf("  -dump=DUMP_FILENAME  source dump.bin file\n");
        printf("  -map=MAP_FILENAME    map file\n");
        printf("  -bin=BIN_FILENAME    bin file\n");
        printf("  -out=OUT_DIRECTORY   output directory\n");
    }

    getchar();

    return ret;
}
