/*
 * wui_REST_api.c
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_REST_api.h"
#include "wui.h"
#include "filament.h" //get_selected_filament_name
#include <string.h>
#include "wui_vars.h"
#include "marlin_vars.h"
#include "ff.h"

#include "print_utils.hpp"

// for data exchange between wui thread and HTTP thread
static wui_vars_t wui_vars_copy;
static FIL upload_file;
static uint32_t start_print = 0;
static char filename[FILE_NAME_MAX_LEN];

void get_printer(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

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

    switch (wui_vars_copy.print_state) {
    case mpsPrinting:
        if (wui_vars_copy.time_to_end && wui_vars_copy.time_to_end != (-1UL)) {
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
        (int)wui_vars_copy.temp_nozzle, (int)((abs(wui_vars_copy.temp_nozzle - (int)wui_vars_copy.temp_nozzle)) * 10),
        (int)wui_vars_copy.temp_bed, (int)((abs(wui_vars_copy.temp_bed - (int)wui_vars_copy.temp_bed)) * 10),
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

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

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
        wui_vars_copy.time_to_end, wui_vars_copy.gcode_name, wui_vars_copy.gcode_name, 0UL,
        (int)(wui_vars_copy.sd_precent_done == 100), (int)(wui_vars_copy.sd_precent_done % 100), 0UL, wui_vars_copy.print_dur, wui_vars_copy.time_to_end,
        (int)wui_vars_copy.pos[Z_AXIS_POS], (int)((wui_vars_copy.pos[Z_AXIS_POS] - (int)wui_vars_copy.pos[Z_AXIS_POS]) * 1000),
        wui_vars_copy.print_speed, wui_vars_copy.flow_factor);
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

uint32_t wui_upload_begin(const char *filename) {
    return f_open(&upload_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
}

uint32_t wui_upload_data(const char *data, uint32_t length) {
    UINT written;
    f_write(&upload_file, data, length, &written);
    return written;
}

uint32_t wui_upload_finish(const char *old_filename, const char *new_filename, uint32_t start) {
    f_close(&upload_file);

    if (!strstr(new_filename, "gcode")) {
        f_unlink(old_filename);
        return 415;
    }

    if (f_rename(old_filename, new_filename) != FR_OK) {
        f_unlink(old_filename);
        return 409;
    } else {
        strcpy(filename, new_filename);
    }

    if (wui_vars.sd_printing && start) {
        return 409;
    } else {
        start_print = start;
        print_begin(new_filename);
    }

    return 200;
}
