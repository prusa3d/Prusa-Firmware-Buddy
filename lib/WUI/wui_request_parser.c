#include "wui_request_parser.h"
#include "wui_helper_funcs.h"
#include <string.h>
#include "jsmn.h"
#include "dbg.h"

#define CMD_LIMIT 10 // number of commands accepted in low level command response

#define MAX_ACK_SIZE 16

static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

uint32_t httpd_json_parser(char *json, uint32_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return 0;
    }

    for (int i = 1; i < ret; i++) {
        if (t[i].size >= MAX_REQ_MARLIN_SIZE) {
            // Request is too long
            return 0;
        }
    }

    for (int i = 0; i < ret; i++) {
        wui_cmd_t request;
        if (json_cmp(json, &t[i], "command") == 0) {
            strlcpy(request.arg, json + t[i + 1].start, t[i + 1].size + 1);
            request.lvl = LOW_LVL_CMD;
            i++;
            _dbg("command received: %s", request.arg);
            send_request_to_wui(&request);
        }
    }
    return 1;
}
