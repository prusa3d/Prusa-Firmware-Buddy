/*
 * Netconn server example is based on single "user" thread
 * which listens for new connections and accepts them.
 *
 * When a new client is accepted by server,
 * separate thread for client is created where
 * data is read, processed and send back to user
 */
#include "netconn_server.h"
#include "lwesp/lwesp.h"

static void netconn_server_processing_thread(void* const arg);

/**
 * \brief           Main page response file
 */
static const uint8_t
rlwesp_data_mainpage_top[] = ""
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "\r\n"
                           "<html>"
                           "   <head>"
                           "       <link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\" />"
                           "       <meta http-equiv=\"refresh\" content=\"1\" />"
                           "   </head>"
                           "   <body>"
                           "       <p>Netconn driven website!</p>"
                           "       <p>Total system up time: <b>";

/**
 * \brief           Bottom part of main page
 */
static const uint8_t
rlwesp_data_mainpage_bottom[] = ""
                              "       </b></p>"
                              "   </body>"
                              "</html>";

/**
 * \brief           Style file response
 */
static const uint8_t
rlwesp_data_style[] = ""
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/css\r\n"
                    "\r\n"
                    "body { color: red; font-family: Tahoma, Arial; };";

/**
 * \brief           404 error response
 */
static const uint8_t
rlwesp_error_404[] = ""
                   "HTTP/1.1 404 Not Found\r\n"
                   "\r\n"
                   "Error 404";

/**
 * \brief           Netconn server thread implementation
 * \param[in]       arg: User argument
 */
void
netconn_server_thread(void const* arg) {
    lwespr_t res;
    lwesp_netconn_p server, client;

    /*
     * First create a new instance of netconn
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = lwesp_netconn_new(LWESP_NETCONN_TYPE_TCP);
    if (server != NULL) {
        printf("Server netconn created\r\n");

        /* Bind network connection to port 80 */
        res = lwesp_netconn_bind(server, 80);
        if (res == lwespOK) {
            printf("Server netconn listens on port 80\r\n");
            /*
             * Start listening for incoming connections
             * on previously binded port
             */
            res = lwesp_netconn_listen(server);

            while (1) {
                /*
                 * Wait and accept new client connection
                 *
                 * Function will block thread until
                 * new client is connected to server
                 */
                res = lwesp_netconn_accept(server, &client);
                if (res == lwespOK) {
                    printf("Netconn new client connected. Starting new thread...\r\n");
                    /*
                     * Start new thread for this request.
                     *
                     * Read and write back data to user in separated thread
                     * to allow processing of multiple requests at the same time
                     */
                    if (lwesp_sys_thread_create(NULL, "client", (lwesp_sys_thread_fn)netconn_server_processing_thread, client, 512, LWESP_SYS_THREAD_PRIO)) {
                        printf("Netconn client thread created\r\n");
                    } else {
                        printf("Netconn client thread creation failed!\r\n");

                        /* Force close & delete */
                        lwesp_netconn_close(client);
                        lwesp_netconn_delete(client);
                    }
                } else {
                    printf("Netconn connection accept error!\r\n");
                    break;
                }
            }
        } else {
            printf("Netconn server cannot bind to port\r\n");
        }
    } else {
        printf("Cannot create server netconn\r\n");
    }

    lwesp_netconn_delete(server);                 /* Delete netconn structure */
    lwesp_sys_thread_terminate(NULL);             /* Terminate current thread */
}

/**
 * \brief           Thread to process single active connection
 * \param[in]       arg: Thread argument
 */
static void
netconn_server_processing_thread(void* const arg) {
    lwesp_netconn_p client;
    lwesp_pbuf_p pbuf, p = NULL;
    lwespr_t res;
    char strt[20];

    client = arg;                               /* Client handle is passed to argument */

    printf("A new connection accepted!\r\n");   /* Print simple message */

    do {
        /*
         * Client was accepted, we are now
         * expecting client will send to us some data
         *
         * Wait for data and block thread for that time
         */
        res = lwesp_netconn_receive(client, &pbuf);

        if (res == lwespOK) {
            printf("Netconn data received, %d bytes\r\n", (int)lwesp_pbuf_length(pbuf, 1));
            /* Check reception of all header bytes */
            if (p == NULL) {
                p = pbuf;                       /* Set as first buffer */
            } else {
                lwesp_pbuf_cat(p, pbuf);          /* Concatenate buffers together */
            }
            if (lwesp_pbuf_strfind(pbuf, "\r\n\r\n", 0) != LWESP_SIZET_MAX) {
                if (lwesp_pbuf_strfind(pbuf, "GET / ", 0) != LWESP_SIZET_MAX) {
                    uint32_t now;
                    printf("Main page request\r\n");
                    now = lwesp_sys_now();        /* Get current time */
                    sprintf(strt, "%u ms; %d s", (unsigned)now, (unsigned)(now / 1000));
                    lwesp_netconn_write(client, rlwesp_data_mainpage_top, sizeof(rlwesp_data_mainpage_top) - 1);
                    lwesp_netconn_write(client, strt, strlen(strt));
                    lwesp_netconn_write(client, rlwesp_data_mainpage_bottom, sizeof(rlwesp_data_mainpage_bottom) - 1);
                } else if (lwesp_pbuf_strfind(pbuf, "GET /style.css ", 0) != LWESP_SIZET_MAX) {
                    printf("Style page request\r\n");
                    lwesp_netconn_write(client, rlwesp_data_style, sizeof(rlwesp_data_style) - 1);
                } else {
                    printf("404 error not found\r\n");
                    lwesp_netconn_write(client, rlwesp_error_404, sizeof(rlwesp_error_404) - 1);
                }
                lwesp_netconn_close(client);      /* Close netconn connection */
                lwesp_pbuf_free(p);               /* Do not forget to free memory after usage! */
                p = NULL;
                break;
            }
        }
    } while (res == lwespOK);

    if (p != NULL) {                            /* Free received data */
        lwesp_pbuf_free(p);
        p = NULL;
    }
    lwesp_netconn_delete(client);                 /* Destroy client memory */
    lwesp_sys_thread_terminate(NULL);             /* Terminate this thread */
}
