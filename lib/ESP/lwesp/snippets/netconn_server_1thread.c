/*
 * Netconn server example is based on single thread
 * and it listens for single client only on port 23
 */
#include "netconn_server_1thread.h"
#include "esp/esp.h"

/**
 * \brief           Basic thread for netconn server to test connections
 * \param[in]       arg: User argument
 */
void
netconn_server_1thread_thread(void* arg) {
    espr_t res;
    esp_netconn_p server, client;
    esp_pbuf_p p;

    /* Create netconn for server */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    if (server == NULL) {
        printf("Cannot create server netconn!\r\n");
    }

    /* Bind it to port 23 */
    res = esp_netconn_bind(server, 23);
    if (res != espOK) {
        printf("Cannot bind server\r\n");
        goto out;
    }

    /* Start listening for incoming connections with maximal 1 client */
    res = esp_netconn_listen_with_max_conn(server, 1);
    if (res != espOK) {
        goto out;
    }

    /* Unlimited loop */
    while (1) {
        /* Accept new client */
        res = esp_netconn_accept(server, &client);
        if (res != espOK) {
            break;
        }
        printf("New client accepted!\r\n");
        while (1) {
            /* Receive data */
            res = esp_netconn_receive(client, &p);
            if (res == espOK) {
                printf("Data received!\r\n");
                esp_pbuf_free(p);
            } else {
                printf("Netconn receive returned: %d\r\n", (int)res);
                if (res == espCLOSED) {
                    printf("Connection closed by client\r\n");
                    break;
                }
            }
        }
        /* Delete client */
        if (client != NULL) {
            esp_netconn_delete(client);
            client = NULL;
        }
    }
    /* Delete client */
    if (client != NULL) {
        esp_netconn_delete(client);
        client = NULL;
    }

out:
    printf("Terminating netconn thread!\r\n");
    if (server != NULL) {
        esp_netconn_delete(server);
    }
    esp_sys_thread_terminate(NULL);
}
