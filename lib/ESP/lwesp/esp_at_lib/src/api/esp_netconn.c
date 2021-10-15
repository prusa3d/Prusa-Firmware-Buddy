/**
 * \file            esp_netconn.c
 * \brief           API functions for sequential calls
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#include "esp/esp_netconn.h"
#include "esp/esp_private.h"
#include "esp/esp_conn.h"
#include "esp/esp_mem.h"

#if ESP_CFG_NETCONN || __DOXYGEN__

/**
 * \brief           Sequential API structure
 */
typedef struct esp_netconn {
    struct esp_netconn* next;                   /*!< Linked list entry */

    esp_netconn_type_t type;                    /*!< Netconn type */
    esp_port_t listen_port;                     /*!< Port on which we are listening */

    size_t rcv_packets;                         /*!< Number of received packets so far on this connection */
    esp_conn_p conn;                            /*!< Pointer to actual connection */

    esp_sys_mbox_t mbox_accept;                 /*!< List of active connections waiting to be processed */
    esp_sys_mbox_t mbox_receive;                /*!< Message queue for receive mbox */
    size_t mbox_receive_entries;                /*!< Number of entries written to receive mbox */

    esp_linbuff_t buff;                         /*!< Linear buffer structure */

    uint16_t conn_timeout;                      /*!< Connection timeout in units of seconds when
                                                    netconn is in server (listen) mode.
                                                    Connection will be automatically closed if there is no
                                                    data exchange in time. Set to `0` when timeout feature is disabled. */

#if ESP_CFG_NETCONN_RECEIVE_TIMEOUT || __DOXYGEN__
    uint32_t rcv_timeout;                       /*!< Receive timeout in unit of milliseconds */
#endif
} esp_netconn_t;

static uint8_t recv_closed = 0xFF, recv_not_present = 0xFF;
static esp_netconn_t* listen_api;               /*!< Main connection in listening mode */
static esp_netconn_t* netconn_list;             /*!< Linked list of netconn entries */

/**
 * \brief           Flush all mboxes and clear possible used memories
 * \param[in]       nc: Pointer to netconn to flush
 */
static void
flush_mboxes(esp_netconn_t* nc, uint8_t protect) {
    esp_pbuf_p pbuf;
    esp_netconn_t* new_nc;
    if (protect) {
        esp_core_lock();
    }
    if (esp_sys_mbox_isvalid(&nc->mbox_receive)) {
        while (esp_sys_mbox_getnow(&nc->mbox_receive, (void **)&pbuf)) {
            if (nc->mbox_receive_entries > 0) {
                nc->mbox_receive_entries--;
            }
            if (pbuf != NULL && (uint8_t *)pbuf != (uint8_t *)&recv_closed) {
                esp_pbuf_free(pbuf);            /* Free received data buffers */
            }
        }
        esp_sys_mbox_delete(&nc->mbox_receive); /* Delete message queue */
        esp_sys_mbox_invalid(&nc->mbox_receive);/* Invalid handle */
    }
    if (esp_sys_mbox_isvalid(&nc->mbox_accept)) {
        while (esp_sys_mbox_getnow(&nc->mbox_accept, (void **)&new_nc)) {
            if (new_nc != NULL
                && (uint8_t *)new_nc != (uint8_t *)&recv_closed
                && (uint8_t *)new_nc != (uint8_t *)&recv_not_present) {
                esp_netconn_close(new_nc);      /* Close netconn connection */
            }
        }
        esp_sys_mbox_delete(&nc->mbox_accept);  /* Delete message queue */
        esp_sys_mbox_invalid(&nc->mbox_accept); /* Invalid handle */
    }
    if (protect) {
        esp_core_unlock();
    }
}

/**
 * \brief           Callback function for every server connection
 * \param[in]       evt: Pointer to callback structure
 * \return          Member of \ref espr_t enumeration
 */
static espr_t
netconn_evt(esp_evt_t* evt) {
    esp_conn_p conn;
    esp_netconn_t* nc = NULL;
    uint8_t close = 0;

    conn = esp_conn_get_from_evt(evt);          /* Get connection from event */
    switch (esp_evt_get_type(evt)) {
        /*
         * A new connection has been active
         * and should be handled by netconn API
         */
        case ESP_EVT_CONN_ACTIVE: {             /* A new connection active is active */
            if (esp_conn_is_client(conn)) {     /* Was connection started by us? */
                nc = esp_conn_get_arg(conn);    /* Argument should be already set */
                if (nc != NULL) {
                    nc->conn = conn;            /* Save actual connection */
                } else {
                    close = 1;                  /* Close this connection, invalid netconn */
                }
            } else if (esp_conn_is_server(conn) && listen_api != NULL) {    /* Is the connection server type and we have known listening API? */
                /*
                 * Create a new netconn structure
                 * and set it as connection argument.
                 */
                nc = esp_netconn_new(ESP_NETCONN_TYPE_TCP); /* Create new API */
                ESP_DEBUGW(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING,
                    nc == NULL, "[NETCONN] Cannot create new structure for incoming server connection!\r\n");

                if (nc != NULL) {
                    nc->conn = conn;            /* Set connection handle */
                    esp_conn_set_arg(conn, nc); /* Set argument for connection */

                    /*
                     * In case there is no listening connection,
                     * simply close the connection
                     */
                    if (!esp_sys_mbox_isvalid(&listen_api->mbox_accept)
                        || !esp_sys_mbox_putnow(&listen_api->mbox_accept, nc)) {
                        close = 1;
                    }
                } else {
                    close = 1;
                }
            } else {
                ESP_DEBUGW(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING, listen_api == NULL,
                    "[NETCONN] Closing connection as there is no listening API in netconn!\r\n");
                close = 1;                      /* Close the connection at this point */
            }

            /* Decide if some events want to close the connection */
            if (close) {
                if (nc != NULL) {
                    esp_conn_set_arg(conn, NULL);   /* Reset argument */
                    esp_netconn_delete(nc);     /* Free memory for API */
                }
                esp_conn_close(conn, 0);        /* Close the connection */
                close = 0;
            }
            break;
        }

        /*
         * We have a new data received which
         * should have netconn structure as argument
         */
        case ESP_EVT_CONN_RECV: {
            esp_pbuf_p pbuf;

            nc = esp_conn_get_arg(conn);        /* Get API from connection */
            pbuf = esp_evt_conn_recv_get_buff(evt); /* Get received buff */

#if !ESP_CFG_CONN_MANUAL_TCP_RECEIVE
            esp_conn_recved(conn, pbuf);        /* Notify stack about received data */
#endif /* !ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

            esp_pbuf_ref(pbuf);                 /* Increase reference counter */
            if (nc == NULL || !esp_sys_mbox_isvalid(&nc->mbox_receive)
                || !esp_sys_mbox_putnow(&nc->mbox_receive, pbuf)) {
                ESP_DEBUGF(ESP_CFG_DBG_NETCONN,
                    "[NETCONN] Ignoring more data for receive!\r\n");
                esp_pbuf_free(pbuf);            /* Free pbuf */
                return espOKIGNOREMORE;         /* Return OK to free the memory and ignore further data */
            }
            nc->mbox_receive_entries++;         /* Increase number of packets in receive mbox */
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
            /* Check against 1 less to still allow potential close event to be written to queue */
            if (nc->mbox_receive_entries >= (ESP_CFG_NETCONN_RECEIVE_QUEUE_LEN - 1)) {
                conn->status.f.receive_blocked = 1; /* Block reading more data */
            }
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */

            nc->rcv_packets++;                  /* Increase number of packets received */
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE,
                "[NETCONN] Received pbuf contains %d bytes. Handle written to receive mbox\r\n",
                (int)esp_pbuf_length(pbuf, 0));
            break;
        }

        /* Connection was just closed */
        case ESP_EVT_CONN_CLOSE: {
            nc = esp_conn_get_arg(conn);        /* Get API from connection */

            /*
             * In case we have a netconn available,
             * simply write pointer to received variable to indicate closed state
             */
            if (nc != NULL && esp_sys_mbox_isvalid(&nc->mbox_receive)) {
                if (esp_sys_mbox_putnow(&nc->mbox_receive, (void*)& recv_closed)) {
                    nc->mbox_receive_entries++;
                }
            }

            break;
        }
        default:
            return espERR;
    }
    return espOK;
}

/**
 * \brief           Global event callback function
 * \param[in]       evt: Callback information and data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_evt(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_WIFI_DISCONNECTED: {       /* Wifi disconnected event */
            if (listen_api != NULL) {           /* Check if listen API active */
                esp_sys_mbox_putnow(&listen_api->mbox_accept, &recv_closed);
            }
            break;
        }
        case ESP_EVT_DEVICE_PRESENT: {          /* Device present event */
            if (listen_api != NULL && !esp_device_is_present()) {   /* Check if device present */
                esp_sys_mbox_putnow(&listen_api->mbox_accept, &recv_not_present);
            }
        }
        default: break;
    }
    return espOK;
}

/**
 * \brief           Create new netconn connection
 * \param[in]       type: Netconn connection type
 * \return          New netconn connection on success, `NULL` otherwise
 */
esp_netconn_p
esp_netconn_new(esp_netconn_type_t type) {
    esp_netconn_t* a;
    static uint8_t first = 1;

    /* Register only once! */
    esp_core_lock();
    if (first) {
        first = 0;
        esp_evt_register(esp_evt);              /* Register global event function */
    }
    esp_core_unlock();
    a = esp_mem_calloc(1, sizeof(*a));          /* Allocate memory for core object */
    if (a != NULL) {
        a->type = type;                         /* Save netconn type */
        a->conn_timeout = 0;                    /* Default connection timeout */
        if (!esp_sys_mbox_create(&a->mbox_accept, ESP_CFG_NETCONN_ACCEPT_QUEUE_LEN)) {  /* Allocate memory for accepting message box */
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_DANGER,
                "[NETCONN] Cannot create accept MBOX\r\n");
            goto free_ret;
        }
        if (!esp_sys_mbox_create(&a->mbox_receive, ESP_CFG_NETCONN_RECEIVE_QUEUE_LEN)) {    /* Allocate memory for receiving message box */
            ESP_DEBUGF(ESP_CFG_DBG_NETCONN | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_DANGER,
                "[NETCONN] Cannot create receive MBOX\r\n");
            goto free_ret;
        }
        esp_core_lock();
        if (netconn_list == NULL) {             /* Add new netconn to the existing list */
            netconn_list = a;
        } else {
            a->next = netconn_list;             /* Add it to beginning of the list */
            netconn_list = a;
        }
        esp_core_unlock();
    }
    return a;
free_ret:
    if (esp_sys_mbox_isvalid(&a->mbox_accept)) {
        esp_sys_mbox_delete(&a->mbox_accept);
        esp_sys_mbox_invalid(&a->mbox_accept);
    }
    if (esp_sys_mbox_isvalid(&a->mbox_receive)) {
        esp_sys_mbox_delete(&a->mbox_receive);
        esp_sys_mbox_invalid(&a->mbox_receive);
    }
    if (a != NULL) {
        esp_mem_free_s((void **)&a);
    }
    return NULL;
}

/**
 * \brief           Delete netconn connection
 * \param[in]       nc: Netconn handle
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_delete(esp_netconn_p nc) {
    ESP_ASSERT("netconn != NULL", nc != NULL);

    esp_core_lock();
    flush_mboxes(nc, 0);                        /* Clear mboxes */

    /* Stop listening on netconn */
    if (nc == listen_api) {
        listen_api = NULL;
        esp_core_unlock();
        esp_set_server(0, nc->listen_port, 0, 0, NULL, NULL, NULL, 1);
        esp_core_lock();
    }

    /* Remove netconn from linkedlist */
    if (netconn_list == nc) {
        netconn_list = netconn_list->next;      /* Remove first from linked list */
    } else if (netconn_list != NULL) {
        esp_netconn_p tmp, prev;
        /* Find element on the list */
        for (prev = netconn_list, tmp = netconn_list->next;
            tmp != NULL; prev = tmp, tmp = tmp->next) {
            if (nc == tmp) {
                prev->next = tmp->next;         /* Remove tmp from linked list */
                break;
            }
        }
    }
    esp_core_unlock();

    esp_mem_free_s((void **)&nc);
    return espOK;
}

/**
 * \brief           Connect to server as client
 * \param[in]       nc: Netconn handle
 * \param[in]       host: Pointer to host, such as domain name or IP address in string format
 * \param[in]       port: Target port to use
 * \return          \ref espOK if successfully connected, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_connect(esp_netconn_p nc, const char* host, esp_port_t port) {
    espr_t res;

    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("host != NULL", host != NULL);
    ESP_ASSERT("port > 0", port > 0);

    /*
     * Start a new connection as client and:
     *
     *  - Set current netconn structure as argument
     *  - Set netconn callback function for connection management
     *  - Start connection in blocking mode
     */
    res = esp_conn_start(NULL, (esp_conn_type_t)nc->type, host, port, nc, netconn_evt, 1);
    return res;
}

/**
 * \brief           Bind a connection to specific port, can be only used for server connections
 * \param[in]       nc: Netconn handle
 * \param[in]       port: Port used to bind a connection to
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_bind(esp_netconn_p nc, esp_port_t port) {
    espr_t res = espOK;

    ESP_ASSERT("nc != NULL", nc != NULL);

    /*
     * Protection is not needed as it is expected
     * that this function is called only from single
     * thread for single netconn connection,
     * thus it is considered reentrant
     */

    nc->listen_port = port;

    return res;
}

/**
 * \brief           Set timeout value in units of seconds when connection is in listening mode
 *                  If new connection is accepted, it will be automatically closed after `seconds` elapsed
 *                  without any data exchange.
 * \note            Call this function before you put connection to listen mode with \ref esp_netconn_listen
 * \param[in]       nc: Netconn handle used for listen mode
 * \param[in]       timeout: Time in units of seconds. Set to `0` to disable timeout feature
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_set_listen_conn_timeout(esp_netconn_p nc, uint16_t timeout) {
    espr_t res = espOK;
    ESP_ASSERT("nc != NULL", nc != NULL);

    /*
     * Protection is not needed as it is expected
     * that this function is called only from single
     * thread for single netconn connection,
     * thus it is reentrant in this case
     */

    nc->conn_timeout = timeout;

    return res;
}

/**
 * \brief           Listen on previously binded connection
 * \param[in]       nc: Netconn handle used to listen for new connections
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_listen(esp_netconn_p nc) {
    return esp_netconn_listen_with_max_conn(nc, ESP_CFG_MAX_CONNS);
}

/**
 * \brief           Listen on previously binded connection with max allowed connections at a time
 * \param[in]       nc: Netconn handle used to listen for new connections
 * \param[in]       max_connections: Maximal number of connections server can accept at a time
 *                      This parameter may not be larger than \ref ESP_CFG_MAX_CONNS
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_netconn_listen_with_max_conn(esp_netconn_p nc, uint16_t max_connections) {
    espr_t res;

    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->type must be TCP", nc->type == ESP_NETCONN_TYPE_TCP);

    /* Enable server on port and set default netconn callback */
    if ((res = esp_set_server(1, nc->listen_port,
        ESP_U16(ESP_MIN(max_connections, ESP_CFG_MAX_CONNS)),
        nc->conn_timeout, netconn_evt, NULL, NULL, 1)) == espOK) {
        esp_core_lock();
        listen_api = nc;                        /* Set current main API in listening state */
        esp_core_unlock();
    }
    return res;
}

/**
 * \brief           Accept a new connection
 * \param[in]       nc: Netconn handle used as base connection to accept new clients
 * \param[out]      client: Pointer to netconn handle to save new connection to
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_accept(esp_netconn_p nc, esp_netconn_p* client) {
    esp_netconn_t* tmp;
    uint32_t time;

    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("client != NULL", client != NULL);
    ESP_ASSERT("nc->type must be TCP", nc->type == ESP_NETCONN_TYPE_TCP);
    ESP_ASSERT("nc == listen_api", nc == listen_api);

    *client = NULL;
    time = esp_sys_mbox_get(&nc->mbox_accept, (void **)&tmp, 0);
    if (time == ESP_SYS_TIMEOUT) {
        return espTIMEOUT;
    }
    if ((uint8_t *)tmp == (uint8_t *)&recv_closed) {
        esp_core_lock();
        listen_api = NULL;                      /* Disable listening at this point */
        esp_core_unlock();
        return espERRWIFINOTCONNECTED;          /* Wifi disconnected */
    } else if ((uint8_t *)tmp == (uint8_t *)&recv_not_present) {
        esp_core_lock();
        listen_api = NULL;                      /* Disable listening at this point */
        esp_core_unlock();
        return espERRNODEVICE;                  /* Device not present */
    }
    *client = tmp;                              /* Set new pointer */
    return espOK;                               /* We have a new connection */
}

/**
 * \brief           Write data to connection output buffers
 * \note            This function may only be used on TCP or SSL connections
 * \param[in]       nc: Netconn handle used to write data to
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_write(esp_netconn_p nc, const void* data, size_t btw) {
    size_t len, sent;
    const uint8_t* d = data;
    espr_t res;

    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->type must be TCP or SSL", nc->type == ESP_NETCONN_TYPE_TCP || nc->type == ESP_NETCONN_TYPE_SSL);
    ESP_ASSERT("nc->conn must be active", esp_conn_is_active(nc->conn));

    /*
     * Several steps are done in write process
     *
     * 1. Check if buffer is set and check if there is something to write to it.
     *    1. In case buffer will be full after copy, send it and free memory.
     * 2. Check how many bytes we can write directly without needed to copy
     * 3. Try to allocate a new buffer and copy remaining input data to it
     * 4. In case buffer allocation fails, send data directly (may affect on speed and effectivenes)
     */

    /* Step 1 */
    if (nc->buff.buff != NULL) {                /* Is there a write buffer ready to accept more data? */
        len = ESP_MIN(nc->buff.len - nc->buff.ptr, btw);    /* Get number of bytes we can write to buffer */
        if (len > 0) {
            ESP_MEMCPY(&nc->buff.buff[nc->buff.ptr], data, len);/* Copy memory to temporary write buffer */
            d += len;
            nc->buff.ptr += len;
            btw -= len;
        }

        /* Step 1.1 */
        if (nc->buff.ptr == nc->buff.len) {
            res = esp_conn_send(nc->conn, nc->buff.buff, nc->buff.len, &sent, 1);

            esp_mem_free_s((void **)&nc->buff.buff);
            if (res != espOK) {
                return res;
            }
        } else {
            return espOK;                       /* Buffer is not yet full yet */
        }
    }

    /* Step 2 */
    if (btw >= ESP_CFG_CONN_MAX_DATA_LEN) {
        size_t rem;
        rem = btw % ESP_CFG_CONN_MAX_DATA_LEN;  /* Get remaining bytes for max data length */
        res = esp_conn_send(nc->conn, d, btw - rem, &sent, 1);  /* Write data directly */
        if (res != espOK) {
            return res;
        }
        d += sent;                              /* Advance in data pointer */
        btw -= sent;                            /* Decrease remaining data to send */
    }

    if (btw == 0) {                             /* Sent everything? */
        return espOK;
    }

    /* Step 3 */
    if (nc->buff.buff == NULL) {                /* Check if we should allocate a new buffer */
        nc->buff.buff = esp_mem_malloc(sizeof(*nc->buff.buff) * ESP_CFG_CONN_MAX_DATA_LEN);
        nc->buff.len = ESP_CFG_CONN_MAX_DATA_LEN;   /* Save buffer length */
        nc->buff.ptr = 0;                       /* Save buffer pointer */
    }

    /* Step 4 */
    if (nc->buff.buff != NULL) {                /* Memory available? */
        ESP_MEMCPY(&nc->buff.buff[nc->buff.ptr], d, btw);   /* Copy data to buffer */
        nc->buff.ptr += btw;
    } else {                                    /* Still no memory available? */
        return esp_conn_send(nc->conn, data, btw, NULL, 1); /* Simply send directly blocking */
    }
    return espOK;
}

/**
 * \brief           Flush buffered data on netconn \e TCP/SSL connection
 * \note            This function may only be used on \e TCP/SSL connection
 * \param[in]       nc: Netconn handle to flush data
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_flush(esp_netconn_p nc) {
    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->type must be TCP or SSL", nc->type == ESP_NETCONN_TYPE_TCP || nc->type == ESP_NETCONN_TYPE_SSL);
    ESP_ASSERT("nc->conn must be active", esp_conn_is_active(nc->conn));

    /*
     * In case we have data in write buffer,
     * flush them out to network
     */
    if (nc->buff.buff != NULL) {                /* Check remaining data */
        if (nc->buff.ptr > 0) {                 /* Do we have data in current buffer? */
            esp_conn_send(nc->conn, nc->buff.buff, nc->buff.ptr, NULL, 1);  /* Send data */
        }
        esp_mem_free_s((void **)&nc->buff.buff);
    }
    return espOK;
}

/**
 * \brief           Send data on \e UDP connection to default IP and port
 * \param[in]       nc: Netconn handle used to send
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_send(esp_netconn_p nc, const void* data, size_t btw) {
    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->type must be UDP", nc->type == ESP_NETCONN_TYPE_UDP);
    ESP_ASSERT("nc->conn must be active", esp_conn_is_active(nc->conn));

    return esp_conn_send(nc->conn, data, btw, NULL, 1);
}

/**
 * \brief           Send data on \e UDP connection to specific IP and port
 * \note            Use this function in case of UDP type netconn
 * \param[in]       nc: Netconn handle used to send
 * \param[in]       ip: Pointer to IP address
 * \param[in]       port: Port number used to send data
 * \param[in]       data: Pointer to data to write
 * \param[in]       btw: Number of bytes to write
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_sendto(esp_netconn_p nc, const esp_ip_t* ip, esp_port_t port, const void* data, size_t btw) {
    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->type must be UDP", nc->type == ESP_NETCONN_TYPE_UDP);
    ESP_ASSERT("nc->conn must be active", esp_conn_is_active(nc->conn));

    return esp_conn_sendto(nc->conn, ip, port, data, btw, NULL, 1);
}

/**
 * \brief           Receive data from connection
 * \param[in]       nc: Netconn handle used to receive from
 * \param[in]       pbuf: Pointer to pointer to save new receive buffer to.
 *                     When function returns, user must check for valid pbuf value `pbuf != NULL`
 * \return          \ref espOK when new data ready
 * \return          \ref espCLOSED when connection closed by remote side
 * \return          \ref espTIMEOUT when receive timeout occurs
 * \return          Any other member of \ref espr_t otherwise
 */
espr_t
esp_netconn_receive(esp_netconn_p nc, esp_pbuf_p* pbuf) {
    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("pbuf != NULL", pbuf != NULL);

    *pbuf = NULL;
#if ESP_CFG_NETCONN_RECEIVE_TIMEOUT
    /*
     * Wait for new received data for up to specific timeout
     * or throw error for timeout notification
     */
    if (esp_sys_mbox_get(&nc->mbox_receive, (void **)pbuf, nc->rcv_timeout) == ESP_SYS_TIMEOUT) {
        return espTIMEOUT;
    }
#else /* ESP_CFG_NETCONN_RECEIVE_TIMEOUT */
    /* Forever wait for new receive packet */
    esp_sys_mbox_get(&nc->mbox_receive, (void **)pbuf, 0);
#endif /* !ESP_CFG_NETCONN_RECEIVE_TIMEOUT */

    esp_core_lock();
    if (nc->mbox_receive_entries > 0) {
        nc->mbox_receive_entries--;
    }
    esp_core_unlock();

    /* Check if connection closed */
    if ((uint8_t *)(*pbuf) == (uint8_t *)&recv_closed) {
        *pbuf = NULL;                           /* Reset pbuf */
        return espCLOSED;
    }
#if ESP_CFG_CONN_MANUAL_TCP_RECEIVE
    else {
        esp_core_lock();
        nc->conn->status.f.receive_blocked = 0; /* Resume reading more data */
        esp_conn_recved(nc->conn, *pbuf);       /* Notify stack about received data */
        esp_core_unlock();
    }
#endif /* ESP_CFG_CONN_MANUAL_TCP_RECEIVE */
    return espOK;                               /* We have data available */
}

/**
 * \brief           Close a netconn connection
 * \param[in]       nc: Netconn handle to close
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_netconn_close(esp_netconn_p nc) {
    esp_conn_p conn;

    ESP_ASSERT("nc != NULL", nc != NULL);
    ESP_ASSERT("nc->conn != NULL", nc->conn != NULL);
    ESP_ASSERT("nc->conn must be active", esp_conn_is_active(nc->conn));

    esp_netconn_flush(nc);                      /* Flush data and ignore result */
    conn = nc->conn;
    nc->conn = NULL;

    esp_conn_set_arg(conn, NULL);               /* Reset argument */
    esp_conn_close(conn, 1);                    /* Close the connection */
    flush_mboxes(nc, 1);                        /* Flush message queues */
    return espOK;
}

/**
 * \brief           Get connection number used for netconn
 * \param[in]       nc: Netconn handle
 * \return          `-1` on failure, connection number between `0` and \ref ESP_CFG_MAX_CONNS otherwise
 */
int8_t
esp_netconn_getconnnum(esp_netconn_p nc) {
    if (nc != NULL && nc->conn != NULL) {
        return esp_conn_getnum(nc->conn);
    }
    return -1;
}

#if ESP_CFG_NETCONN_RECEIVE_TIMEOUT || __DOXYGEN__

/**
 * \brief           Set timeout value for receiving data.
 *
 * When enabled, \ref esp_netconn_receive will only block for up to
 * \e timeout value and will return if no new data within this time
 *
 * \param[in]       nc: Netconn handle
 * \param[in]       timeout: Timeout in units of milliseconds.
 *                  Set to `0` to disable timeout for \ref esp_netconn_receive function
 */
void
esp_netconn_set_receive_timeout(esp_netconn_p nc, uint32_t timeout) {
    nc->rcv_timeout = timeout;
}

/**
 * \brief           Get netconn receive timeout value
 * \param[in]       nc: Netconn handle
 * \return          Timeout in units of milliseconds.
 *                  If value is `0`, timeout is disabled (wait forever)
 */
uint32_t
esp_netconn_get_receive_timeout(esp_netconn_p nc) {
    return nc->rcv_timeout;                     /* Return receive timeout */
}

#endif /* ESP_CFG_NETCONN_RECEIVE_TIMEOUT || __DOXYGEN__ */

#endif /* ESP_CFG_NETCONN || __DOXYGEN__ */
