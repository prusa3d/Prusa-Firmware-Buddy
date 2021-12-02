/*
 * wui_REST_api.c
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
 */

#include "wui_REST_api.h"
#include "wui_api.h"
#include "filament.h" //get_selected_filament_name
#include "marlin_client.h"
#include "lwip/init.h"
#include "netdev.h"

#include <string.h>
#include <stdio.h>

extern uint32_t start_print;
extern char *filename;

void get_printer(char *data, const uint32_t buf_len) {
    marlin_vars_t *vars = marlin_vars();
    const char *filament_material = get_selected_filament_name();

    uint32_t operational = 1;
    uint32_t paused = 0;
    uint32_t printing = 0;
    uint32_t cancelling = 0;
    uint32_t pausing = 0;
    uint32_t sd_ready = 1;
    uint32_t error = 0;
    uint32_t ready = 1;
    uint32_t closed_on_error = 0;
    uint32_t busy = 0;

    marlin_client_loop();

    switch (vars->print_state) {
    case mpsPrinting:
        if (vars->time_to_end && vars->time_to_end != (-1UL)) {
            printing = busy = 1;
            ready = operational = 0;
        }
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        paused = busy = 1;
        ready = operational = 0;
        break;
    case mpsPaused:
        paused = 1;
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead:
        ready = operational = 0;
        busy = printing = 1;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        cancelling = busy = 1;
        ready = operational = 0;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        busy = 1;
        ready = operational = 0;
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsIdle:
    default:
        break;
    }

    snprintf(data, buf_len,
        "{"
        "\"telemetry\": {"
        "\"temp-bed\": %.1f,"
        "\"temp-nozzle\": %.1f,"
        "\"print-speed\": %d,"
        "\"z-height\": %.1f,"
        "\"material\": \"%s\""
        "},"
        "\"temperature\": {"
        "\"tool0\": {"
        "\"actual\": %.1f,"
        "\"target\": %.1f,"
        "\"offset\": 0"
        "},"
        "\"bed\": {"
        "\"actual\": %.1f,"
        "\"target\": %.1f,"
        "\"offset\": 0"
        "}"
        "},"
        "\"sd\": {"
        "\"ready\": 1"
        "},"
        "\"state\": {"
        "\"text\": \"Operational\","
        "\"flags\": {"
        "\"operational\": %" PRIu32 ","
        "\"paused\": %" PRIu32 ","
        "\"printing\": %" PRIu32 ","
        "\"cancelling\": %" PRIu32 ","
        "\"pausing\": %" PRIu32 ","
        "\"sdReady\": %" PRIu32 ","
        "\"error\": %" PRIu32 ","
        "\"ready\": %" PRIu32 ","
        "\"closedOrError\": %" PRIu32 ","
        "\"busy\": %" PRIu32
        "}"
        "}"
        "}",
        (double)vars->temp_bed,
        (double)vars->temp_nozzle,
        (int)vars->print_speed,
        (double)vars->pos[2], // XYZE, mm
        filament_material,
        (double)vars->temp_nozzle,
        // Sometimes, the GUI lies about the target temperature (eg. when
        // preheating). Let's keep the company and lie too..
        (double)vars->display_nozzle,
        (double)vars->temp_bed,
        (double)vars->target_bed,

        // TODO: Format as bools to be according to the spec
        operational, paused, printing, cancelling, pausing, sd_ready,
        error, ready, closed_on_error, busy);
}

void get_version(char *data, const uint32_t buf_len) {

    snprintf(data, buf_len,
        "{"
        "\"api\": \"%s\","
        "\"server\": \"%s\","
        "\"text\": \"PrusaLink MINI\","
        "\"hostname\": \"%s\""
        "}",
        PL_VERSION_STRING, LWIP_VERSION_STRING, netdev_get_hostname(netdev_get_active_id()));
}

void get_job(char *data, const uint32_t buf_len) {
    marlin_vars_t *vars = marlin_vars();

    marlin_client_loop();

    snprintf(data, buf_len,
        "{"
        "\"job\":{"
        "\"estimatedPrintTime\":%" PRIu32 ","
        "\"file\":{"
        "\"date\":null,"
        "\"name\":\"%s\","
        "\"origin\":\"USB\","
        "\"path\":\"%s\","
        "\"size\":%ld"
        "}"
        "},"
        "\"state\":\"Printing\","
        "\"progress\":{"
        "\"completion\":%d.%.2d,"
        "\"filepos\":%ld,"
        "\"printTime\":%" PRIu32 ","
        "\"printTimeLeft\":%" PRIu32 ","
        "\"pos_z_mm\":%d.%.3d,"
        "\"printSpeed\":%" PRIu16 ","
        "\"flow_factor\":%" PRIu16 ","
        "\"filament_status\":3"
        "},"
        "\"filament\":{"
        "\"length\":3,"
        "\"volume\":5.33"
        "}"
        "}",
        vars->time_to_end, vars->media_LFN, vars->media_LFN, 0UL,
        (int)(vars->sd_percent_done == 100), (int)(vars->sd_percent_done % 100), 0UL, vars->print_duration, vars->time_to_end,
        (int)vars->pos[MARLIN_VAR_INDEX_Z], (int)((vars->pos[MARLIN_VAR_INDEX_Z] - (int)vars->pos[MARLIN_VAR_INDEX_Z]) * 1000),
        vars->print_speed, vars->flow_factor);
}

void get_files(char *data, const uint32_t buf_len) {

    /*
     * Warning: This is a hack/stub.
     *
     * * This is being used both to answer /api/files _and_ as the content of the post GCODE.
     * * We don't have a way to stream the body (eg. generate on the fly as we
     *   are iterating through the files on the flash drive).
     * * This is missing a lot of fields in the files.
     *
     * This probably depends on first getting an actual HTTP server.
     */

    snprintf(data, buf_len,
        "{"
        "\"files\": [{"
        "\"local\": {"
        "\"name\": \"%s\","
        "\"origin\": \"local\""
        "}"
        "}],"
        "\"done\": %d"
        "}",
        filename, (int)!start_print);
}
