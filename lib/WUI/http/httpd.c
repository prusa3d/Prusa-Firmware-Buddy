/*
 * cyclone_httpd.c
 *
 *  Created on: May 14, 2021
 *      Author: joshy
 */

#include "http/http_server.h"
#include "error.h"
#include "os_port.h"
#include "cyclone_debug.h"

//Application configuration
#define APP_HTTP_MAX_CONNECTIONS 1

HttpServerSettings httpServerSettings;
HttpServerContext httpServerContext;
HttpConnection httpConnections[APP_HTTP_MAX_CONNECTIONS];

//Forward declaration of functions
error_t httpServerCgiCallback(HttpConnection *connection,
    const char_t *param);

error_t httpServerRequestCallback(HttpConnection *connection,
    const char_t *uri);

error_t httpServerUriNotFoundCallback(HttpConnection *connection,
    const char_t *uri);

void init_cyclone_httpd() {
    error_t error;
    //Get default settings
    httpServerGetDefaultSettings(&httpServerSettings);
    //Bind HTTP server to the desired interface
    //httpServerSettings.interface = &netInterface[0];
    //Listen to port 80
    httpServerSettings.port = HTTP_PORT;
    //Client connections
    httpServerSettings.maxConnections = APP_HTTP_MAX_CONNECTIONS;
    httpServerSettings.connections = httpConnections;
    //Specify the server's root directory
    strcpy(httpServerSettings.rootDirectory, "/www/");
    //Set default home page
    strcpy(httpServerSettings.defaultDocument, "index.shtm");
    //Callback functions
    httpServerSettings.cgiCallback = httpServerCgiCallback;
    httpServerSettings.requestCallback = httpServerRequestCallback;
    httpServerSettings.uriNotFoundCallback = httpServerUriNotFoundCallback;

    //HTTP server initialization
    error = httpServerInit(&httpServerContext, &httpServerSettings);
    //Failed to initialize HTTP server?
    if (error) {
        //Debug message
        TRACE_ERROR("Failed to initialize HTTP server!\r\n");
    }

    //Start HTTP server
    error = httpServerStart(&httpServerContext);
    //Failed to start HTTP server?
    if (error) {
        //Debug message
        TRACE_ERROR("Failed to start HTTP server!\r\n");
    }
}

void http_server_init(void) {
    init_cyclone_httpd();
}

/**
 * @brief CGI callback function
 * @param[in] connection Handle referencing a client connection
 * @param[in] param NULL-terminated string that contains the CGI parameter
 * @return Error code
 **/

error_t httpServerCgiCallback(HttpConnection *connection,
    const char_t *param) {
    //Not implemented
    return ERROR_NOT_FOUND;
}

/**
 * @brief HTTP request callback
 * @param[in] connection Handle referencing a client connection
 * @param[in] uri NULL-terminated string containing the path to the requested resource
 * @return Error code
 **/

error_t httpServerRequestCallback(HttpConnection *connection,
    const char_t *uri) {
    //Not implemented
    return ERROR_NOT_FOUND;
}

/**
 * @brief URI not found callback
 * @param[in] connection Handle referencing a client connection
 * @param[in] uri NULL-terminated string containing the path to the requested resource
 * @return Error code
 **/

error_t httpServerUriNotFoundCallback(HttpConnection *connection,
    const char_t *uri) {
    //Not implemented
    return ERROR_NOT_FOUND;
}
