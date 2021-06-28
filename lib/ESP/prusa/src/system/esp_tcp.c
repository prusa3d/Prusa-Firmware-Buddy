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
 *
 */

#include "esp/esp_config.h"

#if ESP_ALTCP /* don't build if not configured for use in espopts.h */

    #include "esp/esp.h"
    #include <string.h>
    #include "lwip/altcp.h"
    #include "lwip/priv/altcp_priv.h"
    #include "esp/esp_netconn.h"

    #include "lwip/tcp.h"
// #include "lwip/mem.h"
//#include "lwip/altcp_tcp.h"

/* Variable prototype, the actual declaration is at the end of this file
   since it contains pointers to static functions declared here */
extern const struct altcp_functions altcp_esp_functions;

static void altcp_esp_setup(struct altcp_pcb *conn, esp_netconn_p tpcb);

/* callback functions for TCP */
static err_t
altcp_esp_accept(void *arg, esp_netconn_p new_tpcb, err_t err) {
    struct altcp_pcb *listen_conn = (struct altcp_pcb *)arg;
    if (listen_conn && listen_conn->accept) {
        /* create a new altcp_conn to pass to the next 'accept' callback */
        struct altcp_pcb *new_conn = altcp_alloc();
        if (new_conn == NULL) {
            return ERR_MEM;
        }
        altcp_esp_setup(new_conn, new_tpcb);
        return listen_conn->accept(listen_conn->arg, new_conn, err);
    }
    return ERR_ARG;
}

static err_t
altcp_esp_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    return ERR_OK;
}

static err_t
altcp_esp_recv(void *arg, struct altcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // esp_netconn_p conn = (esp_netconn_p)arg;
    // if (conn) {
    //   esp_netconn_receive(conn, )
    // }
    // if (p != NULL) {
    //   /* prevent memory leaks */
    //   pbuf_free(p);
    // }
    return ERR_OK;
}

static err_t
altcp_esp_sent(void *arg, struct altcp_pcb *tpcb, u16_t len) {
    // struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    // if (conn) {
    //   ALTCP_TCP_ASSERT_CONN_PCB(conn, tpcb);
    //   if (conn->sent) {
    //     return conn->sent(conn->arg, conn, len);
    //   }
    // }
    return ERR_OK;
}

static err_t
altcp_esp_poll(void *arg, struct altcp_pcb *tpcb) {
    // struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    // if (conn) {
    //   ALTCP_TCP_ASSERT_CONN_PCB(conn, tpcb);
    //   if (conn->poll) {
    //     return conn->poll(conn->arg, conn);
    //   }
    // }
    return ERR_OK;
}

static void
altcp_esp_err(void *arg, err_t err) {
    // struct altcp_pcb *conn = (struct altcp_pcb *)arg;
    // if (conn) {
    //   conn->state = NULL; /* already freed */
    //   if (conn->err) {
    //     conn->err(conn->arg, err);
    //   }
    //   altcp_free(conn);
    // }
}

/* setup functions */

static void
altcp_esp_remove_callbacks(struct altcp_pcb *pcb) {
    LWIP_ASSERT_CORE_LOCKED();
    if (pcb != NULL) {
        pcb->recv = NULL;
        pcb->sent = NULL;
        pcb->err = NULL;
        pcb->poll = NULL;
    }
    // tcp_arg(tpcb, NULL);
    // tcp_recv(tpcb, NULL);
    // tcp_sent(tpcb, NULL);
    // tcp_err(tpcb, NULL);
    // tcp_poll(tpcb, NULL, tpcb->pollinterval);
}

static void
altcp_esp_setup_callbacks(struct altcp_pcb *pcb, esp_netconn_p tpcb) {
    LWIP_ASSERT_CORE_LOCKED();
    if (pcb != NULL) {
        pcb->recv = altcp_esp_recv;
        pcb->sent = altcp_esp_sent;
        pcb->err = altcp_esp_err;
    }

    // tcp_arg(tpcb, conn);
    /* tcp_poll is set when interval is set by application */
    /* listen is set totally different :-) */
}

static void
altcp_esp_setup(struct altcp_pcb *conn, esp_netconn_p tpcb) {
    altcp_esp_setup_callbacks(conn, tpcb);
    conn->state = tpcb;
    conn->fns = &altcp_esp_functions;
}

struct altcp_pcb *
altcp_esp_new_ip_type(u8_t ip_type) {
    /* Allocate the tcp pcb first to invoke the priority handling code
     if we're out of pcbs */
    esp_netconn_p tpcb = esp_netconn_new(ip_type);
    if (tpcb != NULL) {
        struct altcp_pcb *ret = altcp_alloc();
        if (ret != NULL) {
            altcp_esp_setup(ret, tpcb);
            return ret;
        } else {
            /* altcp_pcb allocation failed -> free the tcp_pcb too */
            esp_netconn_delete(tpcb);
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
    LWIP_UNUSED_ARG(arg);
    return altcp_esp_new_ip_type(ip_type);
}

struct altcp_pcb *
altcp_esp_wrap(struct tcp_pcb *tpcb) {
    // if (tpcb != NULL) {
    //   struct altcp_pcb *ret = altcp_alloc();
    //   if (ret != NULL) {
    //     altcp_esp_setup(ret, tpcb);
    //     return ret;
    //   }
    // }
    return NULL;
}

/* "virtual" functions calling into tcp */
static void
altcp_esp_set_poll(struct altcp_pcb *conn, u8_t interval) {
    // if (conn != NULL) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   tcp_poll(pcb, altcp_esp_poll, interval);
    // }
}

static void
altcp_esp_recved(struct altcp_pcb *conn, u16_t len) {
    // if (conn != NULL) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   tcp_recved(pcb, len);
    // }
}

static err_t
altcp_esp_bind(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_bind(pcb, ipaddr, port);
}

static err_t
altcp_esp_connect(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port, altcp_connected_fn connected) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // conn->connected = connected;
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_connect(pcb, ipaddr, port, altcp_esp_connected);
}

static struct altcp_pcb *
altcp_esp_listen(struct altcp_pcb *conn, u8_t backlog, err_t *err) {
    esp_netconn_p pcb;
    esp_netconn_p lpcb;
    if (conn == NULL) {
        return NULL;
    }

    pcb = (esp_netconn_p)conn->state;
    lpcb = esp_netconn_listen(pcb);
    if (lpcb != NULL) {
        conn->state = lpcb;
        esp_netconn_accept()
            tcp_accept(lpcb, altcp_esp_accept);
        return conn;
    }
    return NULL;
}

static void
altcp_esp_abort(struct altcp_pcb *conn) {
    // if (conn != NULL) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   if (pcb) {
    //     tcp_abort(pcb);
    //   }
    // }
}

static err_t
altcp_esp_close(struct altcp_pcb *conn) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    //   return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // if (pcb) {
    //   err_t err;
    //   tcp_poll_fn oldpoll = pcb->poll;
    //   altcp_esp_remove_callbacks(pcb);
    //   err = tcp_close(pcb);
    //   if (err != ERR_OK) {
    //     /* not closed, set up all callbacks again */
    //     altcp_esp_setup_callbacks(conn, pcb);
    //     /* poll callback is not included in the above */
    //     tcp_poll(pcb, oldpoll, pcb->pollinterval);
    //     return err;
    //   }
    //   conn->state = NULL; /* unsafe to reference pcb after tcp_close(). */
    // }
    // altcp_free(conn);
    return ERR_OK;
}

static err_t
altcp_esp_shutdown(struct altcp_pcb *conn, int shut_rx, int shut_tx) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_shutdown(pcb, shut_rx, shut_tx);
}

static err_t
altcp_esp_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_write(pcb, dataptr, len, apiflags);
}

static err_t
altcp_esp_output(struct altcp_pcb *conn) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return ERR_VAL;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_output(pcb);
}

static u16_t
altcp_esp_mss(struct altcp_pcb *conn) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return 0;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_mss(pcb);
}

static u16_t
altcp_esp_sndbuf(struct altcp_pcb *conn) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return 0;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_sndbuf(pcb);
}

static u16_t
altcp_esp_sndqueuelen(struct altcp_pcb *conn) {
    // struct tcp_pcb *pcb;
    // if (conn == NULL) {
    return 0;
    // }
    // ALTCP_TCP_ASSERT_CONN(conn);
    // pcb = (struct tcp_pcb *)conn->state;
    // return tcp_sndqueuelen(pcb);
}

static void
altcp_esp_nagle_disable(struct altcp_pcb *conn) {
    // if (conn && conn->state) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   tcp_nagle_disable(pcb);
    // }
}

static void
altcp_esp_nagle_enable(struct altcp_pcb *conn) {
    // if (conn && conn->state) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   tcp_nagle_enable(pcb);
    // }
}

static int
altcp_esp_nagle_disabled(struct altcp_pcb *conn) {
    // if (conn && conn->state) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   return tcp_nagle_disabled(pcb);
    // }
    return 0;
}

static void
altcp_esp_setprio(struct altcp_pcb *conn, u8_t prio) {
    // if (conn != NULL) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   tcp_setprio(pcb, prio);
    // }
}

static void
altcp_esp_dealloc(struct altcp_pcb *conn) {
    // ESP_UNUSED_ARG(conn);
    // ALTCP_TCP_ASSERT_CONN(conn);
    /* no private state to clean up */
}

static err_t
altcp_esp_get_tcp_addrinfo(struct altcp_pcb *conn, int local, ip_addr_t *addr, u16_t *port) {
    // if (conn) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   return tcp_tcp_get_tcp_addrinfo(pcb, local, addr, port);
    // }
    return ERR_VAL;
}

static ip_addr_t *
altcp_esp_get_ip(struct altcp_pcb *conn, int local) {
    // if (conn) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   if (pcb) {
    //     if (local) {
    //       return &pcb->local_ip;
    //     } else {
    //       return &pcb->remote_ip;
    //     }
    //   }
    // }
    return NULL;
}

static u16_t
altcp_esp_get_port(struct altcp_pcb *conn, int local) {
    // if (conn) {
    //   struct tcp_pcb *pcb = (struct tcp_pcb *)conn->state;
    //   ALTCP_TCP_ASSERT_CONN(conn);
    //   if (pcb) {
    //     if (local) {
    //       return pcb->local_port;
    //     } else {
    //       return pcb->remote_port;
    //     }
    //   }
    // }
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
