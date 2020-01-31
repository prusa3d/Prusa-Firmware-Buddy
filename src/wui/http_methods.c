// http_methods.c
// implementation of GET / POST methods

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "marlin_client.h"
#include "wui.h"
#include "cmsis_os.h"

#include <string.h>

#define MSG_BUFFSIZE 25
#define MSG_GCODE 1

static void * current_connection;
static void * valid_connection;

extern osMessageQId wui_queue; // input queue (uint8_t)
extern osSemaphoreId wui_sema; // semaphore handle

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
    for(size_t i = 0; i < req_len; i++){
        if(tmp_str[i] == '+'){
            tmp_str[i] = ' ';
        }
    }
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
        u16_t token_move = pbuf_memfind(p, "gcode=", 6, 0);

        if(token_move != 0xFFFF){
            u16_t value_pos = token_move + 6;
            u16_t len_pos = pbuf_memfind(p, "&", 1, value_pos);
            if(len_pos == 0xFFFF){
                len_pos = p->tot_len - value_pos;
            }
            if(len_pos > 0 && len_pos < MSG_BUFFSIZE){
                char request_buf[MSG_BUFFSIZE];
                u16_t ret = pbuf_copy_partial(p, request_buf, len_pos, value_pos);
                if(ret){
                    send_request_to_server(format_request(MSG_GCODE, request_buf));
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
