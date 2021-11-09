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

static marlin_print_state_t cached_state = mpsIdle;

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
        if (vars->time_to_end != (-1UL)) {
            printing = busy = 1;
            ready = operational = 0;
        }
        cached_state = vars->print_state;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        paused = busy = 1;
        ready = operational = 0;
        cached_state = vars->print_state;
        break;
    case mpsPaused:
        paused = 1;
        cached_state = vars->print_state;
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead:
        ready = operational = 0;
        busy = printing = 1;
        cached_state = vars->print_state;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        cancelling = busy = 1;
        ready = operational = 0;
        cached_state = vars->print_state;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        busy = 1;
        ready = operational = 0;
        cached_state = vars->print_state;
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsIdle:
    default:
        if (cached_state != vars->print_state) {
            busy = 1;
            ready = operational = 0;
        }
        break;
    }

    snprintf(data, buf_len,
        "{"
        "\"telemetry\": {"
        "\"material\": \"%s\""
        "},"
        "\"temperature\": {"
        "\"tool0\": {"
        "\"actual\": %d.%.1d,"
        "\"target\": 0,"
        "\"offset\": 0"
        "},"
        "\"bed\": {"
        "\"actual\": %d.%.1d,"
        "\"target\": 0,"
        "\"offset\": 0"
        "},"
        "\"chamber\": {"
        "\"actual\": 0,"
        "\"target\": 0,"
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
        filament_material,
        (int)vars->temp_nozzle, abs((int)(vars->temp_nozzle * 10) % 10),
        (int)vars->temp_bed, abs((int)(vars->temp_bed * 10) % 10),
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
    if (vars->sd_percent_done == 100) {
        cached_state = mpsFinished;
    }
}

void get_files(char *data, const uint32_t buf_len) {

    snprintf(data, buf_len,
        "{"
        "\"files\": {"
        "\"local\": {"
        "\"name\": \"%s\","
        "\"origin\": \"local\","
        "}"
        "},"
        "\"done\": %d"
        "}",
        filename, (int)!start_print);
}
