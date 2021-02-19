// dump_marlinapi.c

#include "dump_marlinapi.h"
#include <stdlib.h>
#include <string.h>
#include "dump_rtos.h"

void dump_marlinapi_print_vars(dump_t *pd, mapfile_t *pm, marlin_vars_t *pv, const char *prefix) {
    printf(" %s.motion = 0x%02x\n", prefix, pv->motion);
    printf(" %s.gqueue = 0x%02x\n", prefix, pv->gqueue);
    printf(" %s.pqueue = 0x%02x\n", prefix, pv->pqueue);
    printf(" %s.ipos[X] = %d\n", prefix, pv->ipos[0]);
    printf(" %s.ipos[Y] = %d\n", prefix, pv->ipos[1]);
    printf(" %s.ipos[Z] = %d\n", prefix, pv->ipos[2]);
    printf(" %s.ipos[E] = %d\n", prefix, pv->ipos[3]);
    printf(" %s.pos[X] = %f\n", prefix, pv->pos[0]);
    printf(" %s.pos[Y] = %f\n", prefix, pv->pos[1]);
    printf(" %s.pos[Z] = %f\n", prefix, pv->pos[2]);
    printf(" %s.pos[E] = %f\n", prefix, pv->pos[3]);

    printf(" %s.temp_nozzle = %f\n", prefix, pv->temp_nozzle);
    printf(" %s.temp_bed = %f\n", prefix, pv->temp_bed);
    printf(" %s.target_nozzle = %f\n", prefix, pv->target_nozzle);
    printf(" %s.target_bed = %f\n", prefix, pv->target_bed);
    printf(" %s.z_offset = %f\n", prefix, pv->z_offset);

    printf(" %s.fan_speed = %u\n", prefix, pv->fan_speed);
    printf(" %s.print_speed = %u\n", prefix, pv->print_speed);
    printf(" %s.flow_factor = %u\n", prefix, pv->flow_factor);
    printf(" %s.wait_heat = %u\n", prefix, pv->wait_heat);
    printf(" %s.wait_user = %u\n", prefix, pv->wait_user);
    printf(" %s.sd_printing = %u\n", prefix, pv->sd_printing);
    printf(" %s.sd_percent_done = %u\n", prefix, pv->sd_percent_done);

    printf(" %s.print_duration = %u\n", prefix, pv->print_duration);
    printf(" %s.media_inserted = %u\n", prefix, pv->media_inserted);
    printf(" %s.print_state = %u\n", prefix, pv->print_state);

    char *media_LFN = (char *)dump_get_data_ptr(pd, (uint32_t)pv->media_LFN);
    printf(" %s.media_LFN = %s\n", prefix, media_LFN);

    char *media_SFN_path = (char *)dump_get_data_ptr(pd, (uint32_t)pv->media_SFN_path);
    printf(" %s.media_SFN_path = %s\n", prefix, media_SFN_path);

    printf(" %s.display_nozzle = %f\n", prefix, pv->display_nozzle);
    printf(" %s.time_to_end = %u\n", prefix, pv->time_to_end);
}

void dump_marlinapi_print(dump_t *pd, mapfile_t *pm) {
    if (!pm)
        return;
    mapfile_mem_entry_t *e;
    printf("\nMarlinAPI\n");
    marlin_server_t *pmarlin_server = 0;
    if ((e = dump_print_var(pd, pm, "marlin_server")) != NULL) {
        pmarlin_server = (marlin_server_t *)dump_get_data_ptr(pd, e->addr);
        printf(" .flags = %04x\n", pmarlin_server->flags);
        for (int c = 0; c < 3; c++)
            printf(" .notify_events[%d] = 0x%016" PRIx64 "\n", c, pmarlin_server->notify_events[c]);
        for (int c = 0; c < 3; c++)
            printf(" .notify_changes[%d] = 0x%016" PRIx64 "\n", c, pmarlin_server->notify_changes[c]);
        printf(" .vars:\n");
        dump_marlinapi_print_vars(pd, pm, &(pmarlin_server->vars), " ");
        printf(" .request = '%s'\n", pmarlin_server->request);
        printf(" .request_len = %d\n", pmarlin_server->request_len);
        for (int c = 0; c < 3; c++)
            printf(" .client_events[%d] = 0x%016" PRIx64 "\n", c, pmarlin_server->client_events[c]);
        for (int c = 0; c < 3; c++)
            printf(" .client_changes[%d] = 0x%016" PRIx64 "\n", c, pmarlin_server->client_changes[c]);
        printf(" .last_update = %u\n", pmarlin_server->last_update);

        printf(" .last_update = %u\n", pmarlin_server->last_update);
        printf(" .idle_cnt = %u\n", pmarlin_server->idle_cnt);
        printf(" .pqueue_head = %u\n", pmarlin_server->pqueue_head);
        printf(" .pqueue_tail = %u\n", pmarlin_server->pqueue_tail);
        printf(" .pqueue = %u\n", pmarlin_server->pqueue);
        printf(" .gqueue = %u\n", pmarlin_server->gqueue);
        printf(" .command = %u\n", pmarlin_server->command);
        printf(" .command_begin = %u\n", pmarlin_server->command_begin);
        printf(" .command_end = %u\n", pmarlin_server->command_end);

        printf(" .mesh[%u][%u]:\n", pmarlin_server->mesh.xc, pmarlin_server->mesh.xc);
        for (int y = 0; y < pmarlin_server->mesh.yc; y++)
            for (int x = 0; x < pmarlin_server->mesh.xc; x++)
                printf("  [%u][%u] = %f\n", x, y, pmarlin_server->mesh.z[x + y * pmarlin_server->mesh.xc]);
        for (int c = 0; c < 3; c++)
            printf(" .mesh_point_notsent[%d] = 0x%016" PRIx64 "\n", c, pmarlin_server->mesh_point_notsent[c]);
        printf(" .update_vars = 0x%016" PRIx64 "\n", pmarlin_server->update_vars);
        printf(" .print_state = %u\n", pmarlin_server->print_state);
        printf(" .resume_pos[X] = %f\n", pmarlin_server->resume_pos[0]);
        printf(" .resume_pos[Y] = %f\n", pmarlin_server->resume_pos[1]);
        printf(" .resume_pos[Z] = %f\n", pmarlin_server->resume_pos[2]);
        printf(" .resume_pos[E] = %f\n", pmarlin_server->resume_pos[3]);
        printf(" .resume_nozzle_temp = %f\n", pmarlin_server->resume_nozzle_temp);
        printf(" .resume_fan_speed = %u\n", pmarlin_server->resume_fan_speed);
        printf(" .paused_ticks = %u\n", pmarlin_server->paused_ticks);
    }

    marlin_client_t *pmarlin_client = 0;
    if ((e = dump_print_var(pd, pm, "marlin_client")) != NULL) {
        pmarlin_client = (marlin_client_t *)dump_get_data_ptr(pd, e->addr);
        for (int c = 0; c < 3; c++) {
            printf(" marlin_client[%u] @x0x%08x\n", c, e->addr + c * sizeof(marlin_client_t));
            printf("  .id = %u\n", pmarlin_client[c].id);
            printf("  .flags = 0x%04x\n", pmarlin_client[c].flags);
            printf("  .events = 0x%016" PRIx64 "\n", pmarlin_client[c].events);
            printf("  .changes = 0x%016" PRIx64 "\n", pmarlin_client[c].changes);
            printf("  .vars:\n");
            dump_marlinapi_print_vars(pd, pm, &(pmarlin_client[c].vars), "  ");
            printf("  .ack = 0x%08x (%u)\n", pmarlin_client[c].ack, pmarlin_client[c].ack);
            printf("  .last_count = %u\n", pmarlin_client[c].last_count);
            printf("  .errors = 0x%016" PRIx64 "\n", pmarlin_client[c].errors);
            printf("  .mesh[%u][%u]:\n", pmarlin_client[c].mesh.xc, pmarlin_client[c].mesh.xc);
            for (int y = 0; y < pmarlin_client[c].mesh.yc; y++)
                for (int x = 0; x < pmarlin_client[c].mesh.xc; x++)
                    printf("   [%u][%u] = %f\n", x, y, pmarlin_client[c].mesh.z[x + y * pmarlin_client[c].mesh.xc]);
            printf("  .command = 0x%08x (%u)\n", pmarlin_client[c].command, pmarlin_client[c].command);
            printf("  .reheating = %u\n", pmarlin_client[c].reheating);
            printf("  .fsm_cb = 0x%08x\n", pmarlin_client[c].fsm_cb);
        }
    }

    return;

    uint32_t marlin_server_queue = 0;
    if ((e = dump_print_var(pd, pm, "marlin_server_queue")) != NULL) {
        marlin_server_queue = dump_get_ui32(pd, e->addr);
        dump_rtos_print_queue(pd, pm, marlin_server_queue);
    }
}
