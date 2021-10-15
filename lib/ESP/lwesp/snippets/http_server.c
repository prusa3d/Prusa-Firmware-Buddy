#include "http_server.h"
#include "esp/esp.h"
#include "esp/apps/esp_http_server.h"
#include "esp/apps/esp_http_server_fs.h"

static size_t   http_ssi_cb(http_state_t* hs, const char* tag_name, size_t tag_len);
#if HTTP_SUPPORT_POST
static espr_t   http_post_start_cb(http_state_t* hs, const char* uri, uint32_t content_len);
static espr_t   http_post_data_cb(http_state_t* hs, esp_pbuf_p pbuf);
static espr_t   http_post_end_cb(http_state_t* hs);
#endif /* HTTP_SUPPORT_POST */

static char *   led_cgi_handler(http_param_t* params, size_t params_len);
static char *   usart_cgi_handler(http_param_t* params, size_t params_len);

/**
 * \brief           List of CGI handlers
 */
const http_cgi_t
cgi_handlers[] = {
    { "/led.cgi", led_cgi_handler },
    { "/usart.cgi", usart_cgi_handler },
};

/**
 * \brief           HTTP init structure
 */
const http_init_t
http_init = {
#if HTTP_SUPPORT_POST
    .post_start_fn = http_post_start_cb,
    .post_data_fn = http_post_data_cb,
    .post_end_fn = http_post_end_cb,
#endif /* HTTP_SUPPORT_POST */
    .cgi = cgi_handlers,
    .cgi_count = ESP_ARRAYSIZE(cgi_handlers),
    .ssi_fn = http_ssi_cb,
    
    /*
     * Use native WIN32 file system API
     */
#if WIN32
    .fs_open = http_fs_open,
    .fs_read = http_fs_read,
    .fs_close = http_fs_close,
#endif /* WIN32 */
};

/**
 * \brief           Start HTTP server on port 80
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
http_server_start(void) {
    espr_t res;
    printf("Starting HTTP server on port 80...\r\n");
    if ((res = esp_http_server_init(&http_init, 80)) == espOK) {
        printf("HTTP server ready!\r\n");
    } else {
        printf("Cannot start HTTP server\r\n");
    }
    return res;
}

#if HTTP_SUPPORT_POST

/**
 * \brief           Callback function indicating post request method started
 * \param[in]       hs: HTTP state
 * \param[in]       uri: NULL-terminated uri string for POST request
 * \param[in]       content_len: Total content length received by "Content-Length" header
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
http_post_start_cb(http_state_t* hs, const char* uri, uint32_t content_len) {
    printf("POST started with %d length on URI: %s\r\n", (int)content_len, uri);
    return espOK;
}

/**
 * \brief           Callback function indicating post request data received
 * \param[in]       hs: HTTP state
 * \param[in]       pbuf: New chunk of received data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
http_post_data_cb(http_state_t* hs, esp_pbuf_p pbuf) {
    printf("POST data received: %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
    return espOK;
}

/**
 * \brief           Callback function indicating post request finished
 * \param[in]       hs: HTTP state
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
http_post_end_cb(http_state_t* hs) {
    printf("POST finished!\r\n");
    return espOK;
}

#endif /* HTTP_SUPPORT_POST */

/**
 * \brief           Global SSI callback function
 *          
 *                  Called in case SSI tag was found and ready to be replaced by custom data
 *
 * \param[in]       hs: HTTP state
 * \param[in]       tag_name: Name of tag
 * \param[in]       tag_len: Length of tag
 * \return          1 if more data to write on this tag or 0 if everything written for specific tag
 */
static size_t
http_ssi_cb(http_state_t* hs, const char* tag_name, size_t tag_len) {
    static char ssi_buffer[32];
    static uint32_t cnt;

    cnt++;
    
    ESP_UNUSED(ssi_buffer);
    if (!strncmp(tag_name, "title", tag_len)) {
        esp_http_server_write_string(hs, "ESP8266 SSI TITLE");
        return cnt % 3;
    } else if (!strncmp(tag_name, "led_status", tag_len)) {
        esp_http_server_write_string(hs, "Led is on");
    } else if (!strncmp(tag_name, "wifi_list", tag_len)) {
        size_t i = 0;

        ESP_UNUSED(i);
        esp_http_server_write_string(hs, "<table class=\"table\">");
        esp_http_server_write_string(hs, "<thead><tr><th>#</th><th>SSID</th><th>MAC</th><th>RSSI</th></tr></thead><tbody>");
        
        //for (i = 0; i < apf; i++) {
        //    esp_http_server_write_string(hs, "<tr><td>");
        //    sprintf(ssi_buffer, "%d", (int)i);
        //    esp_http_server_write_string(hs, ssi_buffer);
        //    esp_http_server_write_string(hs, "</td><td>");
        //    esp_http_server_write_string(hs, aps[i].ssid);
        //    esp_http_server_write_string(hs, "</td><td>");
        //    sprintf(ssi_buffer, "%02X:%02X:%02X:%02X:%02X:%02X", aps[i].mac[0], aps[i].mac[1], aps[i].mac[2], aps[i].mac[3], aps[i].mac[4], aps[i].mac[5]);
        //    esp_http_server_write_string(hs, ssi_buffer);
        //    esp_http_server_write_string(hs, "</td><td>");
        //    sprintf(ssi_buffer, "%d", (int)aps[i].rssi);
        //    esp_http_server_write_string(hs, ssi_buffer);
        //    esp_http_server_write_string(hs, "</td></tr>");
        //}
        esp_http_server_write_string(hs, "</tbody></table>");
    }
    return 1;
}

/**
 * \brief           CGI handler function when user connects to "http://ip/led.cgi?param1=value1&param2=value2"
 * \param[in]       params: Pointer to list of parameters with URI
 * \param[in]       params_len: Number of parameters
 * \return          URI string to return to user
 */
char *
led_cgi_handler(http_param_t* params, size_t params_len) {
    printf("LED CGI HANDLER\r\n");
    while (params_len--) {
        printf("Param: name = %s, value = %s\r\n", params->name, params->value);
        params++;
    }
    return "/index.shtml";
}

/**
 * \brief           CGI handler function when user connects to "http://ip/usart.cgi?param1=value1&param2=value2"
 * \param[in]       params: Pointer to list of parameters with URI
 * \param[in]       params_len: Number of parameters
 * \return          URI string to return to user
 */
char *
usart_cgi_handler(http_param_t* params, size_t params_len) {
    printf("USART CGI HANDLER!\r\n");
    while (params_len--) {
        printf("Param: name = %s, value = %s\r\n", params->name, params->value);
        params++;
    }
    return "/index.html";
}
