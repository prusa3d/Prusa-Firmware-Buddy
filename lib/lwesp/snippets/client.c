#include "client.h"
#include "lwesp/lwesp.h"

/* Host parameter */
#define CONN_HOST           "example.com"
#define CONN_PORT           80

static lwespr_t   conn_callback_func(lwesp_evt_t* evt);

/**
 * \brief           Request data for connection
 */
static const
uint8_t req_data[] = ""
                     "GET / HTTP/1.1\r\n"
                     "Host: " CONN_HOST "\r\n"
                     "Connection: close\r\n"
                     "\r\n";

/**
 * \brief           Start a new connection(s) as client
 */
void
client_connect(void) {
    lwespr_t res;

    /* Start a new connection as client in non-blocking mode */
    if ((res = lwesp_conn_start(NULL, LWESP_CONN_TYPE_TCP, "example.com", 80, NULL, conn_callback_func, 0)) == lwespOK) {
        printf("Connection to " CONN_HOST " started...\r\n");
    } else {
        printf("Cannot start connection to " CONN_HOST "!\r\n");
    }

    /* Start 2 more */
    lwesp_conn_start(NULL, LWESP_CONN_TYPE_TCP, CONN_HOST, CONN_PORT, NULL, conn_callback_func, 0);

    /*
     * An example of connection which should fail in connecting.
     * When this is the case, \ref LWESP_EVT_CONN_ERROR event should be triggered
     * in callback function processing
     */
    lwesp_conn_start(NULL, LWESP_CONN_TYPE_TCP, CONN_HOST, 10, NULL, conn_callback_func, 0);
}

/**
 * \brief           Event callback function for connection-only
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
conn_callback_func(lwesp_evt_t* evt) {
    lwesp_conn_p conn;
    lwespr_t res;
    uint8_t conn_num;

    conn = lwesp_conn_get_from_evt(evt);          /* Get connection handle from event */
    if (conn == NULL) {
        return lwespERR;
    }
    conn_num = lwesp_conn_getnum(conn);           /* Get connection number for identification */
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_CONN_ACTIVE: {             /* Connection just active */
            printf("Connection %d active!\r\n", (int)conn_num);
            res = lwesp_conn_send(conn, req_data, sizeof(req_data) - 1, NULL, 0); /* Start sending data in non-blocking mode */
            if (res == lwespOK) {
                printf("Sending request data to server...\r\n");
            } else {
                printf("Cannot send request data to server. Closing connection manually...\r\n");
                lwesp_conn_close(conn, 0);        /* Close the connection */
            }
            break;
        }
        case LWESP_EVT_CONN_CLOSE: {              /* Connection closed */
            if (lwesp_evt_conn_close_is_forced(evt)) {
                printf("Connection %d closed by client!\r\n", (int)conn_num);
            } else {
                printf("Connection %d closed by remote side!\r\n", (int)conn_num);
            }
            break;
        }
        case LWESP_EVT_CONN_SEND: {               /* Data send event */
            lwespr_t res = lwesp_evt_conn_send_get_result(evt);
            if (res == lwespOK) {
                printf("Data sent successfully on connection %d...waiting to receive data from remote side...\r\n", (int)conn_num);
            } else {
                printf("Error while sending data on connection %d!\r\n", (int)conn_num);
            }
            break;
        }
        case LWESP_EVT_CONN_RECV: {               /* Data received from remote side */
            lwesp_pbuf_p pbuf = lwesp_evt_conn_recv_get_buff(evt);
            lwesp_conn_recved(conn, pbuf);        /* Notify stack about received pbuf */
            printf("Received %d bytes on connection %d..\r\n", (int)lwesp_pbuf_length(pbuf, 1), (int)conn_num);
            break;
        }
        case LWESP_EVT_CONN_ERROR: {              /* Error connecting to server */
            const char* host = lwesp_evt_conn_error_get_host(evt);
            lwesp_port_t port = lwesp_evt_conn_error_get_port(evt);
            printf("Error connecting to %s:%d\r\n", host, (int)port);
            break;
        }
        default:
            break;
    }
    return lwespOK;
}
