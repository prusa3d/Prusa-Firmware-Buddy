// dump_marlinapi.c

#include "dump_marlinapi.h"
#include <stdlib.h>
#include <string.h>
#include "dump_rtos.h"



void dump_marlinapi_print(dump_t *pd, mapfile_t *pm) {
    if (!pm)
        return;
    printf("\nMarlin_server\n");
    mapfile_mem_entry_t *e;
    uint32_t marlin_server_queue = 0;
    if ((e = dump_print_var(pd, pm, "marlin_server_queue")) != NULL) {
    	marlin_server_queue = dump_get_ui32(pd, e->addr);
    	dump_rtos_print_queue(pd, pm, marlin_server_queue);
    }
}
