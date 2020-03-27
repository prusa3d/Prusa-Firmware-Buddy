/*
 * wui_api.c
 * \brief
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_api.h"

#include "wui.h"
#include "filament.h"
#include "wui_vars.h"
#include "progress_data_wrapper.h"

#include "stdarg.h"

#define BDY_WUI_API_BUFFER_SIZE 512
#define BDY_NO_FS_FLAGS         0  // no flags for fs_open
#define BDY_API_TELEMETRY_LEN   14 // length of "/api/telemetry" string

// for data exchange between wui thread and HTTP thread
static web_vars_t web_vars_copy;
// for storing /api/* data
static struct fs_file api_file;

const char *get_update_str(const char *header) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        web_vars_copy = web_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    int32_t actual_nozzle = (int32_t)(web_vars_copy.temp_nozzle);
    int32_t actual_heatbed = (int32_t)(web_vars_copy.temp_bed);
    double z_pos_mm = (double)web_vars_copy.pos[Z_AXIS_POS];
    uint16_t print_speed = (uint16_t)(web_vars_copy.print_speed);
    uint16_t flow_factor = (uint16_t)(web_vars_copy.flow_factor);
    const char *filament_material = filaments[get_filament()].name;

    if (!web_vars_copy.sd_printing) {
        return char_streamer("%s{"
                             "\"temp_nozzle\":%d,"
                             "\"temp_bed\":%d,"
                             "\"material\":\"%s\","
                             "\"pos_z_mm\":%.2f,"
                             "\"printing_speed\":%d,"
                             "\"flow_factor\":%d"
                             "}",
            header,
            actual_nozzle, actual_heatbed, filament_material,
            z_pos_mm, print_speed, flow_factor);
    }

    uint8_t percent_done;
    char time_2_end[9], print_time[15];
    if (is_percentage_valid(web_vars_copy.print_dur)) {
        percent_done = progress_get_percentage();
        progress_format_time2end(time_2_end, web_vars_copy.print_speed);
    } else {
        strlcpy(time_2_end, "N/A", 4);
        percent_done = web_vars_copy.sd_precent_done;
    }

    print_dur_to_string(print_time, sizeof(print_time), web_vars_copy.print_dur);

    return char_streamer("%s{"
                         "\"temp_nozzle\":%d,"
                         "\"temp_bed\":%d,"
                         "\"material\":\"%s\","
                         "\"pos_z_mm\":%.2f,"
                         "\"printing_speed\":%d,"
                         "\"flow_factor\":%d,"
                         "\"progress\":%d,"
                         "\"print_dur\":\"%s\","
                         "\"time_est\":\"%s\","
                         "\"project_name\":\"%s\""
                         "}",
        header,
        actual_nozzle, actual_heatbed, filament_material,
        z_pos_mm, print_speed, flow_factor,
        percent_done, print_time, time_2_end, web_vars_copy.gcode_name);
}

static void wui_api_telemetry(struct fs_file *file) {

    const char *ptr = get_update_str("");

    uint16_t response_len = strlen(ptr);
    file->len = response_len;
    file->data = ptr;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
}

struct fs_file *wui_api_main(const char *uri) {

    api_file.len = 0;
    api_file.data = NULL;
    api_file.index = 0;
    api_file.pextension = NULL;
    api_file.flags = BDY_NO_FS_FLAGS; // http server adds response header
    if (!strncmp(uri, "/api/telemetry", BDY_API_TELEMETRY_LEN) && (BDY_API_TELEMETRY_LEN == strlen(uri))) {
        wui_api_telemetry(&api_file);
        return &api_file;
    }
    return NULL;
}
