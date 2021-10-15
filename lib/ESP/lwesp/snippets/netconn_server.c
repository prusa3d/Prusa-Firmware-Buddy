/*
 * Netconn server example is based on single "user" thread
 * which listens for new connections and accepts them.
 *
 * When a new client is accepted by server,
 * separate thread for client is created where 
 * data is read, processed and send back to user
 */
#include "netconn_server.h"
#include "esp/esp.h"

static void netconn_server_processing_thread(void* const arg);

/**
 * \brief           Main page response file
 */
static const uint8_t
resp_data_mainpage_top[] = ""
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
resp_data_mainpage_bottom[] = ""
"       </b></p>"
"   </body>"
"</html>";

/**
 * \brief           Style file response
 */
static const uint8_t
resp_data_style[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/css\r\n"
"\r\n"
"body { color: red; font-family: Tahoma, Arial; };";

/**
 * \brief           404 error response
 */
static const uint8_t
resp_error_404[] = ""
"HTTP/1.1 404 Not Found\r\n"
"\r\n"
"Error 404";

/**
 * \brief           Netconn server thread implementation
 * \param[in]       arg: User argument
 */
void
netconn_server_thread(void const* arg) {
    espr_t res;
    esp_netconn_p server, client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    if (server != NULL) {
        printf("Server netconn created\r\n");

        /* Bind network connection to port 80 */
        res = esp_netconn_bind(server, 80);
        if (res == espOK) {
            printf("Server netconn listens on port 80\r\n");
            /*
             * Start listening for incoming connections
             * on previously binded port
             */
            res = esp_netconn_listen(server);
            
            while (1) {
                /*
                 * Wait and accept new client connection
                 * 
                 * Function will block thread until 
                 * new client is connected to server
                 */
                res = esp_netconn_accept(server, &client);
                if (res == espOK) {
                    printf("Netconn new client connected. Starting new thread...\r\n");
                    /*
                     * Start new thread for this request.
                     *
                     * Read and write back data to user in separated thread
                     * to allow processing of multiple requests at the same time
                     */
                    if (esp_sys_thread_create(NULL, "client", (esp_sys_thread_fn)netconn_server_processing_thread, client, 512, ESP_SYS_THREAD_PRIO)) {
                        printf("Netconn client thread created\r\n");
                    } else {
                        printf("Netconn client thread creation failed!\r\n");

                        /* Force close & delete */
                        esp_netconn_close(client);
                        esp_netconn_delete(client);
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
    
    esp_netconn_delete(server);                 /* Delete netconn structure */
    esp_sys_thread_terminate(NULL);             /* Terminate current thread */
}

/**
 * \brief           Thread to process single active connection
 * \param[in]       arg: Thread argument
 */
static void
netconn_server_processing_thread(void* const arg) {
    esp_netconn_p client;
    esp_pbuf_p pbuf, p = NULL;
    espr_t res;
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
        res = esp_netconn_receive(client, &pbuf);

        if (res == espOK) {
            printf("Netconn data received, %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
            /* Check reception of all header bytes */
            if (p == NULL) {
                p = pbuf;                       /* Set as first buffer */
            } else {
                esp_pbuf_cat(p, pbuf);          /* Concatenate buffers together */
            }
            if (esp_pbuf_strfind(pbuf, "\r\n\r\n", 0) != ESP_SIZET_MAX) {
                if (esp_pbuf_strfind(pbuf, "GET / ", 0) != ESP_SIZET_MAX) {
                    uint32_t now;
                    printf("Main page request\r\n");
                    now = esp_sys_now();        /* Get current time */
                    sprintf(strt, "%u ms; %d s", (unsigned)now, (unsigned)(now / 1000));
                    esp_netconn_write(client, resp_data_mainpage_top, sizeof(resp_data_mainpage_top) - 1);
                    esp_netconn_write(client, strt, strlen(strt));
                    esp_netconn_write(client, resp_data_mainpage_bottom, sizeof(resp_data_mainpage_bottom) - 1);
                } else if (esp_pbuf_strfind(pbuf, "GET /style.css ", 0) != ESP_SIZET_MAX) {
                    printf("Style page request\r\n");
                    esp_netconn_write(client, resp_data_style, sizeof(resp_data_style) - 1);
                } else {
                    printf("404 error not found\r\n");
                    esp_netconn_write(client, resp_error_404, sizeof(resp_error_404) - 1);
                }
                esp_netconn_close(client);      /* Close netconn connection */
                esp_pbuf_free(p);               /* Do not forget to free memory after usage! */
                p = NULL;
                break;
            }
        }
    } while (res == espOK);

    if (p != NULL) {                            /* Free received data */
        esp_pbuf_free(p);
        p = NULL;
    }
    esp_netconn_delete(client);                 /* Destroy client memory */
    esp_sys_thread_terminate(NULL);             /* Terminate this thread */
}
