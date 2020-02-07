// http_methods.c
// implementation of GET / POST methods

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "marlin_client.h"
#include "wui.h"
#include "cmsis_os.h"
#include "jsmn.h"

#include <string.h>

#define MSG_BUFFSIZE 512
#define MAX_MARLIN_REQUEST_LEN 100

static void * current_connection;
static void * valid_connection;

extern osMessageQId wui_queue; // input queue (uint8_t)
extern osSemaphoreId wui_sema; // semaphore handle


static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void send_request_to_server(const char * request);

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

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd)
{
    //LWIP_UNUSED_ARG();
    if(!memcmp(uri, "/post_gcode.html", 16)){
        if(current_connection != connection){
            current_connection = connection;
            valid_connection = NULL;
            /* default page */
            snprintf(response_uri, response_uri_len, "/post_gcode.html");
            return ERR_OK;
        }
    }
    return ERR_VAL;
}

err_t httpd_post_receive_data(void * connection, struct pbuf * p)
{
    if(current_connection == connection){
        u16_t token_move = pbuf_memfind(p, "{", 1, 0);

        if(token_move != 0xFFFF){
            u16_t len = p->tot_len - token_move;
            if(len != 0 && len < MSG_BUFFSIZE){
                char request_buf[MSG_BUFFSIZE];
                u16_t ret = pbuf_copy_partial(p, request_buf, len, token_move);
                if(ret){
                    request_buf[ret] = 0;
                    json_parse_jsmn(request_buf, ret);
                    valid_connection = connection;
                }
            }
        }
        return ERR_OK;
    }
    return ERR_VAL;
}

void httpd_post_finished(void * connection, char * response_uri, u16_t response_uri_len)
{
    /* default page */
    snprintf(response_uri, response_uri_len, "/post_gcode.html");
    if(current_connection == connection){
        if(valid_connection == connection){
            /*receiving data succeeded*/
            snprintf(response_uri, response_uri_len, "/post_gcode.html");
        }
        current_connection = NULL;
        valid_connection = NULL;
    }
}
