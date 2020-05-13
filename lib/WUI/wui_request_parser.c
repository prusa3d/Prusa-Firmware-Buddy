#include "wui_request_parser.h"
#include "wui_helper_funcs.h"
#include <string.h>
#include "jsmn.h"
#include "dbg.h"

#define HTTP_DUBAI_HACK 0

#if HTTP_DUBAI_HACK
    #include "version.h"
#endif
#define CMD_LIMIT 10 // number of commands accepted in low level command response

#define MAX_ACK_SIZE 16

static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void httpd_json_parser(char *json, uint32_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return;
    }

    for (int i = 0; i < ret; i++) {
        wui_cmd_t request;
#if HTTP_DUBAI_HACK
        if (json_cmp(json, &t[i], project_firmware_name) == 0) {
#else
        if (json_cmp(json, &t[i], "command") == 0) {
#endif //HTTP_DUBAI_HACK
            strlcpy(request.arg, json + t[i + 1].start, (t[i + 1].end - t[i + 1].start + 1));
            request.lvl = LOW_LVL_CMD;
            i++;
            _dbg("command received: %s", request.arg);
            send_request_to_wui(&request);
        }
    }
}
