#include "wui_helper_funcs.h"
#include "jsmn.h"
#include "wui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "eeprom.h"
#include "ip4_addr.h"

extern osMessageQId wui_queue; // input queue (uint8_t)
extern osSemaphoreId wui_sema; // semaphore handle

char buffer[MAX_REQ_BODY_SIZE] = "";
static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void send_request_to_server(const char *request) {
    size_t req_len = strlen(request);
    osMessageQId queue = 0;

    osSemaphoreWait(wui_sema, osWaitForever); // lock
    if ((queue = wui_queue) != 0)             // queue valid
    {
        while (req_len) {
            int end, i;
            uint32_t q_space = osMessageAvailableSpace(queue);
            if (q_space >= 1) {
                if (q_space < req_len) {
                    end = q_space;
                    req_len -= q_space;
                } else {
                    end = req_len;
                    req_len = 0;
                }
                for (i = 0; i < end; i++) {
                    osMessagePut(queue, request[i], 0);
                }
                if (request[i - 1] != '\n') {
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

void json_parse_jsmn(const char *json, uint16_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens
    char request[MAX_REQ_MARLIN_SIZE];

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return;
    }

    for (int i = 0; i < ret; i++) {
        if (json_cmp(json, &t[i], "command") == 0) {
            strncpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            request[t[i + 1].end - t[i + 1].start] = 0;
            i++;
            send_request_to_server(request);
        } else if (json_cmp(json, &t[i], "connect_ip") == 0) {
            strncpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            request[t[i + 1].end - t[i + 1].start] = 0;
            ip4_addr_t tmp_addr;
            if (ip4addr_aton(request, &tmp_addr)) {
                eeprom_set_var(EEVAR_CONNECT_IP, variant8_ui32(tmp_addr.addr));
            }
            i++;
        } else if (json_cmp(json, &t[i], "connect_key") == 0) {
            strncpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            request[t[i + 1].end - t[i + 1].start] = 0;
            eeprom_set_string(EEVAR_CONNECT_KEY_START, request, CONNECT_SEC_KEY_LEN);
            i++;
        }

        /* else if(json_cmp(json, &t[i], "axis") == 0){
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

const char *char_streamer(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, MAX_REQ_BODY_SIZE, format, args);
    va_end(args);
    return (const char *)&buffer;
}
