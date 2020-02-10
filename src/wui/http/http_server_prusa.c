// http_methods.c
// implementation of GET / POST methods

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "marlin_client.h"
#include "wui.h"
#include "cmsis_os.h"
#include "wui_api.h"

#include <string.h>

#define MSG_BUFFSIZE 512

static void * current_connection;
static void * valid_connection;

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
