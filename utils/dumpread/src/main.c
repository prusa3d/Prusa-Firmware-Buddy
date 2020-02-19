//dumpread - main.c

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dump.h"
#include "mapfile.h"

#define _TEST
//#define _PRINTER "MI"
//#define _PRINTER "MQ"
#define _PRINTER "ND"
//#define _PRINTER "NH"

int main(int argc, char **argv) {
    int ret = 0;

    char dump_fn[MAX_PATH] = "dump.bin";
    char map_fn[MAX_PATH] = "firmware.map";
    char bin_fn[MAX_PATH] = "firmware.bin";
    char out_dir[MAX_PATH] = "";
    dump_t *pdump = 0;
    mapfile_t *pmap = 0;
    uint8_t *pbin = 0;

#ifdef _TEST

    strcpy(dump_fn, "d:\\Projects\\Prusa3D\\STM32\\PFW-Buddy-HARDFAULT\\DUMP_FW194\\"_PRINTER
                    "\\Dump\\dump.bin");
    strcpy(map_fn, "d:\\Projects\\Prusa3D\\STM32\\PFW-Buddy-HARDFAULT\\DUMP_FW194\\Firmware\\firmware.map");
    strcpy(bin_fn, "d:\\Projects\\Prusa3D\\STM32\\PFW-Buddy-HARDFAULT\\DUMP_FW194\\Firmware\\firmware.bin");
    strcpy(out_dir, "d:\\Projects\\Prusa3D\\STM32\\PFW-Buddy-HARDFAULT\\DUMP_FW194\\"_PRINTER
                    "\\Dump\\");

#else //_TEST

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
    //check args
    if ((ret == 0) && (strlen(dump_fn) == 0)) {
        fputs("DUMP_FILENAME not defined!\n", stderr);
        ret = 1;
    }
    //    if ((ret == 0) && (strlen(map_fn) == 0))
    //        { fputs("MAP_FILENAME not defined!\n", stderr); ret = 1; }
    //    if ((ret == 0) && (strlen(bin_fn) == 0))
    //        { fputs("BIN_FILENAME not defined!\n", stderr); ret = 1; }
    //    if ((ret == 0) && (strlen(bin_fn) == 0))
    //        { fputs("BIN_FILENAME not defined!\n", stderr); ret = 1; }
    if (ret != 0) {
        printf("dumpread - utility for working with dump.bin file\n");
        printf(" arguments:\n");
        printf("  -dump=DUMP_FILENAME  source dump.bin file\n");
        printf("  -map=MAP_FILENAME    map file\n");
        printf("  -bin=BIN_FILENAME    bin file\n");
        printf("  -out=OUT_DIRECTORY   output directory\n");
        getchar();
    }
    if (ret == 0) {
    }

#endif

    if (strlen(dump_fn))
        pdump = dump_load(dump_fn);

    if (strlen(map_fn))
        pmap = mapfile_load(map_fn);

    if (strlen(bin_fn)) {
        pbin = (uint8_t *)malloc(DUMP_FLASH_SIZE);
        dump_load_bin_from_file(pbin, DUMP_FLASH_SIZE, bin_fn);
    }

    if (pdump) {
        //        dump_print_hardfault_simple(pdump);
        dump_print_hardfault_detail(pdump);
        //        dump_save_all_sections(pdump, out_dir);

        if (pmap) {
            //test
            mapfile_mem_entry_t *e;

            e = mapfile_find_mem_entry(pmap, "_Balloc");
            printf("<_Balloc> 0x%08x\n", e->addr);

            e = mapfile_find_mem_entry(pmap, "xTickCount");
            uint32_t xTickCount = dump_get_ui32(pdump, e->addr);
            printf("xTickCount=%u ms (%u hours)\n", xTickCount, xTickCount / (1000 * 60 * 60));

            e = mapfile_find_mem_entry(pmap, "uwTick");
            uint32_t uwTick = dump_get_ui32(pdump, e->addr);
            printf("uwTick=%u ms (%u hours)\n", uwTick, uwTick / (1000 * 60 * 60));

            e = mapfile_find_mem_entry(pmap, "defaultTaskHandle");
            printf("defaultTaskHandle 0x%08x\n", e->addr);
            uint32_t defaultTaskHandle = dump_get_ui32(pdump, e->addr);
            printf("defaultTaskHandle=0x%08x\n", defaultTaskHandle);

            e = mapfile_find_mem_entry(pmap, "displayTaskHandle");
            printf("displayTaskHandle 0x%08x\n", e->addr);
            uint32_t displayTaskHandle = dump_get_ui32(pdump, e->addr);
            printf("displayTaskHandle=0x%08x\n", displayTaskHandle);
        }
    }

    if (pmap)
        mapfile_free(pmap);

    if (pdump)
        dump_free(pdump);

    printf("ready\n");

    fflush(stdout);
    getchar();

    return ret;
}
