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

#include "cmsis_os.h"
#include "stdarg.h"

#define BDY_WUI_API_BUFFER_SIZE 512
#define BDY_NO_FS_FLAGS         0  // no flags for fs_open
#define BDY_API_PRINTER_LEN     12 // length of "/api/printer" string
#define BDY_API_JOB_LEN         8  // length of "/api/job" string
#define X_AXIS_POS              0
#define Y_AXIS_POS              1
#define Z_AXIS_POS              2

static marlin_vars_t webserver_marlin_vars_copy;
// for storing /api/* data
static struct fs_file api_file;
static char _buffer[BDY_WUI_API_BUFFER_SIZE];

static int char_streamer(const char *format, ...) {
    int rv = 0;
    va_list args;
    va_start(args, format);
    rv = vsnprintf(_buffer, BDY_WUI_API_BUFFER_SIZE, format, args);
    va_end(args);
    return rv;
}

static void wui_api_job(struct fs_file *file) {

    const char *file_name = "test.gcode";
    uint8_t sd_percent_done = (uint8_t)(webserver_marlin_vars_copy.sd_percent_done);
    uint32_t print_duration = (uint32_t)(webserver_marlin_vars_copy.print_duration);

    int response_len = char_streamer("{"
                                     "\"file\":\"%s\","
                                     "\"total_print_time\":%d, "
                                     "\"progress\":{\"precent_done\":%d}"
                                     "}",
        file_name,
        print_duration,
        sd_percent_done);
    file->len = response_len;
    file->data = (const char *)&_buffer;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS;
}

static void wui_api_printer(struct fs_file *file) {

    int32_t actual_nozzle = (int32_t)(webserver_marlin_vars_copy.temp_nozzle);
    int32_t target_nozzle = (int32_t)(webserver_marlin_vars_copy.target_nozzle);
    int32_t actual_heatbed = (int32_t)(webserver_marlin_vars_copy.temp_bed);
    int32_t target_heatbed = (int32_t)(webserver_marlin_vars_copy.target_bed);

    double x_pos_mm = (double)webserver_marlin_vars_copy.pos[X_AXIS_POS];
    double y_pos_mm = (double)webserver_marlin_vars_copy.pos[Y_AXIS_POS];
    double z_pos_mm = (double)webserver_marlin_vars_copy.pos[Z_AXIS_POS];
    uint16_t print_speed = (uint16_t)(webserver_marlin_vars_copy.print_speed);
    uint16_t flow_factor = (uint16_t)(webserver_marlin_vars_copy.flow_factor);
    const char *filament_material = filaments[get_filament()].name;

    int response_len = char_streamer(
        "{\"temperature\":{"
        "\"tool0\":{\"actual\":%d, \"target\":%d},"
        "\"bed\":{\"actual\":%d, \"target\":%d}},"
        "\"xyz_pos_mm\":{"
        "\"x\":%.2f, \"y\":%.2f, \"z\":%.2f},"
        "\"print_settings\":{"
        "\"printing_speed\":%hd, \"flow_factor\":%hd, \"filament_material\":\"%s\"} }",

        actual_nozzle, target_nozzle, actual_heatbed, target_heatbed,
        x_pos_mm, y_pos_mm, z_pos_mm, print_speed, flow_factor,
        filament_material);
    file->len = response_len;
    file->data = (const char *)&_buffer;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
}

struct fs_file *wui_api_main(char *uri, struct fs_file *file) {

    osStatus status = osMutexWait(wui_web_mutex_id, osWaitForever);
    if (status == osOK) {
        webserver_marlin_vars_copy = webserver_marlin_vars;
    }
    osMutexRelease(wui_web_mutex_id);
    file = &api_file;
    file->len = 0;
    file->data = NULL;
    file->index = 0;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
    if (!strncmp(uri, "/api/printer", BDY_API_PRINTER_LEN) && (BDY_API_PRINTER_LEN == strlen(uri))) {
        wui_api_printer(file);
        return file;
    } else if (!strncmp(uri, "/api/job", BDY_API_JOB_LEN) && (BDY_API_JOB_LEN == strlen(uri))) {
        wui_api_job(file);
        return file;
    }
    return NULL;
}
