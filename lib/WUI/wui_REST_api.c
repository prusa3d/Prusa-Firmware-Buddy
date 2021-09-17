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
        "\"operational\": %ld,"
        "\"paused\": %ld,"
        "\"printing\": %ld,"
        "\"cancelling\": %ld,"
        "\"pausing\": %ld,"
        "\"sdReady\": %ld,"
        "\"error\": %ld,"
        "\"ready\": %ld,"
        "\"closedOrError\": %ld,"
        "\"busy\": %ld"
        "}"
        "}"
        "}",
        filament_material,
        (int)vars->temp_nozzle, (int)((abs(vars->temp_nozzle - (int)vars->temp_nozzle)) * 10),
        (int)vars->temp_bed, (int)((abs(vars->temp_bed - (int)vars->temp_bed)) * 10),
        operational, paused, printing, cancelling, pausing, sd_ready,
        error, ready, closed_on_error, busy);
}

void get_version(char *data, const uint32_t buf_len) {
    snprintf(data, buf_len,
        "{"
        "\"api\": \"0.1\","
        "\"server\": \"2.0.0\","
        "\"text\": \"OctoPrint 1.1.1\","
        "\"hostname\": \"prusa-mini\""
        "}");
}

void get_job(char *data, const uint32_t buf_len) {
    marlin_vars_t *vars = marlin_vars();

    marlin_client_loop();

    snprintf(data, buf_len,
        "{"
        "\"job\":{"
        "\"estimatedPrintTime\":%ld,"
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
        "\"printTime\":%ld,"
        "\"printTimeLeft\":%ld,"
        "\"pos_z_mm\":%d.%.3d,"
        "\"printSpeed\":%d,"
        "\"flow_factor\":%d,"
        "\"filament_status\":3"
        "},"
        "\"filament\":{"
        "\"length\":3,"
        "\"volume\":5.333333333333333"
        "}"
        "}",
        vars->time_to_end, vars->media_LFN, vars->media_LFN, 0UL,
        (int)(vars->sd_percent_done == 100), (int)(vars->sd_percent_done % 100), 0UL, vars->print_duration, vars->time_to_end,
        (int)vars->pos[MARLIN_VAR_INDEX_Z], (int)((vars->pos[MARLIN_VAR_INDEX_Z] - (int)vars->pos[MARLIN_VAR_INDEX_Z]) * 1000),
        vars->print_speed, vars->flow_factor);
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
        "\"done\": %ld"
        "}",
        filename, (uint32_t)!start_print);
}
