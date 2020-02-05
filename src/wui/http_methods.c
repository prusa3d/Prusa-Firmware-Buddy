// http_methods.c
// implementation of GET / POST methods

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "marlin_client.h"
#include "wui.h"
#include "cmsis_os.h"
#include "frozen.h"

#include <string.h>

#define MSG_BUFFSIZE 512
#define MSG_GCODE 1

static void * current_connection;
static void * valid_connection;

extern osMessageQId wui_queue; // input queue (uint8_t)
extern osSemaphoreId wui_sema; // semaphore handle

/*
void json_parse_callback(void *callback_data, const char *name,
                         size_t name_len, const char *path,
                         const struct json_token *token)
{
    switch(token->type) {
        case JSON_TYPE_INVALID:
        case JSON_TYPE_STRING:
            uint16_t len = strlen((const char*) callback_data);
            if(strncmp("command", name, name_len) == 0){
                if(strncmp("init", (const char*) callback_data, len) == 0){

                } else if (strncmp("init", (const char*) callback_data, len) == 0) {

                } else if (strncmp("refresh", (const char*) callback_data, len) == 0) {

                } else if (strncmp("home", (const char*) callback_data, len) == 0) {

                } else if (strncmp("init", (const char*) callback_data, len) == 0) {

                } else if (strncmp("init", (const char*) callback_data, len) == 0) {

                }
            }
            break;
        case JSON_TYPE_NUMBER:
        case JSON_TYPE_TRUE:
        case JSON_TYPE_FALSE:
        case JSON_TYPE_NULL:
        case JSON_TYPE_OBJECT_START:
        case JSON_TYPE_OBJECT_END:
        case JSON_TYPE_ARRAY_START:
        case JSON_TYPE_ARRAY_END:
        break;
    }
}
*/

void send_request_to_server(const char * request);
const char * format_request(uint8_t type, char * request);

void json_parse(const char *request_buf, uint16_t len){

    struct json_token t;
    uint16_t idx;
    char axis[3];
    char command[12];
    uint8_t ret;
    variant8_t val;     //TODO: Devide parse options according to url (printer/tool, printer/printhead)

    for(idx = 0; json_scanf_array_elem(request_buf, len, "", idx, &t) > 0; idx++ ){

        json_scanf(t.ptr, t.len, "{command: %Q}", &command);
        uint8_t command_len = strlen(command);
        if(strncmp(command, "home", command_len) == 0){
            if((ret = json_scanf(t.ptr, t.len, "{axis: [%Q, %Q, %Q]", &axis[0], &axis[1], &axis[2]))){
                char request[100];
                if(ret == 1){
                    sprintf(request, "G28 %c", axis[0]);
                    send_request_to_server(format_request(MSG_GCODE, request));
                } else if (ret == 2) {
                    sprintf(request, "G28 %c %c", axis[0], axis[1]);
                    send_request_to_server(format_request(MSG_GCODE, request));
                } else {
                    sprintf(request, "G28 %c %c %c", axis[0], axis[1], axis[2]);
                    send_request_to_server(format_request(MSG_GCODE, request));
                }

            }
        }


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

const char * format_request(uint8_t type, char * request){
    size_t req_len = strlen(request);
    char tmp_str[req_len];
    strncpy(tmp_str, request, req_len);
    if(type == MSG_GCODE){
        snprintf(request, req_len + 4, "!g %s", tmp_str);
    }
    return request;
}

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd)
{
    //LWIP_UNUSED_ARG();
    if(!memcmp(uri, "/test-post.html", 16)){
        if(current_connection != connection){
            current_connection = connection;
            valid_connection = NULL;
            /* default page */
            snprintf(response_uri, response_uri_len, "/test-post.html");
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
                    request_container.flag = 0;
                    json_parse(request_buf, ret);
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
    snprintf(response_uri, response_uri_len, "/test-post.html");
    if(current_connection == connection){
        if(valid_connection == connection){
            /*receiving data succeeded*/
            snprintf(response_uri, response_uri_len, "/test-post.html");
        }
        current_connection = NULL;
        valid_connection = NULL;
    }
}
