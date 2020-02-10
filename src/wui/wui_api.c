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
#define BDY_NO_FS_FLAGS    0   // no flags for fs_open
#define BDY_API_PRINTER_LEN     12 // length of "/api/printer" string
#define BDY_API_JOB_LEN     8   // length of "/api/job" string
#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2
// for data exchange between wui thread and HTTP thread
extern marlin_vars_t webserver_marlin_vars;
extern osMutexId wui_web_mutex_id;
static marlin_vars_t webserver_marlin_vars_copy;
// for storing /api/* data
static struct fs_file api_file;
static char _buffer[BDY_WUI_API_BUFFER_SIZE];

extern osMessageQId wui_queue; // input queue (uint8_t)
extern osSemaphoreId wui_sema; // semaphore handle

static int char_streamer(const char *format, ...) {
    int rv = 0;
    va_list args;
    va_start(args, format);
    rv = vsnprintf(_buffer, BDY_WUI_API_BUFFER_SIZE, format, args);
    va_end(args);
    return rv;
}

static void wui_api_job(struct fs_file* file) {

    const char* file_name = "test.gcode";
    uint8_t sd_percent_done = (uint8_t)(webserver_marlin_vars_copy.sd_percent_done);
    uint32_t print_duration = (uint32_t)(webserver_marlin_vars_copy.print_duration);

    int response_len = char_streamer("{"
            "\"file\":\"%s\","
            "\"total_print_time\":%d, "
            "\"progress\":{\"precent_done\":%d}"
            "}",
            file_name,
            print_duration,
            sd_percent_done
            );
    file->len = response_len;
    file->data = (const char*)&_buffer;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS;
}

static void wui_api_printer(struct fs_file* file) {

    int32_t actual_nozzle = (int32_t) (webserver_marlin_vars_copy.temp_nozzle);
    int32_t target_nozzle = (int32_t) (webserver_marlin_vars_copy.target_nozzle);
    int32_t actual_heatbed = (int32_t) (webserver_marlin_vars_copy.temp_bed);
    int32_t target_heatbed = (int32_t) (webserver_marlin_vars_copy.target_bed);

    double x_pos_mm = (double) webserver_marlin_vars_copy.pos[X_AXIS_POS];
    double y_pos_mm = (double) webserver_marlin_vars_copy.pos[Y_AXIS_POS];
    double z_pos_mm = (double) webserver_marlin_vars_copy.pos[Z_AXIS_POS];
    uint16_t print_speed = (uint16_t) (webserver_marlin_vars_copy.print_speed);
    uint16_t flow_factor = (uint16_t) (webserver_marlin_vars_copy.flow_factor);
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
    file->data = (const char*)&_buffer;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
}

struct fs_file* wui_api_main(char* uri,struct fs_file* file ) {

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
    if (!strncmp(uri, "/api/printer", BDY_API_PRINTER_LEN) && (BDY_API_PRINTER_LEN == strlen(uri)) ) {
        wui_api_printer(file);
        return file;
    }else if (!strncmp(uri, "/api/job", BDY_API_JOB_LEN) && (BDY_API_JOB_LEN == strlen(uri)) ) {
        wui_api_job(file);
        return file;
    }
    return NULL;
}

static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void json_parse_jsmn(const char* json, uint16_t len){
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens
    char request[MAX_MARLIN_REQUEST_LEN];

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t)/sizeof(jsmntok_t));

    if(ret < 1 || t[0].type != JSMN_OBJECT){
        // Fail to parse JSON or top element is not an object
        return;
    }

    for(int i = 0; i < ret; i++){
        if(json_cmp(json, &t[i], "command") == 0){
            strncpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            request[t[i + 1].end - t[i + 1].start] = 0;
            i++;
            send_request_to_server(request);
        }/* else if(json_cmp(json, &t[i], "axis") == 0){
            if(t[i + 1].type != JSMN_ARRAY){
                continue;
            }
            strncat(request, " ", 1);
            int j;
            for(j = 0; j < t[i + 1].size; j++){
                strncat(request, json + t[i + j + 2].start, 1);
            }
            i += j + 1; //array token (1) + array size (j-1) + axis (1)
        }*/
    }
}

void send_request_to_server(const char * request){
    size_t req_len = strlen(request);
    osMessageQId queue = 0;

    osSemaphoreWait(wui_sema, osWaitForever); // lock
    if ((queue = wui_queue) != 0) // queue valid
    {
        while (req_len){
            int end, i;
            uint32_t q_space = osMessageAvailableSpace(queue);
            if(q_space >= 1){
                if(q_space < req_len){
                    end = q_space;
                    req_len -= q_space;
                } else {
                    end = req_len;
                    req_len = 0;
                }
                for(i = 0; i < end; i++){
                    osMessagePut(queue, request[i], 0);
                }
                if(request[i - 1] != '\n'){
                    osMessagePut(queue, '\n', 0);
                }
            } else {
                osSemaphoreRelease(wui_sema); // unlock
                osDelay(10);
                osSemaphoreWait(wui_sema, osWaitForever); //lock
            }
        }
    }
    osSemaphoreRelease(wui_sema); //unlock
}
