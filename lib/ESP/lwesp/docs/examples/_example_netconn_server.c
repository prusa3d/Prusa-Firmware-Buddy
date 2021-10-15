/*
 * \brief           Server netconn thread
 */
void
server_thread(void const* arg) {
    espr_t res;
    esp_pbuf_p pbuf;
    esp_netconn_p server, client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    
    if (server != NULL) {
        
        /*
         * Bind network connection to port 80
         */
        res = esp_netconn_bind(server, 80);
        if (res == espOK) {
            
            /*
             * Start listening for incoming connections
             * on previously binded port
             */
            res = esp_netconn_listen(server);
            
            /*
             * Loop forever
             */
            while (1) {
                
                /*
                 * Wait and accept new client connection
                 * 
                 * Function will block thread until 
                 * new client is connected to server.
                 *
                 * From performance point of view,
                 * this allows you zero overhead
                 */
                res = esp_netconn_accept(server, &client);
                if (res == espOK) {
                    printf("A new connection accepted!\r\n");
                    
                    do {
                        /*
                         * Since we accepted a client, we
                         * are expecting that client will send to us some data
                         *
                         * Wait for data and block thread for that time
                         */
                        res = esp_netconn_receive(client, &pbuf);
                        
                        /*
                         * It may happen that connection 
                         * is closed from client side
                         */
                        if (res == espCLOSED) {
                            break;
                        }
                        
                        /*
                         * Process buffer and decide if you expect more data,
                         * such as CRLFCRLF sequence in HTTP server.
                         *
                         * When you are ready to continue, break this for loop
                         */
                        esp_pbuf_free(pbuf);    /* Do not forget to free memory after usage! */
                        
                        /*
                        if (everything_received(client)) {
                            break;
                        }
                        */
                    } while (res == espOK);
                    
                    /*
                     * If everything is still ready,
                     * it is time to send response to client
                     */
                    if (res == espOK) {
                        
                        /*
                         * Send data back to client
                         * and wait to be sent successfully
                         *
                         * Data are written to TCP send buffer
                         * to allow user to call write function multiple 
                         * times and to speed up communication
                         */
                        res = esp_netconn_write(client, your_data, your_data_len);
                        
                        /*
                         * When done sending everything, close client connection
                         */
                        esp_netconn_close(client);
                    }
                    
                    /*
                     * Last step is to free memory 
                     * for netconn and to go to ready state for next connection
                     */
                    esp_netconn_delete(client);
                }
            }
        }
    }
}