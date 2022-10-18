//dumpread - main.c

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crash_dump/dump.h>
#include "mapfile.h"

dump_t *pdump = 0;
mapfile_t *pmap = 0;
uint8_t *pbin = 0;

mapfile_mem_entry_t *_print_ui32(const char *name) {
    return dump_print_var_ui32(pdump, pmap, name);
}

mapfile_mem_entry_t *_print_pchar(const char *name) {
    return dump_print_var_pchar(pdump, pmap, name);
}

int main(int argc, char **argv) {
    int ret = 0;

    char dump_fn[MAX_PATH] = "dump.bin";
    char map_fn[MAX_PATH] = "firmware.map";
    char bin_fn[MAX_PATH] = "firmware.bin";
    char out_dir[MAX_PATH] = "";

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
        if (strlen(dump_fn)) {
            pdump = dump_load(dump_fn);
            if (pdump != NULL)
                fprintf(stdout, "DUMP file '%s' loaded - OK.\n", dump_fn);
            else
                fprintf(stderr, "DUMP file '%s' load error - NG!\n", dump_fn);
        }

        // load map file
        if (strlen(map_fn)) {
            pmap = mapfile_load(map_fn);
            if (pmap != NULL)
                fprintf(stdout, "MAP file '%s' loaded - OK.\n", map_fn);
            else
                fprintf(stderr, "MAP file '%s' load error - NG!\n", map_fn);
        }

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
            dump_print_all(pdump, pmap);
            /*
            //uint32_t pattern = 0x432bbae1;
            //uint32_t pattern = 0x00a02517;
            //uint32_t pattern = 0x00a02508;
            //uint32_t pattern = 0x00a02513;
            uint32_t addr = 0x20000000;
            char name[128];
            uint32_t offs;
            while (addr != 0xffffffff) {
                addr = dump_find_in_ram(pdump, (uint8_t *)(&pattern), 4, addr, 0x2001ffff);
                if (addr != 0xffffffff) {
                    printf("addr = 0x%08x", addr);
                    if (dump_resolve_addr(pdump, pmap, addr, name, &offs))
                        printf("(%s + %u)", name, offs);
                    printf("\n");
                    addr += 4;
                }
            }*/

            puts("");
        }

        if (pbin)
            free(pbin);

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

    fflush(stdout);

    //    getchar();

    return ret;
}
