/**
 * @file
 * Application layered TCP connection API (to be used from TCPIP thread)\n
 * This interface mimics the tcp callback API to the application while preventing
 * direct linking (much like virtual functions).
 * This way, an application can make use of other application layer protocols
 * on top of TCP without knowing the details (e.g. TLS, proxy connection).
 *
 * This file contains the base implementation calling into tcp.
 */

/*
 * Copyright (c) 2021 Marek Mosna
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the ESPIP TCP/IP stack.
 *
 * Author: Marek Mosna <marek.mosna@prusa3d.cz>
 * Author: Vladimir Matena <vladimir.matena@prusa3d.cz>
 *
 */

#include "esp/esp_config.h"

#if ESP_ALTCP /* don't build if not configured for use in espopts.h */

    #include <assert.h>
    #include "esp/esp.h"
    #include <string.h>
    #include "lwip/altcp.h"
    #include "lwip/priv/altcp_priv.h"

    #include "sockets/lwesp_sockets_priv.h"

    #include "lwip/tcp.h"

    #include "esp_tcp.h"
    #include "esp/esp_mem.h"
    #include "esp/esp_private.h"

    #ifdef ALTCP_ESP_DEBUG
        #include "dbg.h"
        #define ALTCP_ESP_DEBUG_FN(fmt, ...) _dbg(fmt, ##__VA_ARGS__)
        #define ALTCP_ESP_DEBUG_ASSERT(cond, ...) \
            if (!(cond)) {                        \
                ALTCP_ESP_DEBUG_FN(__VA_ARGS__);  \
                assert(0);                        \
            }
    #else
        #define ALTCP_ESP_DEBUG_FN(fmt, ...)
        #define ALTCP_ESP_DEBUG_ASSERT(cond, ...)
    #endif

/* Variable prototype, the actual declaration is at the end of this file
   since it contains pointers to static functions declared here */
extern const struct altcp_functions altcp_esp_functions;

static void altcp_esp_setup(struct altcp_pcb *conn, esp_pcb *epcb);

/* Translate ESP errors into LwIP errors */
static err_t espr_t2err_t(const espr_t err) {
    switch (err) {
    case espOK:
        return ERR_OK;    /*!< Function succeeded */
    case espOKIGNOREMORE: /*!< Function succedded, should continue as espOK but ignore sending more data. This result is possible on connection data receive callback */
        ALTCP_ESP_DEBUG_FN("espOKIGNOREMORE - pretending all ok");
        return ERR_OK;
    case espERR:
        ALTCP_ESP_DEBUG_FN("Generic ESP err");
        return ERR_IF;
    case espERRCONNTIMEOUT: /*!< Timeout received when connection to access point */
        ALTCP_ESP_DEBUG_FN("espERRCONNTIMEOUT");
        return ERR_IF;
    case espERRPASS: /*!< Invalid password for access point */
        ALTCP_ESP_DEBUG_FN("espERRPASS");
        return ERR_IF;
    case espERRNOAP: /*!< No access point found with specific SSID and MAC address */
        ALTCP_ESP_DEBUG_FN("espERRNOAP");
        return ERR_IF;
    case espERRCONNFAIL: /*!< Connection failed to access point */
        ALTCP_ESP_DEBUG_FN("espERRCONNFAIL");
        return ERR_IF;
    case espERRWIFINOTCONNECTED: /*!< Wifi not connected to access point */
        ALTCP_ESP_DEBUG_FN("espERRWIFINOTCONNECTED");
        return ERR_IF;
    case espERRNODEVICE: /*!< Device is not present */
        ALTCP_ESP_DEBUG_FN("espERRNODEVICE");
        return ERR_IF;
    case espCONT:
        ALTCP_ESP_DEBUG_FN("espCONT"); /*!< There is still some command to be processed in current command */
        return ERR_IF;
    case espPARERR:
        return ERR_VAL;    /*!< Wrong parameters on function call */
    case espERRNOFREECONN: /*!< There is no free connection available to start */
    case espERRMEM:
        return ERR_MEM; /*!< Memory error occurred */
    case espTIMEOUT:
        return ERR_TIMEOUT; /*!< Timeout occurred on command */
    case espCLOSED:
        return ERR_CLSD; /*!< Connection just closed */
    case espINPROG:
        return ERR_INPROGRESS; /*!< Operation is in progress */
    case espERRNOIP:
        return ERR_ISCONN; /*!< Station does not have IP address */
    case espERRBLOCKING:
        return ERR_WOULDBLOCK;
    default:
        ALTCP_ESP_DEBUG_FN("Unknown ESP err");
        return -1;
    }
}

static void ASSERT_EPCB_VALID(esp_pcb *epcb) {
    #ifdef ALTCP_ESP_DEBUG
    ALTCP_ESP_DEBUG_ASSERT(epcb->magic_start == ALTCP_ESP_EPCB_MAGIC, "Corrupted EPCB magic");
    ALTCP_ESP_DEBUG_ASSERT(epcb->magic_end == ALTCP_ESP_EPCB_MAGIC, "Corrupted EPCB magic");
    #endif

    assert(epcb);
}

static void ASSERT_EPCB(esp_pcb *epcb) {
    ASSERT_EPCB_VALID(epcb);
    ALTCP_ESP_DEBUG_ASSERT(epcb->alconn->state == epcb, "Failed: CORRUPTED EPCB ALCONN LINK: %x != %x", epcb->alconn->state, epcb);
    #ifdef ALTCP_ESP_DEBUG
    esp_pcb *econn_link = esp_conn_get_arg(epcb->econn);
    #endif
    ALTCP_ESP_DEBUG_ASSERT(econn_link == epcb, "Failed: CORRUPTED EPCB ECONN LINK: %x != %x", econn_link, epcb);
}

/* Check ALTCP and ESP PCB connection consistency */
static void ALTCP_TCP_ASSERT_CONN_PCB(struct altcp_pcb *alconn, esp_pcb *epcb) {
    ALTCP_ESP_DEBUG_ASSERT(alconn, "Failed: ESP connection pointer is NULL when it should be set !!!");
    ALTCP_ESP_DEBUG_ASSERT(epcb, "Failed: ESP PCB pointer is NULL when it should be set !!!!");
    ALTCP_ESP_DEBUG_ASSERT(alconn->state == epcb, "Failed: ESP connection - ESP PCB mismatch conn->state: %x != epcb: %x !!!", alconn->state, epcb);
    ALTCP_ESP_DEBUG_ASSERT(epcb->alconn == alconn, "Failed: ESP PCB - ALTCP connection mismatch epcb->alconn: %x != conn: %x !!!", epcb->alconn, alconn);
}

static esp_pcb *get_epcb(esp_conn_p conn) {
    esp_pcb *epcb = esp_conn_get_arg(conn);
    #ifdef ALTCP_ESP_DEBUG
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("Failed: EPCB for ECONN: %x is NULL", conn);
    }
    ASSERT_EPCB(epcb);
    #endif
    return epcb;
}

/* callback functions for TCP */
static err_t altcp_esp_accept(void *arg, esp_pcb *new_epcb, err_t err) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_accept");
    struct altcp_pcb *listen_conn = (struct altcp_pcb *)arg;
    if (!listen_conn || !listen_conn->accept) {
        ALTCP_ESP_DEBUG_FN("!!! listen connection not set properly !!!");
        return ERR_ARG;
    }

    /* create a new altcp_conn to pass to the next 'accept' callback */
    struct altcp_pcb *new_conn = altcp_alloc();
    if (new_conn == NULL) {
        ALTCP_ESP_DEBUG_FN("No mem to alloc altcp !!!");
        return ERR_MEM;
    }
    altcp_esp_setup(new_conn, new_epcb);
    ALTCP_TCP_ASSERT_CONN_PCB(new_conn, new_epcb);
    ASSERT_EPCB(new_epcb);
    err_t ret = listen_conn->accept(listen_conn->arg, new_conn, err);
    return ret;
}

static err_t altcp_esp_connected(void *arg, esp_pcb *epcb, err_t err) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_connected");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (epcb->alconn && epcb->alconn->connected) {
        epcb->alconn->connected(conn->arg, conn, 0);
    }
    return ERR_OK;
}

static err_t altcp_esp_recv(void *arg, esp_pcb *epcb, struct pbuf *p, err_t err) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_recv");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;

    if (!p) {
        ALTCP_ESP_DEBUG_FN("Recv - no pbuf !!!");
        goto error_no_pbuf;
    }

    if (!conn) {
        ALTCP_ESP_DEBUG_FN("Recv - no conn !!!");
        goto cleanup_pbuf;
    }

    ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
    ASSERT_EPCB(epcb);
    if (!conn->recv) {
        ALTCP_ESP_DEBUG_FN("Recv - no recv !!!");
        goto cleanup_pbuf;
    }

    // This takes sometimes quite some time ~ 500ms
    // Thanks to the manual TCP receive this is not fatal
    #ifdef ALTCP_ESP_DEBUG
    const long start = xTaskGetTickCount();
    #endif
    err_t ret = conn->recv(conn->arg, conn, p, err);
    #ifdef ALTCP_ESP_DEBUG
    const long end = xTaskGetTickCount();
    ALTCP_ESP_DEBUG_FN("recv callback in %ld ms", (end - start) / portTICK_RATE_MS);
    #endif
    return ret;

cleanup_pbuf:
    pbuf_free(p);
error_no_pbuf:
    return ERR_VAL;
}

static err_t altcp_esp_sent(void *arg, esp_pcb *epcb, u16_t len) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_sent");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("esp sent - no conn !!!");
    }
    ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
    ASSERT_EPCB(epcb);
    if (conn->sent) {
        return conn->sent(conn->arg, conn, len);
    }
    return ERR_OK;
}

static err_t altcp_esp_poll(void *arg, esp_pcb *epcb) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_poll");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("esp poll - no conn !!!");
        return ERR_VAL;
    }
    ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
    ASSERT_EPCB(epcb);
    if (conn->poll) {
        return conn->poll(conn->arg, conn);
    }
    return ERR_OK;
}

static void altcp_esp_err(void *arg, err_t err) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_err");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("esp err - > no conn !!!");
        return;
    }
    if (conn->err) {
        conn->err(conn->arg, err);
    }
    altcp_free(conn);
}

/* setup functions */

static void altcp_esp_setup(struct altcp_pcb *conn, esp_pcb *epcb) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_setup");
    conn->state = epcb;
    conn->fns = &altcp_esp_functions;
    epcb->alconn = conn;
}

static esp_pcb *esp_new_ip_type(u8_t ip_type) {
    esp_pcb *epcb = (esp_pcb *)esp_mem_malloc(sizeof(esp_pcb));
    if (!epcb) {
        return NULL;
    }

    memset(epcb, 0, sizeof(esp_pcb));
    epcb->buff_avail = ESP_CFG_CONN_MAX_DATA_LEN;
    #ifdef ALTCP_ESP_DEBUG
    epcb->magic_start = ALTCP_ESP_EPCB_MAGIC;
    epcb->magic_end = ALTCP_ESP_EPCB_MAGIC;
    #endif
    return epcb;
}

static esp_pcb *listen_api = NULL;

typedef struct {
    struct pbuf_custom custom_lwip_pbuf;
    esp_pbuf_p esp_pbuf;
} lwip_esp_pbuf_custom;

static void custom_pbuf_free(struct pbuf *p) {
    // pbuf is first member of custom lwip esp wrapper
    lwip_esp_pbuf_custom *custom = (lwip_esp_pbuf_custom *)p;

    esp_pbuf_free(custom->esp_pbuf);
    esp_mem_free(custom);
}

static espr_t esp_evt_conn_active(esp_conn_p conn) {
    uint8_t close = 0;
    esp_pcb *epcb = NULL;

    if (esp_conn_is_client(conn)) {
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_ACTIVE - CLIENT");
        epcb = get_epcb(conn);
        if (epcb != NULL) {
            epcb->econn = conn;
            altcp_esp_connected(epcb->alconn, epcb, 0);
        } else {
            ALTCP_ESP_DEBUG_FN("ACTIVE CLIENT WITHOUT EPCB POINTER !!!!");
            close = 1;
        }
    } else if (esp_conn_is_server(conn) && listen_api != NULL) {
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_ACTIVE - SERVER");
        epcb = esp_new_ip_type(0);
        ESP_DEBUGW(ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING,
            epcb == NULL, "[ESPTCP] Cannot create new structure for incoming server connection!\r\n");

        if (epcb != NULL) {
            epcb->econn = conn;
            esp_conn_set_arg(conn, epcb);
            if (altcp_esp_accept(listen_api->alconn, epcb, 0) != ERR_OK) {
                close = 1;
            }
        } else {
            ALTCP_ESP_DEBUG_FN("esp_pcb not created");
            close = 1;
        }
    } else {
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_ACTIVE - OTHER");
        ESP_DEBUGW(ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, listen_api == NULL,
            "[ESPTCP] Closing connection as there is no listening API in ESP PCB!\r\n");
        close = 1; /* Close the connection at this point */
    }

    /* Decide if some events want to close the connection */
    if (close) {
        ALTCP_ESP_DEBUG_FN("Closing conn %d", esp_conn_getnum(conn));
        esp_conn_close(conn, 0);
        return espERR;
    }
    return espOK;
}

static espr_t esp_evt_conn_recv(esp_conn_p conn, esp_evt_t *evt) {
    esp_pcb *epcb = get_epcb(conn);                    /* Get API from connection */
    esp_pbuf_p pbuf = esp_evt_conn_recv_get_buff(evt); /* Get received buff */

    if (!pbuf) {
        ALTCP_ESP_DEBUG_FN("Receive pbuf not set !!!");
        goto error;
    }

    if (!epcb) {
        goto cleanup_pbuf;
    }

    if (epcb->closed) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closed !!!");
        goto cleanup_pbuf;
    }

    epcb->rcv_packets++; // Increase number of received packets
    const size_t recv_len = esp_pbuf_length(pbuf, 0);
    epcb->rcv_bytes += recv_len;
    ALTCP_ESP_DEBUG_FN("Received %ld, total %ld packets, %ld bytes", recv_len,
        epcb->rcv_packets, epcb->rcv_bytes);

    if (recv_len != esp_pbuf_length(pbuf, 1)) {
        ALTCP_ESP_DEBUG_FN("!!! rcv pbuf has multiple parts, this is not supported !!!!");
        goto cleanup_pbuf;
    }

    /** This does a bit tricky zero copy LwESP PBUF to LwIP PBUF conversion
     * Custom pbuf wrapper holds custom LwIP PBUF together with original LwESP
     * PBUF in order to dealocate the later one at the same time as the first one */
    lwip_esp_pbuf_custom *custom_pbuf_wrapper = esp_mem_malloc(sizeof(lwip_esp_pbuf_custom));
    if (!custom_pbuf_wrapper) {
        ALTCP_ESP_DEBUG_FN("Failed to alloc custom pbuf wrapper");
        goto cleanup_pbuf;
    }
    memset(custom_pbuf_wrapper, 0, sizeof(lwip_esp_pbuf_custom));
    custom_pbuf_wrapper->custom_lwip_pbuf.custom_free_function = custom_pbuf_free;
    struct pbuf *lwip_pbuf = pbuf_alloced_custom(PBUF_RAW, recv_len, PBUF_REF,
        &custom_pbuf_wrapper->custom_lwip_pbuf, (char *)esp_pbuf_data(pbuf), recv_len);
    if (!lwip_pbuf) {
        ALTCP_ESP_DEBUG_FN("Failed to obtain custom LwIP pbuf, len: %ld", recv_len);
        goto cleanup_wrapper;
    }
    custom_pbuf_wrapper->esp_pbuf = pbuf;

    return altcp_esp_recv(epcb->alconn, epcb, lwip_pbuf, 0);

cleanup_wrapper:
    esp_mem_free(custom_pbuf_wrapper);
cleanup_pbuf:
    esp_pbuf_free(pbuf);
error:
    return espERR;
}

static espr_t esp_evt_conn_closed(esp_conn_p conn) {
    esp_pcb *epcb = get_epcb(conn); /* Get API from connection */
    if (!epcb) {
        return espERR;
    }

    esp_conn_set_arg(conn, NULL);
    epcb->closed = 1;
    ALTCP_ESP_DEBUG_FN("Connection closed and EPCB not NULL -> free, err");
    struct altcp_pcb *alpcb = epcb->alconn;
    if (!alpcb) {
        ALTCP_ESP_DEBUG_FN("Connection closed and ALPCB already NULL");
        return espERR;
    }
    ALTCP_TCP_ASSERT_CONN_PCB(alpcb, epcb);

    altcp_esp_err(alpcb, ERR_CLSD);
    return espOK;
}

static espr_t esp_evt_conn_send(esp_conn_p conn, esp_evt_t *evt) {
    ALTCP_ESP_DEBUG_FN("esp_evt_conn_send");
    if (!esp_conn_is_active(conn) || esp_conn_is_closed(conn)) {
        ALTCP_ESP_DEBUG_FN("Connection not active or closed !!!");
        return espCLOSED;
    }

    esp_pcb *epcb = get_epcb(conn);
    if (!epcb) {
        return espERR;
    }

    if (epcb->closed) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closed !!!");
        return espCLOSED;
    }

    if (epcb->closing) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closing !!!");
        return espCLOSED;
    }

    struct altcp_pcb *pcb = epcb->alconn;
    if (!pcb) {
        ALTCP_ESP_DEBUG_FN("PCB is NULL !!!");
        return espERR;
    }

    const size_t sent = esp_evt_conn_send_get_length(evt);
    ALTCP_ESP_DEBUG_FN("Sent size: %d", sent);
    altcp_esp_sent(pcb, epcb, sent);
    return espOK;
}

static espr_t esp_evt_conn_poll(esp_conn_p conn) {
    esp_pcb *epcb = get_epcb(conn);
    if (!epcb) {
        esp_conn_close(conn, 0);
        return espERR;
    }

    if (epcb->closed) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closed !!!");
        return espCLOSED;
    }

    if (!epcb->alconn) {
        ALTCP_ESP_DEBUG_FN("ALCONN is NULL !!!");
        esp_conn_close(conn, 0);
        return espERR;
    }

    return altcp_esp_poll(epcb->alconn, epcb);
}

static espr_t altcp_esp_evt(esp_evt_t *evt) {
    esp_conn_p conn = esp_conn_get_from_evt(evt);
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("Conn is NULL !!!");
        return espERR;
    }

    ALTCP_ESP_DEBUG_FN("Event from conn %d, conn: 0x%x, arg: 0x%x",
        esp_conn_getnum(conn), conn, esp_conn_get_arg(conn));
    switch (esp_evt_get_type(evt)) {
    case ESP_EVT_CONN_ACTIVE:
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_ACTIVE");
        return esp_evt_conn_active(conn);

    /*
     * We have a new data received which
     * should have esp pcb structure as argument
     */
    case ESP_EVT_CONN_RECV:
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_RECV");
        return esp_evt_conn_recv(conn, evt);

    /* Connection was just closed */
    case ESP_EVT_CONN_CLOSE:
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_CLOSED");
        if (esp_evt_conn_close_get_result(evt) == espOK) {
            return esp_evt_conn_closed(conn);
        } else {
            return espERR;
        }

    case ESP_EVT_CONN_SEND:
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_SEND");
        return esp_evt_conn_send(conn, evt);

    case ESP_EVT_CONN_POLL:
        ALTCP_ESP_DEBUG_FN("ESP_EVT_CONN_POLL");
        return esp_evt_conn_poll(conn);
    default:
        ALTCP_ESP_DEBUG_FN("Unknown event type: %d", esp_evt_get_type(evt));
        return espERR;
    }
}

struct altcp_pcb *altcp_esp_new_ip_type(u8_t ip_type) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_new_ip_type");
    /* Allocate the tcp pcb first to invoke the priority handling code
     if we're out of pcbs */
    esp_pcb *epcb = esp_new_ip_type(ip_type);
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("Failed to alloc EPCB !!!");
        return NULL;
    }

    struct altcp_pcb *alpcb = altcp_alloc();
    if (!alpcb) {
        ALTCP_ESP_DEBUG_FN("No mem to alloc ALPCB !!!");
        esp_mem_free(epcb);
        return NULL;
    }

    altcp_esp_setup(alpcb, epcb);
    return alpcb;
}

/** altcp_esp allocator function fitting to @ref altcp_allocator_t / @ref altcp_new.
*
* arg pointer is not used for TCP.
*/
struct altcp_pcb *altcp_esp_alloc(void *arg, u8_t ip_type) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_alloc");
    LWIP_UNUSED_ARG(arg);
    return altcp_esp_new_ip_type(ip_type);
}

struct altcp_pcb *altcp_esp_wrap(struct tcp_pcb *tpcb) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_wrap - NOT IMPLEMENTED");
    return NULL;
}

/* "virtual" functions calling into tcp */
static void altcp_esp_set_poll(struct altcp_pcb *conn, u8_t interval) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_set_poll - NOT IMPLEMENTED");
}

static void altcp_esp_recved(struct altcp_pcb *conn, u16_t len) {
    if (conn == NULL) {
        return;
    }
    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("epcb is NULL !!! -> closing connection");
        esp_conn_close(epcb->econn, 0);
        return;
    }
    ASSERT_EPCB(epcb);

    // Notify stack about received data
    // This passes fake pbuf containing only total length, this may be problem in future.
    esp_pbuf_t pbuf;
    pbuf.tot_len = len;
    esp_conn_recved(epcb->econn, &pbuf);
}

static err_t altcp_esp_bind(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_bind");
    if (conn == NULL) {
        return ERR_VAL;
    }
    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("epcb is NULL !!!");
        return espERR;
    }
    ASSERT_EPCB_VALID(epcb);

    // ESP does not support listening on IP
    epcb->listen_port = port;
    return ERR_OK;
}

static err_t altcp_esp_connect(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port, altcp_connected_fn connected) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_connect"); // TODO: Not properly tested
    if (conn == NULL) {
        return ERR_VAL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("epcb is NULL !!!");
        return espERR;
    }

    memcpy(epcb->host, ip4addr_ntoa(ipaddr), IP4ADDR_STRLEN_MAX);
    const espr_t ret = esp_conn_start(&epcb->econn, ESP_CONN_TYPE_TCP, epcb->host, port, NULL, altcp_esp_evt, 0);
    epcb->alconn->connected = connected;
    return espr_t2err_t(ret);
}

static struct altcp_pcb *altcp_esp_listen(struct altcp_pcb *conn, u8_t backlog, err_t *err) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_listen");
    if (conn == NULL) {
        return NULL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("epcb is NULL !!!");
        return NULL;
    }
    ASSERT_EPCB_VALID(epcb);

    // Enable server on port and set default altcp callback
    if (esp_set_server(1, epcb->listen_port, ESP_U16(ESP_MIN(backlog, ESP_CFG_MAX_CONNS)), epcb->conn_timeout, altcp_esp_evt, NULL, NULL, 1) != espOK) {
        ALTCP_ESP_DEBUG_FN("Failed to set connection to server mode");
    }
    ASSERT_EPCB_VALID(epcb);
    listen_api = epcb;
    return conn;
}

static err_t altcp_esp_close(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_close");

    if (conn == NULL) {
        return ERR_VAL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("epcb is NULL !!!");
        return espERR;
    }
    ASSERT_EPCB(epcb);

    ALTCP_ESP_DEBUG_FN("Closing connection: %d, total %ld packets, %ld bytes", esp_conn_getnum(epcb->econn), epcb->rcv_packets, epcb->rcv_bytes);
    espr_t err = esp_conn_close(epcb->econn, 0);

    if (err != espOK) {
        ALTCP_ESP_DEBUG_FN("Failed to close connection: %d", err);
        return espr_t2err_t(err);
    }

    epcb->closing = 1;

    return ERR_OK;
}

static void altcp_esp_abort(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_abort - Best effort implementation");
    /*
     * FIXME:
     * ESP doesn't seem to have a way to forcefully abort (though it closes
     * through connection reset anyway). And it's close function may fail, for
     * example if it runs out of memory, which is bad. But at least _trying_ is
     * better than nothing, so let's do our best attempt...
     *
     * But we need some better architectural solution (or at least have some
     * kind of memory reserve for closing things).
     */
    altcp_esp_close(conn);
}

// Fortunately not used in our code at all.
static err_t altcp_esp_shutdown(struct altcp_pcb *conn, int shut_rx, int shut_tx) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_shutdown - NOT IMPLEMENTED");
    return ERR_VAL;
}

static err_t altcp_esp_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_write: data:: 0x%x, len: %d", dataptr, len);
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("CONN is NULL !!!");
        return ERR_VAL;
    }
    esp_pcb *epcb = conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("EPCB is NULL !!!");
        return ERR_VAL;
    }
    ASSERT_EPCB(epcb);

    if (epcb->closed) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closed !!!");
        return espCLOSED;
    }

    if (epcb->closing) {
        ALTCP_ESP_DEBUG_FN("EPCB says connection closing !!!");
        return espCLOSED;
    }

    if (!epcb->alconn) {
        ALTCP_ESP_DEBUG_FN("ALCONN is NULL !!!");
        return espERR;
    }

    if (!epcb->econn) {
        ALTCP_ESP_DEBUG_FN("ECONN is NULL !!!");
        return espERR;
    }

    if (!dataptr && len) {
        ALTCP_ESP_DEBUG_FN("dataptr is NULL and len != 0 !!!");
        return espERR;
    }

    espr_t err = esp_conn_write(epcb->econn, dataptr, len, 0, &epcb->buff_avail);
    if (err != espOK) {
        ALTCP_ESP_DEBUG_FN("esp write, err: %d", err);
    }
    return espr_t2err_t(err);
}

static err_t altcp_esp_output(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_output");
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("CONN is NULL !!!");
        return ERR_VAL;
    }
    esp_pcb *epcb = conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("EPCB is NULL !!!");
        return ERR_VAL;
    }
    ASSERT_EPCB(epcb);
    // TODO: This is not perfect, write does something else than just flush.
    // If also deallocates send buffer if there is nothing to send, bug?
    espr_t err = esp_conn_write(epcb->econn, NULL, 0, 1, &epcb->buff_avail);
    if (!epcb->buff_avail) {
        // Possibly there was nothing to write, flush dealocated the buffer
        // and reports 0 as there is no buffer allocated yet.
        epcb->buff_avail = ESP_CFG_CONN_MAX_DATA_LEN;
    }
    return espr_t2err_t(err);
}

static u16_t altcp_esp_mss(struct altcp_pcb *conn) {
    return ESP_TCP_MSS;
}

static u16_t altcp_esp_sndbuf(struct altcp_pcb *conn) {
    if (!conn) {
        ALTCP_ESP_DEBUG_FN("CONN is NULL !!!");
        return ERR_VAL;
    }
    esp_pcb *epcb = conn->state;
    if (!epcb) {
        ALTCP_ESP_DEBUG_FN("EPCB is NULL !!!");
        return ERR_VAL;
    }
    ASSERT_EPCB(epcb);

    ALTCP_ESP_DEBUG_FN("altcp_esp_sndbuf: %d", epcb->buff_avail);
    return epcb->buff_avail;
}

static u16_t altcp_esp_sndqueuelen(struct altcp_pcb *conn) {
    // TODO: Can we do better?
    uint16_t len = ESP_CFG_CONN_MAX_DATA_LEN - altcp_esp_sndbuf(conn);
    ALTCP_ESP_DEBUG_FN("altcp_esp_sndqueuelen: %d", len);
    return len;
}

static void altcp_esp_nagle_disable(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_nagle_disable - NOT IMPLEMENTED");
}

static void altcp_esp_nagle_enable(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_nagle_ensable - NOT IMPLEMENTED");
}

static int altcp_esp_nagle_disabled(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_nagle_disabled - NOT IMPLEMENTED");
    return 0;
}

static void altcp_esp_setprio(struct altcp_pcb *conn, u8_t prio) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_setprio - NOT IMPLEMENTED");
}

static void altcp_esp_dealloc(struct altcp_pcb *conn) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_dealloc");
    esp_pcb *epcb = conn->state;
    if (!epcb) {
        return;
    }

    esp_mem_free(epcb);
    conn->state = NULL;
}

static err_t altcp_esp_get_tcp_addrinfo(struct altcp_pcb *conn, int local, ip_addr_t *addr, u16_t *port) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_get_tcp_addrinfo - NOT IMPLEMENTED");
    return ERR_VAL;
}

static ip_addr_t *altcp_esp_get_ip(struct altcp_pcb *conn, int local) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_get_ip - NOT IMPLEMENTED");
    return NULL;
}

static u16_t altcp_esp_get_port(struct altcp_pcb *conn, int local) {
    ALTCP_ESP_DEBUG_FN("altcp_esp_get_port - NOT IMPLEMENTED");
    return 0;
}

    #ifdef ESP_DEBUG
static enum tcp_state
altcp_esp_dbg_get_tcp_state(struct altcp_pcb *conn) {
    if (conn) {
        struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
        ALTCP_TCP_ASSERT_CONN(conn);
        if (pcb) {
            return pcb->state;
        }
    }
    return CLOSED;
}
    #endif
const struct altcp_functions altcp_esp_functions = {
    altcp_esp_set_poll,
    altcp_esp_recved,
    altcp_esp_bind,
    altcp_esp_connect,
    altcp_esp_listen,
    altcp_esp_abort,
    altcp_esp_close,
    altcp_esp_shutdown,
    altcp_esp_write,
    altcp_esp_output,
    altcp_esp_mss,
    altcp_esp_sndbuf,
    altcp_esp_sndqueuelen,
    altcp_esp_nagle_disable,
    altcp_esp_nagle_enable,
    altcp_esp_nagle_disabled,
    altcp_esp_setprio,
    altcp_esp_dealloc,
    altcp_esp_get_tcp_addrinfo,
    altcp_esp_get_ip,
    altcp_esp_get_port
    #ifdef ESP_DEBUG
    ,
    altcp_esp_dbg_get_tcp_state
    #endif
};

#endif /* ESP_ALTCP */
