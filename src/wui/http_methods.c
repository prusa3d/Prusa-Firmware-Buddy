// http_methods.c
// implementation of GET / POST methods

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "marlin_client.h"
#include "wui.h"

#include <string.h>

#define POS_BUFFSIZE 5
#define Z_MAX_POS 170
#define Z_MIN_POS 5

static void * current_connection;
static void * valid_connection;

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
        u16_t token_move = pbuf_memfind(p, "move=", 5, 0);
        if(token_move != 0xFFFF){
            u16_t value_pos = token_move + 5;
            u16_t len_pos = 0;
            len_pos = p->tot_len - value_pos;
            if(len_pos > 0 && len_pos < POS_BUFFSIZE){
                char buff_pos[POS_BUFFSIZE];
                char * pos = (char*)pbuf_get_contiguous(p, buff_pos, sizeof(buff_pos), len_pos, value_pos);
                if(pos){
                    pos[POS_BUFFSIZE] = 0;
                    u16_t value = atoi(pos);
                    if(value < Z_MAX_POS && value > Z_MIN_POS){
                        move_Z_axis = value;
                        valid_connection = connection;
                    }
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
