/*
 * wui_REST_api.c
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_REST_api.h"
#include "wui_api.h"
#include "wui.h"
#include "filament.h" //get_selected_filament_name
#include <string.h>
#include "wui_vars.h"
#include "eeprom.h"

#define BDY_WUI_API_BUFFER_SIZE 512

// for data exchange between wui thread and HTTP thread
static wui_vars_t wui_vars_copy;

static void print_dur_to_string(char *buffer, size_t buffer_len, uint32_t print_dur) {
    int d = ((print_dur / 60) / 60) / 24,
        h = ((print_dur / 60) / 60) % 24,
        m = (print_dur / 60) % 60,
        s = print_dur % 60;

    if (d) {
        snprintf(buffer, buffer_len, "%3id %2ih %2im", d, h, m);
    } else if (h) {
        snprintf(buffer, buffer_len, "     %2ih %2im", h, m);
    } else if (m) {
        snprintf(buffer, buffer_len, "     %2im %2is", m, s);
    } else {
        snprintf(buffer, buffer_len, "         %2is", s);
    }
}

void get_telemetry_for_local(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    int32_t actual_nozzle = (int32_t)(wui_vars_copy.temp_nozzle);
    int32_t actual_heatbed = (int32_t)(wui_vars_copy.temp_bed);
    double z_pos_mm = (double)wui_vars_copy.pos[Z_AXIS_POS];
    uint16_t print_speed = (uint16_t)(wui_vars_copy.print_speed);
    uint16_t flow_factor = (uint16_t)(wui_vars_copy.flow_factor);
    const char *filament_material = get_selected_filament_name();
    int8_t time_zone = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));

    if (!wui_vars_copy.sd_printing) {
        snprintf(data, buf_len, "{"
                                "\"temp_nozzle\":%ld,"
                                "\"temp_bed\":%ld,"
                                "\"material\":\"%s\","
                                "\"pos_z_mm\":%.2f,"
                                "\"printing_speed\":%d,"
                                "\"flow_factor\":%d"
                                "}",
            actual_nozzle, actual_heatbed, filament_material,
            z_pos_mm, print_speed, flow_factor);
        return;
    }

    char print_time[15];
    uint32_t time_to_end = wui_vars_copy.time_to_end;

    if (wui_vars_copy.time_to_end == TIME_TO_END_INVALID) {
        time_to_end = 0;
    }

    print_dur_to_string(print_time, sizeof(print_time), wui_vars_copy.print_dur);

    snprintf(data, buf_len, "{"
                            "\"temp_nozzle\":%ld,"
                            "\"temp_bed\":%ld,"
                            "\"material\":\"%s\","
                            "\"pos_z_mm\":%.2f,"
                            "\"printing_speed\":%d,"
                            "\"flow_factor\":%d,"
                            "\"progress\":%d,"
                            "\"print_dur\":\"%s\","
                            "\"time_est\":\"%lu\","
                            "\"time_zone\":\"%d\","
                            "\"project_name\":\"%s\""
                            "}",
        actual_nozzle, actual_heatbed, filament_material,
        z_pos_mm, print_speed, flow_factor, wui_vars_copy.sd_precent_done,
        print_time, time_to_end, time_zone, wui_vars_copy.gcode_name);
}
