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

#include "dbg.h"

#if ESP_ALTCP /* don't build if not configured for use in espopts.h */

    #include "esp/esp.h"
    #include <string.h>
    #include "lwip/altcp.h"
    #include "lwip/priv/altcp_priv.h"

    #include "sockets/lwesp_sockets_priv.h"

    #include "lwip/tcp.h"

    #include "esp_tcp.h"
    #include "esp/esp_mem.h"

/* Variable prototype, the actual declaration is at the end of this file
   since it contains pointers to static functions declared here */
extern const struct altcp_functions altcp_esp_functions;

static void altcp_esp_setup(struct altcp_pcb *conn, esp_pcb *epcb);

static err_t espr_t2err_t(const espr_t err) {
    switch (err) {
    case espOK:
        return ERR_OK;    /*!< Function succeeded */
    case espOKIGNOREMORE: /*!< Function succedded, should continue as espOK but ignore sending more data. This result is possible on connection data receive callback */
        _dbg("espOKIGNOREMORE - pretending all ok");
        return ERR_OK;
    case espERR:
        _dbg("Generic ESP err");
        return ERR_IF;
    case espERRCONNTIMEOUT: /*!< Timeout received when connection to access point */
        _dbg("espERRCONNTIMEOUT");
        return ERR_IF;
    case espERRPASS: /*!< Invalid password for access point */
        _dbg("espERRPASS");
        return ERR_IF;
    case espERRNOAP: /*!< No access point found with specific SSID and MAC address */
        _dbg("espERRNOAP");
        return ERR_IF;
    case espERRCONNFAIL: /*!< Connection failed to access point */
        _dbg("espERRCONNFAIL");
        return ERR_IF;
    case espERRWIFINOTCONNECTED: /*!< Wifi not connected to access point */
        _dbg("espERRWIFINOTCONNECTED");
        return ERR_IF;
    case espERRNODEVICE: /*!< Device is not present */
        _dbg("espERRNODEVICE");
        return ERR_IF;
    case espCONT:
        _dbg("espCONT"); /*!< There is still some command to be processed in current command */
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
        _dbg("Unknown ESP err");
        return -1;
    }
}

static void ALTCP_TCP_ASSERT_CONN_PCB(struct altcp_pcb *conn, esp_pcb *epcb) {
    if (!conn) {
        _dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !conn");
    }

    if (!epcb) {
        _dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !epcb");
    }

    if (conn->state != epcb) {
        _dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! conn->state: %x != epcb: %x", conn->state, epcb);
    }

    if (epcb->alconn != conn) {
        _dbg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! epcb->alconn: %x != conn: %x", epcb->alconn, conn);
    }
}

/* callback functions for TCP */
static err_t
altcp_esp_accept(void *arg, esp_pcb *new_epcb, err_t err) {
    _dbg("altcp_esp_accept");
    struct altcp_pcb *listen_conn = (struct altcp_pcb *)arg;
    if (!listen_conn || !listen_conn->accept) {
        _dbg("!!! listen connection not set properly !!!");
        return ERR_ARG;
    }

    /* create a new altcp_conn to pass to the next 'accept' callback */
    struct altcp_pcb *new_conn = altcp_alloc();
    if (new_conn == NULL) {
        _dbg("No mem to alloc altcp !!!");
        return ERR_MEM;
    }
    altcp_esp_setup(new_conn, new_epcb);
    ALTCP_TCP_ASSERT_CONN_PCB(new_conn, new_epcb);
    err_t ret = listen_conn->accept(listen_conn->arg, new_conn, err);
    return ret;
}

static err_t
altcp_esp_connected(void *arg, esp_pcb *epcb, err_t err) {
    _dbg("altcp_esp_connected");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (epcb->alconn && epcb->alconn->connected) {
        epcb->alconn->connected(conn->arg, conn, 0);
    }
    return ERR_OK;
}

static err_t
altcp_esp_recv(void *arg, esp_pcb *epcb, struct pbuf *p, err_t err) {
    _dbg("altcp_esp_recv");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;

    if (conn) {
        ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
        if (conn->recv) {
            // TODO: This takes ~500ms to execute.
            // Unfortunately UART buffer overflows in the meantime.
    #if (DBG_LEVEL > 0)
            const long start = xTaskGetTickCount();
    #endif
            err_t ret = conn->recv(conn->arg, conn, p, err);
    #if (DBG_LEVEL > 0)
            const long end = xTaskGetTickCount();
            _dbg("recv callback in %ld ms", (end - start) / portTICK_RATE_MS);
    #endif
            return ret;
        }
    }
    if (p != NULL) {
        // prevent memory leaks
        pbuf_free(p);
    }
    return ERR_OK;
}

static err_t
altcp_esp_sent(void *arg, esp_pcb *epcb, u16_t len) {
    _dbg("altcp_esp_sent");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (conn) {
        ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
        if (conn->sent) {
            return conn->sent(conn->arg, conn, len);
        }
    }
    return ERR_OK;
}

static err_t
altcp_esp_poll(void *arg, esp_pcb *epcb) {
    _dbg("altcp_esp_poll");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (conn) {
        ALTCP_TCP_ASSERT_CONN_PCB(conn, epcb);
        if (conn->poll) {
            err_t ret = conn->poll(conn->arg, conn);
            return ret;
        }
    }
    return ERR_OK;
}

static void
altcp_esp_err(void *arg, err_t err) {
    _dbg("altcp_esp_err");
    struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    if (conn) {
        conn->state = NULL; /* already freed */
        if (conn->err) {
            conn->err(conn->arg, err);
        }
        altcp_free(conn);
    }
}

/* setup functions */

static void
altcp_esp_setup(struct altcp_pcb *conn, esp_pcb *epcb) {
    _dbg("altcp_esp_setup");
    conn->state = epcb;
    conn->fns = &altcp_esp_functions;
    epcb->alconn = conn;
}

LWIP_MEMPOOL_DECLARE(EPCB_POOL, EPCB_POOL_SIZE, sizeof(esp_pcb), "ESP PCB pool");

static int memp_initialized = 0;

static esp_pcb *esp_new_ip_type(u8_t ip_type) {
    if (!memp_initialized) {
        memp_initialized = 1;
        LWIP_MEMPOOL_INIT(EPCB_POOL);
    }

    esp_pcb *pcb = (esp_pcb *)LWIP_MEMPOOL_ALLOC(EPCB_POOL);
    if (pcb) {
        memset(pcb, 0, sizeof(esp_pcb));
    }
    return pcb;
}

static void esp_ip_free(esp_pcb *epcb) {
    if (epcb) {
        LWIP_MEMPOOL_FREE(EPCB_POOL, epcb);
    }
}

static esp_pcb *listen_api = NULL;

static void custom_pbuf_free(struct pbuf *p) {
    // This actually holds reference to esp pbuf backing this ones pbuf data
    esp_pbuf_free((esp_pbuf_p)p->next);

    // Free this custom pbuf (as first member the address is also usable to free whole custom pbuf)
    esp_mem_free((struct esp_pbuf_custom *)p);
}

static espr_t altcp_esp_evt(esp_evt_t *evt) {
    esp_conn_p conn;
    esp_pcb *epcb = NULL;
    uint8_t close = 0;

    conn = esp_conn_get_from_evt(evt);
    _dbg("Event from conn %d", esp_conn_getnum(conn));
    switch (esp_evt_get_type(evt)) {
    case ESP_EVT_CONN_ACTIVE: {
        _dbg("ESP_EVT_CONN_ACTIVE");
        if (esp_conn_is_client(conn)) {
            _dbg("ESP_EVT_CONN_ACTIVE - CLIENT");
            epcb = esp_conn_get_arg(conn);
            if (epcb != NULL) {
                epcb->econn = conn;
                altcp_esp_connected(epcb->alconn, epcb, 0);
            } else {
                _dbg("ACTIVE CLIENT WITHOUT EPCB POINTER !!!!");
                close = 1;
            }
        } else if (esp_conn_is_server(conn) && listen_api != NULL) {
            _dbg("ESP_EVT_CONN_ACTIVE - SERVER");
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
                _dbg("esp_pcb not created");
                close = 1;
            }
        } else {
            _dbg("ESP_EVT_CONN_ACTIVE - OTHER");
            ESP_DEBUGW(ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, listen_api == NULL,
                "[ESPTCP] Closing connection as there is no listening API in ESP PCB!\r\n");
            close = 1; /* Close the connection at this point */
        }

        /* Decide if some events want to close the connection */
        if (close) {
            if (epcb != NULL) {
                _dbg("Closing conn %d", esp_conn_getnum(conn));
                esp_conn_set_arg(conn, NULL); /* Reset argument */
                if (epcb->alconn != NULL) {
                    altcp_free(epcb->alconn);
                }
                esp_ip_free(epcb);
            }
            esp_conn_close(conn, 0); /* Close the connection */
            close = 0;
        }
        break;
    }

    /*
         * We have a new data received which
         * should have esp pcb structure as argument
         */
    case ESP_EVT_CONN_RECV: {
        _dbg("ESP_EVT_CONN_RECV");
        esp_pbuf_p pbuf;

        epcb = esp_conn_get_arg(conn);          /* Get API from connection */
        pbuf = esp_evt_conn_recv_get_buff(evt); /* Get received buff */
        if (!epcb) {
            if (pbuf) {
                esp_pbuf_free(pbuf);
            }
            return espERR;
        }
        esp_conn_recved(conn, pbuf); /* Notify stack about received data */
        epcb->rcv_packets++;         /* Increase number of received packets */
        epcb->rcv_bytes += esp_pbuf_length(pbuf, 0);
        _dbg("Received %ld packets, %ld bytes", epcb->rcv_packets, epcb->rcv_bytes);

        if (esp_pbuf_length(pbuf, 0) != esp_pbuf_length(pbuf, 1)) {
            _dbg("!!! rcv pbuf has multiple parts, this is not supported !!!!");
        }

        struct pbuf_custom *custom_pbuf = esp_mem_alloc(sizeof(struct pbuf_custom));
        custom_pbuf->custom_free_function = custom_pbuf_free;
        const size_t recv_len = esp_pbuf_length(pbuf, 0);
        struct pbuf *lwip_pbuf = pbuf_alloced_custom(PBUF_RAW, recv_len, PBUF_REF, custom_pbuf, (char *)esp_pbuf_data(pbuf), recv_len);
        if (!lwip_pbuf) {
            _dbg("Failed to obtain custom LwIP pbuf, len: %ld", recv_len);
            esp_pbuf_free(pbuf);
            esp_mem_free(custom_pbuf);
            return espERR;
        }
        lwip_pbuf->next = (struct pbuf *)pbuf; // Abuse next to hold reference to underlying ESP pbuf

        // Run the recv callback with lwip pbuf
        altcp_esp_recv(epcb->alconn, epcb, lwip_pbuf, 0);

        break;
    }

    /* Connection was just closed */
    case ESP_EVT_CONN_CLOSED: {
        _dbg("ESP_EVT_CONN_CLOSED");
        epcb = esp_conn_get_arg(conn); /* Get API from connection */

        if (epcb) {
            _dbg("Connection closed and epcb not NULL -> free, err");
            struct altcp_pcb *pcb = epcb->alconn;
            esp_conn_set_arg(conn, NULL);
            esp_ip_free(epcb);
            altcp_esp_err(pcb, ERR_CLSD);
        }

        break;
    }
    case ESP_EVT_CONN_SEND:
        _dbg("ESP_EVT_CONN_SEND");
        epcb = esp_conn_get_arg(conn);
        if (epcb) {
            struct altcp_pcb *pcb = epcb->alconn;
            const size_t sent = esp_evt_conn_send_get_length(evt);
            altcp_esp_sent(pcb, epcb, sent);
        }

        break;

    case ESP_EVT_CONN_POLL:
        _dbg("ESP_EVT_CONN_POLL");
        epcb = esp_conn_get_arg(conn);
        if (epcb == NULL) {
            _dbg("epcb is NULL !!!");
            esp_conn_close(conn, 0);
            return espERR;
        }
        altcp_esp_poll(epcb->alconn, epcb);
        break;
    default:
        _dbg("Unknown event type: %d", esp_evt_get_type(evt));
        return espERR;
    }
    return espOK;
}

struct altcp_pcb *
altcp_esp_new_ip_type(u8_t ip_type) {
    _dbg("altcp_esp_new_ip_type");
    /* Allocate the tcp pcb first to invoke the priority handling code
     if we're out of pcbs */
    esp_pcb *epcb = esp_new_ip_type(ip_type);
    if (epcb != NULL) {
        struct altcp_pcb *ret = altcp_alloc();
        if (ret != NULL) {
            altcp_esp_setup(ret, epcb);
            return ret;
        } else {
            /* altcp_pcb allocation failed -> free the tcp_pcb too */
            esp_ip_free(epcb);
        }
    }
    return NULL;
}

/** altcp_esp allocator function fitting to @ref altcp_allocator_t / @ref altcp_new.
*
* arg pointer is not used for TCP.
*/
struct altcp_pcb *
altcp_esp_alloc(void *arg, u8_t ip_type) {
    _dbg("altcp_esp_alloc");
    LWIP_UNUSED_ARG(arg);
    return altcp_esp_new_ip_type(ip_type);
}

struct altcp_pcb *
altcp_esp_wrap(struct tcp_pcb *tpcb) {
    _dbg("altcp_esp_wrap - NOT IMPLEMENTED");
    return NULL;
}

/* "virtual" functions calling into tcp */
static void
altcp_esp_set_poll(struct altcp_pcb *conn, u8_t interval) {
    _dbg("altcp_esp_set_poll - NOT IMPLEMENTED");
}

static void
altcp_esp_recved(struct altcp_pcb *conn, u16_t len) {
    // ESP has already acknowedged the data, nothing to do here.
}

static err_t
altcp_esp_bind(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port) {
    _dbg("altcp_esp_bind");
    if (conn == NULL) {
        return ERR_VAL;
    }
    esp_pcb *epcb = (esp_pcb *)conn->state;
    // ESP does not support listening on IP
    epcb->listen_port = port;
    return ERR_OK;
}

static err_t
altcp_esp_connect(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port, altcp_connected_fn connected) {
    _dbg("altcp_esp_connect"); // TODO: Not properly tested
    if (conn == NULL) {
        return ERR_VAL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;
    memcpy(epcb->host, ip4addr_ntoa(ipaddr), IP4ADDR_STRLEN_MAX);
    const espr_t ret = esp_conn_start(&epcb->econn, ESP_CONN_TYPE_TCP, epcb->host, port, NULL, altcp_esp_evt, 0);
    epcb->alconn->connected = connected;
    return espr_t2err_t(ret);
}

static struct altcp_pcb *
altcp_esp_listen(struct altcp_pcb *conn, u8_t backlog, err_t *err) {
    _dbg("altcp_esp_listen");
    if (conn == NULL) {
        return NULL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;

    // Enable server on port and set default altcp callback
    if (esp_set_server(1, epcb->listen_port, ESP_U16(ESP_MIN(backlog, ESP_CFG_MAX_CONNS)), epcb->conn_timeout, altcp_esp_evt, NULL, NULL, 1) != espOK) {
        _dbg("Failed to set connection to server mode");
    }
    listen_api = epcb;
    return conn;
}

static void
altcp_esp_abort(struct altcp_pcb *conn) {
    _dbg("altcp_esp_abort - NOT IMPLEMENTED");
}

static err_t
altcp_esp_close(struct altcp_pcb *conn) {
    _dbg("altcp_esp_close");

    if (conn == NULL) {
        return ERR_VAL;
    }

    esp_pcb *epcb = (esp_pcb *)conn->state;
    if (epcb) {
        _dbg("Closing connection: %d, total %ld packets, %ld bytes", esp_conn_getnum(epcb->econn), epcb->rcv_packets, epcb->rcv_bytes);
        espr_t err = esp_conn_close(epcb->econn, 0);

        if (err != espOK) {
            _dbg("Failed to close connection: %d", err);
            return espr_t2err_t(err);
        }
    }

    return ERR_OK;
}

static err_t
altcp_esp_shutdown(struct altcp_pcb *conn, int shut_rx, int shut_tx) {
    _dbg("altcp_esp_shutdown - NOT IMPLEMENTED");
    return ERR_VAL;
}

static err_t
altcp_esp_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags) {
    _dbg("altcp_esp_write");
    if (conn == NULL) {
        return ERR_VAL;
    }
    esp_pcb *epcb = conn->state;
    if (epcb == NULL) {
        return ERR_VAL;
    }
    size_t written = 0;
    espr_t err = esp_conn_send(epcb->econn, dataptr, len, &written, 0); // TODO: Flags ignored, we could only set blocking
    _dbg("esp writen: %d commited, err: %d", len, err);
    return espr_t2err_t(err);
}

static err_t
altcp_esp_output(struct altcp_pcb *conn) {
    _dbg("altcp_esp_output - NOT IMPLEMENTED");
    return ERR_VAL;
}

static u16_t
altcp_esp_mss(struct altcp_pcb *conn) {
    return 536; // Minimal required MSS, TODO: Implement properly
}

static u16_t
altcp_esp_sndbuf(struct altcp_pcb *conn) {
    return 536 / 2; // TODO: Some reasoneable size. Reading from ESP would be better
}

static u16_t
altcp_esp_sndqueuelen(struct altcp_pcb *conn) {
    _dbg("altcp_esp_sndqueuelen");
    return 0; // TODO: Implement properly
}

static void
altcp_esp_nagle_disable(struct altcp_pcb *conn) {
    _dbg("altcp_esp_nagle_disable - NOT IMPLEMENTED");
}

static void
altcp_esp_nagle_enable(struct altcp_pcb *conn) {
    _dbg("altcp_esp_nagle_ensable - NOT IMPLEMENTED");
}

static int
altcp_esp_nagle_disabled(struct altcp_pcb *conn) {
    _dbg("altcp_esp_nagle_disabled - NOT IMPLEMENTED");
    return 0;
}

static void
altcp_esp_setprio(struct altcp_pcb *conn, u8_t prio) {
    _dbg("altcp_esp_setprio - NOT IMPLEMENTED");
}

static void
altcp_esp_dealloc(struct altcp_pcb *conn) {
    _dbg("altcp_esp_dealloc");
    esp_pcb *epcb = conn->state;
    if (epcb) {
        esp_ip_free(epcb);
        conn->state = NULL;
    }
}

static err_t
altcp_esp_get_tcp_addrinfo(struct altcp_pcb *conn, int local, ip_addr_t *addr, u16_t *port) {
    _dbg("altcp_esp_get_tcp_addrinfo - NOT IMPLEMENTED");
    return ERR_VAL;
}

static ip_addr_t *
altcp_esp_get_ip(struct altcp_pcb *conn, int local) {
    _dbg("altcp_esp_get_ip - NOT IMPLEMENTED");
    return NULL;
}

static u16_t
altcp_esp_get_port(struct altcp_pcb *conn, int local) {
    _dbg("altcp_esp_get_port - NOT IMPLEMENTED");
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
