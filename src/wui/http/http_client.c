/*
 * http_client.c
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "http_client.h"
#include "wui_helper_funcs.h"
#include <stdbool.h>
#include "wui_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "eeprom.h"
#include "lwip/altcp.h"
#include "lwip.h"

#define CLIENT_CONNECT_DELAY      1000 // 1 Sec.
#define CLIENT_PORT_NO            9000
#define IP4_ADDR_STR_SIZE         16
#define HEADER_MAX_SIZE           128
#define HTTPC_CONTENT_LEN_INVALID 0xFFFFFFFF
#define HTTPC_POLL_INTERVAL       1
#define HTTPC_POLL_TIMEOUT        3 /* 1.5 seconds */

struct tcp_pcb *client_pcb;
static uint32_t client_interval = 0;
static bool init_tick = false;

/**
 * @ingroup httpc
 * HTTP client result codes
 */
typedef enum ehttpc_result {
    /** File successfully received */
    HTTPC_RESULT_OK = 0,
    /** Unknown error */
    HTTPC_RESULT_ERR_UNKNOWN = 1,
    /** Connection to server failed */
    HTTPC_RESULT_ERR_CONNECT = 2,
    /** Failed to resolve server hostname */
    HTTPC_RESULT_ERR_HOSTNAME = 3,
    /** Connection unexpectedly closed by remote server */
    HTTPC_RESULT_ERR_CLOSED = 4,
    /** Connection timed out (server didn't respond in time) */
    HTTPC_RESULT_ERR_TIMEOUT = 5,
    /** Server responded with an error code */
    HTTPC_RESULT_ERR_SVR_RESP = 6,
    /** Local memory error */
    HTTPC_RESULT_ERR_MEM = 7,
    /** Local abort */
    HTTPC_RESULT_LOCAL_ABORT = 8,
    /** Content length mismatch */
    HTTPC_RESULT_ERR_CONTENT_LEN = 9
} httpc_result_t;

typedef struct _httpc_state httpc_state_t;

/**
 * @ingroup httpc
 * Prototype of a http client callback function
 *
 * @param arg argument specified when initiating the request
 * @param httpc_result result of the http transfer (see enum httpc_result_t)
 * @param rx_content_len number of bytes received (without headers)
 * @param srv_res this contains the http status code received (if any)
 * @param err an error returned by internal lwip functions, can help to specify
 *            the source of the error but must not necessarily be != ERR_OK
 */
typedef void (*httpc_result_fn)(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);

/**
 * @ingroup httpc
 * Prototype of http client callback: called when the headers are received
 *
 * @param connection http client connection
 * @param arg argument specified when initiating the request
 * @param hdr header pbuf(s) (may contain data also)
 * @param hdr_len length of the heders in 'hdr'
 * @param content_len content length as received in the headers (-1 if not received)
 * @return if != ERR_OK is returned, the connection is aborted
 */
typedef err_t (*httpc_headers_done_fn)(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

typedef struct _httpc_connection {
    ip_addr_t proxy_addr;
    u16_t proxy_port;
    u8_t use_proxy;
    /* @todo: add username:pass? */

#if LWIP_ALTCP
    altcp_allocator_t *altcp_allocator;
#endif

    /* this callback is called when the transfer is finished (or aborted) */
    httpc_result_fn result_fn;
    /* this callback is called after receiving the http headers
     It can abort the connection by returning != ERR_OK */
    httpc_headers_done_fn headers_done_fn;
} httpc_connection_t;

typedef enum ehttpc_parse_state {
    HTTPC_PARSE_WAIT_FIRST_LINE = 0,
    HTTPC_PARSE_WAIT_HEADERS,
    HTTPC_PARSE_RX_DATA
} httpc_parse_state_t;

typedef struct _httpc_state {
    struct altcp_pcb *pcb;
    ip_addr_t remote_addr;
    u16_t remote_port;
    int timeout_ticks;
    struct pbuf *request;
    struct pbuf *rx_hdrs;
    u16_t rx_http_version;
    u16_t rx_status;
    altcp_recv_fn recv_fn;
    const httpc_connection_t *conn_settings;
    void *callback_arg;
    u32_t rx_content_len;
    u32_t hdr_content_len;
    httpc_parse_state_t parse_state;
#if HTTPC_DEBUG_REQUEST
    char *server_name;
    char *uri;
#endif
} httpc_state_t;

/** Free http client state and deallocate all resources within */
static err_t
httpc_free_state(httpc_state_t *req) {
    struct altcp_pcb *tpcb;

    if (req->request != NULL) {
        pbuf_free(req->request);
        req->request = NULL;
    }
    if (req->rx_hdrs != NULL) {
        pbuf_free(req->rx_hdrs);
        req->rx_hdrs = NULL;
    }

    tpcb = req->pcb;
    mem_free(req);
    req = NULL;

    if (tpcb != NULL) {
        err_t r;
        altcp_arg(tpcb, NULL);
        altcp_recv(tpcb, NULL);
        altcp_err(tpcb, NULL);
        altcp_poll(tpcb, NULL, 0);
        altcp_sent(tpcb, NULL);
        r = altcp_close(tpcb);
        if (r != ERR_OK) {
            altcp_abort(tpcb);
            return ERR_ABRT;
        }
    }
    return ERR_OK;
}

/** Close the connection: call finished callback and free the state */
static err_t
httpc_close(httpc_state_t *req, httpc_result_t result, u32_t server_response, err_t err) {
    if (req != NULL) {
        if (req->conn_settings != NULL) {
            if (req->conn_settings->result_fn != NULL) {
                req->conn_settings->result_fn(req->callback_arg, result, req->rx_content_len, server_response, err);
            }
        }
        return httpc_free_state(req);
    }
    return ERR_OK;
}

/** Parse http header response line 1 */
static err_t
http_parse_response_status(struct pbuf *p, u16_t *http_version, u16_t *http_status, u16_t *http_status_str_offset) {
    u16_t end1 = pbuf_memfind(p, "\r\n", 2, 0);
    if (end1 != 0xFFFF) {
        /* get parts of first line */
        u16_t space1, space2;
        space1 = pbuf_memfind(p, " ", 1, 0);
        if (space1 != 0xFFFF) {
            if ((pbuf_memcmp(p, 0, "HTTP/", 5) == 0) && (pbuf_get_at(p, 6) == '.')) {
                char status_num[10];
                size_t status_num_len;
                /* parse http version */
                u16_t version = pbuf_get_at(p, 5) - '0';
                version <<= 8;
                version |= pbuf_get_at(p, 7) - '0';
                *http_version = version;

                /* parse http status number */
                space2 = pbuf_memfind(p, " ", 1, space1 + 1);
                if (space2 != 0xFFFF) {
                    *http_status_str_offset = space2 + 1;
                    status_num_len = space2 - space1 - 1;
                } else {
                    status_num_len = end1 - space1 - 1;
                }
                memset(status_num, 0, sizeof(status_num));
                if (pbuf_copy_partial(p, status_num, (u16_t)status_num_len, space1 + 1) == status_num_len) {
                    int status = atoi(status_num);
                    if ((status > 0) && (status <= 0xFFFF)) {
                        *http_status = (u16_t)status;
                        return ERR_OK;
                    }
                }
            }
        }
    }
    return ERR_VAL;
}

/** Wait for all headers to be received, return its length and content-length (if available) */
static err_t
http_wait_headers(struct pbuf *p, u32_t *content_length, u16_t *total_header_len) {
    u16_t end1 = pbuf_memfind(p, "\r\n\r\n", 4, 0);
    if (end1 < (0xFFFF - 2)) {
        /* all headers received */
        /* check if we have a content length (@todo: case insensitive?) */
        u16_t content_len_hdr;
        *content_length = HTTPC_CONTENT_LEN_INVALID;
        *total_header_len = end1 + 4;

        content_len_hdr = pbuf_memfind(p, "Content-Length: ", 16, 0);
        if (content_len_hdr != 0xFFFF) {
            u16_t content_len_line_end = pbuf_memfind(p, "\r\n", 2, content_len_hdr);
            if (content_len_line_end != 0xFFFF) {
                char content_len_num[16];
                u16_t content_len_num_len = (u16_t)(content_len_line_end - content_len_hdr - 16);
                memset(content_len_num, 0, sizeof(content_len_num));
                if (pbuf_copy_partial(p, content_len_num, content_len_num_len, content_len_hdr + 16) == content_len_num_len) {
                    int len = atoi(content_len_num);
                    if ((len >= 0) && ((u32_t)len < HTTPC_CONTENT_LEN_INVALID)) {
                        *content_length = (u32_t)len;
                    }
                }
            }
        }
        return ERR_OK;
    }
    return ERR_VAL;
}

/** http client tcp recv callback */
static err_t
httpc_tcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t r) {
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(r);

    if (p == NULL) {
        httpc_result_t result;
        if (req->parse_state != HTTPC_PARSE_RX_DATA) {
            /* did not get RX data yet */
            result = HTTPC_RESULT_ERR_CLOSED;
        } else if ((req->hdr_content_len != HTTPC_CONTENT_LEN_INVALID) && (req->hdr_content_len != req->rx_content_len)) {
            /* header has been received with content length but not all data received */
            result = HTTPC_RESULT_ERR_CONTENT_LEN;
        } else {
            /* receiving data and either all data received or no content length header */
            result = HTTPC_RESULT_OK;
        }
        return httpc_close(req, result, req->rx_status, ERR_OK);
    }
    if (req->parse_state != HTTPC_PARSE_RX_DATA) {
        if (req->rx_hdrs == NULL) {
            req->rx_hdrs = p;
        } else {
            pbuf_cat(req->rx_hdrs, p);
        }
        if (req->parse_state == HTTPC_PARSE_WAIT_FIRST_LINE) {
            u16_t status_str_off;
            err_t err = http_parse_response_status(req->rx_hdrs, &req->rx_http_version, &req->rx_status, &status_str_off);
            if (err == ERR_OK) {
                /* don't care status string */
                req->parse_state = HTTPC_PARSE_WAIT_HEADERS;
            }
        }
        if (req->parse_state == HTTPC_PARSE_WAIT_HEADERS) {
            u16_t total_header_len;
            err_t err = http_wait_headers(req->rx_hdrs, &req->hdr_content_len, &total_header_len);
            if (err == ERR_OK) {
                struct pbuf *q;
                /* full header received, send window update for header bytes and call into client callback */
                altcp_recved(pcb, total_header_len);
                if (req->conn_settings) {
                    if (req->conn_settings->headers_done_fn) {
                        err = req->conn_settings->headers_done_fn(req, req->callback_arg, req->rx_hdrs, total_header_len, req->hdr_content_len);
                        if (err != ERR_OK) {
                            return httpc_close(req, HTTPC_RESULT_LOCAL_ABORT, req->rx_status, err);
                        }
                    }
                }
                /* hide header bytes in pbuf */
                q = pbuf_free_header(req->rx_hdrs, total_header_len);
                p = q;
                req->rx_hdrs = NULL;
                /* go on with data */
                req->parse_state = HTTPC_PARSE_RX_DATA;
            }
        }
    }
    if ((p != NULL) && (req->parse_state == HTTPC_PARSE_RX_DATA)) {
        req->rx_content_len += p->tot_len;
        if (req->recv_fn != NULL) {
            /* directly return here: the connection migth already be aborted from the callback! */
            return req->recv_fn(req->callback_arg, pcb, p, r);
        } else {
            altcp_recved(pcb, p->tot_len);
            pbuf_free(p);
        }
    }
    return ERR_OK;
}

/** http client tcp err callback */
static void
httpc_tcp_err(void *arg, err_t err) {
    httpc_state_t *req = (httpc_state_t *)arg;
    if (req != NULL) {
        /* pcb has already been deallocated */
        req->pcb = NULL;
        httpc_close(req, HTTPC_RESULT_ERR_CLOSED, 0, err);
    }
}

/** http client tcp poll callback */
static err_t
httpc_tcp_poll(void *arg, struct altcp_pcb *pcb) {
    /* implement timeout */
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(pcb);
    if (req != NULL) {
        if (req->timeout_ticks) {
            req->timeout_ticks--;
        }
        if (!req->timeout_ticks) {
            return httpc_close(req, HTTPC_RESULT_ERR_TIMEOUT, 0, ERR_OK);
        }
    }
    return ERR_OK;
}

/** http client tcp sent callback */
static err_t
httpc_tcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len) {
    /* nothing to do here for now */
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    return ERR_OK;
}

/** http client tcp connected callback */
static err_t
httpc_tcp_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    err_t r;
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(err);

    /* send request; last char is zero termination */
    r = altcp_write(req->pcb, req->request->payload, req->request->len - 1, TCP_WRITE_FLAG_COPY);
    if (r != ERR_OK) {
        /* could not write the single small request -> fail, don't retry */
        return httpc_close(req, HTTPC_RESULT_ERR_MEM, 0, r);
    }
    /* everything written, we can free the request */
    pbuf_free(req->request);
    req->request = NULL;

    altcp_output(req->pcb);
    return ERR_OK;
}

/** Function for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t data_received_fun(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {

    LWIP_UNUSED_ARG(tpcb);
    LWIP_UNUSED_ARG(err);
    uint32_t len_copied = 0;
    if (NULL == p) {
        return ERR_ARG;
    }

    char request_part[(const u16_t)p->tot_len + 1];

    while (len_copied < p->tot_len) {

        char *payload = p->payload;
        // check if empty
        if (payload[0] == 0) {
            return ERR_ARG;
        }

        len_copied += pbuf_copy_partial(p, request_part, p->tot_len, 0);

        if (len_copied != p->len) {
            return ERR_ARG;
        }
        p = p->next;
        if (NULL == p) {
            request_part[(const u16_t)p->tot_len] = 0; // end of line added
            break;
        }
    }
    http_json_parser((char *)&request_part, len_copied);
    return ERR_OK;
}

wui_err buddy_http_client_init() {

    char host_ip4_str[IP4_ADDR_STR_SIZE];
    char header[HEADER_MAX_SIZE];
    char *uri = "/p/telemetry";
    char printer_token[CONNECT_TOKEN_SIZE + 1]; // extra space of end of line
    ip4_addr_t host_ip4;

    size_t alloc_len;
    mem_size_t mem_alloc_len;
    int req_len, req_len2;
    httpc_state_t *req;

    host_ip4.addr = eeprom_get_var(EEVAR_CONNECT_IP4).ui32;
    strlcpy(host_ip4_str, ip4addr_ntoa(&host_ip4), IP4_ADDR_STR_SIZE);
    variant8_t token = eeprom_get_var(EEVAR_LAN_HOSTNAME);
    strlcpy(printer_token, token.pch, LAN_HOSTNAME_MAX_LEN + 1);
    variant8_done(&token);
    snprintf(header, HEADER_MAX_SIZE, "POST %s HTTP/1.0\r\nHost: %s\nPrinter-Token: %s\r\n", uri, host_ip4_str, printer_token);
    const char *header_plus_data = get_update_str(header);

    req_len = strlen(header_plus_data);

    if ((req_len < 0) || (req_len > 0xFFFF)) {
        return ERR_VAL;
    }
    /* alloc state and request in one block */
    alloc_len = sizeof(httpc_state_t);
    mem_alloc_len = (mem_size_t)alloc_len;
    if ((mem_alloc_len < alloc_len) || (req_len + 1 > 0xFFFF)) {
        return ERR_VAL;
    }

    req = (httpc_state_t *)mem_malloc((mem_size_t)alloc_len);
    if (req == NULL) {
        return ERR_MEM;
    }
    memset(req, 0, sizeof(httpc_state_t));
    req->timeout_ticks = HTTPC_POLL_TIMEOUT;
    req->request = pbuf_alloc(PBUF_RAW, (u16_t)(req_len + 1), PBUF_RAM);
    if (req->request == NULL) {
        httpc_free_state(req);
        return ERR_MEM;
    }
    if (req->request->next != NULL) {
        /* need a pbuf in one piece */
        httpc_free_state(req);
        return ERR_MEM;
    }
    req->hdr_content_len = HTTPC_CONTENT_LEN_INVALID;
    req->pcb = altcp_new();
    if (req->pcb == NULL) {
        httpc_free_state(req);
        return ERR_MEM;
    }
    req->remote_port = CLIENT_PORT_NO;
    altcp_arg(req->pcb, req);
    altcp_recv(req->pcb, httpc_tcp_recv);
    altcp_err(req->pcb, httpc_tcp_err);
    altcp_poll(req->pcb, httpc_tcp_poll, HTTPC_POLL_INTERVAL);
    altcp_sent(req->pcb, httpc_tcp_sent);

    /* set up request buffer */
    req_len2 = strlcpy((char *)req->request->payload, header_plus_data, req_len + 1);
    if (req_len2 != req_len) {
        httpc_free_state(req);
        return ERR_VAL;
    }

    req->recv_fn = data_received_fun;
    tcp_connect(req->pcb, &host_ip4, 9000, httpc_tcp_connected);
    return ERR_OK;
}

void buddy_http_client_loop() {

    if (!init_tick) {
        client_interval = xTaskGetTickCount();
        init_tick = true;
    }

    if (netif_ip4_addr(&eth0)->addr != 0 && ((xTaskGetTickCount() - client_interval) > CLIENT_CONNECT_DELAY)) {
        buddy_http_client_init();
        client_interval = xTaskGetTickCount();
    }
}
