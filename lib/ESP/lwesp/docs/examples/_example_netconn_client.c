/* Set server parameters */
#define NETCONN_HOST            "example.com"
#define NETCONN_PORT            80

/* Request data for netconn */
const uint8_t netconn_req_data[] = ""
    "GET / HTTP/1.1\r\n"
    "Host: " NETCONN_HOST "\r\n"
    "Connection: close\r\n"
    "\r\n";

/*
 * \brief           Client netconn thread
 */
void
client_thread(void const* arg) {
    espr_t res;
    esp_pbuf_p pbuf;
    esp_netconn_p client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    client = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    
    if (client != NULL) {                       /* Process only if client memory is allocated */
        
        /*
         * Connect to external server as client
         * with custom NETCONN_CONN_HOST and CONN_PORT values
         *
         * Function will block thread until we are successfully connected to server
         */
        res = esp_netconn_connect(client, NETCONN_HOST, NETCONN_PORT);
        if (res == espOK) {                     /* Are we successfully connected? */
            printf("Connected to server " NETCONN_HOST "\r\n");
            /*
             * First write data to TCP output buffer
             * If data are written, manually flush it to
             * force sending the data
             */
            res = esp_netconn_write(client, requestData, sizeof(requestData) - 1);/* Send data to server */
            if (res == espOK) {
                res = esp_netconn_flush(client);    /* Flush data to output */
            }
            if (res == espOK) {                 /* Were data sent? */
                printf("Data were successfully sent to server\r\n");
                
                /*
                 * Since we sent HTTP request, 
                 * we are expecting some data from server
                 * or at least forced connection close from remote side
                 */
                do {
                    
                    /*
                     * Receive single packet of data
                     * 
                     * Function will block thread until new packet 
                     * is ready to be read from remote side
                     *
                     * After function returns, check status as it
                     * may happen that closed status was returned
                     */
                    res = esp_netconn_receive(client, &pbuf);
                    if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
                        printf("Connection closed by remote side...Stopping\r\n");
                        break;
                    }
                    
                    /*
                     * At this point read and manipulate
                     * with received buffer and check if you expect more data
                     *
                     * After you are done using it, it is important
                     * you free the memory otherwise memory leaks will appear
                     */
                    printf("Received new data packet of %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
                    esp_pbuf_free(pbuf);        /* Free the memory after usage */
                } while (1);
            } else {
                printf("Error writing data to remote host!\r\n");
            }
            
            /*
             * Check if connection was closed by remote server
             * and in case it wasn't, close it by yourself
             */
            if (res != espCLOSED) {
                esp_netconn_close(client);
            }
        } else {
            printf("Cannot connect to external server!\r\n");
        }
    }
    
    /* Last step is to delete connection object from memory */
    esp_netconn_delete(client);
}