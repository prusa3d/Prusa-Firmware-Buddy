/* Request data are sent to server once we are connected */
uint8_t req_data[] = ""
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Connection: close\r\n"
    "\r\n";

/*
 * \brief           Connection callback function
 *                  Called on several connection events, such as connected, closed, data received, data sent, ...
 * \param[in]       evt: ESP callback event
 */
static espr_t
conn_evt(esp_evt_t* evt) {
    esp_conn_p conn = esp_conn_get_from_evt(evt);   /* Get connection from current event */
    if (conn == NULL) {
        return espERR;                          /* Return error at this point as this should never happen! */ 
    }
    
    switch (esp_evt_get_type(evt)) {
        /* A new connection just became active */
        case ESP_EVT_CONN_ACTIVE: {
            printf("Connection active!\r\n");
            
            /*
             * After we are connected,
             * send the HTTP request string in non-blocking way
             */
            esp_conn_send(conn, req_data, sizeof(req_data) - 1, NULL, 0);
            break;
        }
        
        /* Connection closed event */
        case ESP_EVT_CONN_CLOSED: {
            printf("Connection closed!\r\n");
            if (evt->evt.conn_active_closed.forced) {   /* Was it forced by user? */
                printf("Connection closed by user\r\n");
            } else {
                printf("Connection closed by remote host\r\n");
            }
        }
        
        /* Data received on connection */
        case ESP_EVT_CONN_RECV: {
            esp_pbuf_p pbuf = cb->evt.conn_data_recv.buff;   /* Get data buffer */
            
            /*
             * Connection data buffer is automatically
             * freed when you return from function
             * If you still want to hold it,
             * then either chain it using esp_pbuf_chain
             * or reference it using esp_pbuf_ref functions.
             */
            
            printf("Connection data received!\r\n");
            if (pbuf != NULL) {
                size_t len;
                /*
                 * You should not call esp_pbuf_free on this variable unless
                 * you used esp_pbuf_ref before to increase reference
                 */
                len = esp_pbuf_length(pbuf, 1); /* Get total length of buffer */
                printf("Length of data: %d bytes\r\n", (int)len);
            }
			
			esp_conn_recved(conn, pbuf);        /* Notify stack about received data */ 
        }
        default:
            break;
    }
    return espOK;
}

/*
 * \brief           Thread function
 */
static void
thread_or_main_func(void) {
    /*
     * Start the connection in non-blocking way and set the
     * function argument to NULL and callback function to conn_evt
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.com", 80, NULL, conn_evt, 0);
    
    // Do other tasks...
}
