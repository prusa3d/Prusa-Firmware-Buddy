/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author:       Joakim Myrland
 * website:      www.LDA.as
 * email:        joakim.myrland@LDA.as
 * project:      https://github.com/Lindem-Data-Acquisition-AS/iot_lib/
 *
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <file_list_defs.h>

#include "httpd.h"
#include "upload_state.h"
#include "lwip/def.h"

#define _dbg(...)

// FIXME: Make it _not global_ to allow multiple parallel uploads.
struct Uploader *uploader = NULL;

// List of accepted URI for POST requests
static uint8_t http_post_uri_file_index = 0;
static uint32_t http_post_content_len = 0;
#define HTTP_POST_URI_NUM 2
const char *endpoints[HTTP_POST_URI_NUM] = {
    "/api/files/sdcard",
    "/api/files/local"
};

static bool authenticate_post(struct HttpHandlers *handlers, const char *http_request, uint16_t http_request_len) {
    /*
     * Create a "fake" pbuf for reusing the authorize_request. We will not
     * allocate it the usual way and we will not free it. But the helper
     * functions to search it will work.
     */
    struct pbuf buf = {
        .payload = (char *)http_request, // FIXME: Dropping const is Bad (tm). It won't be modified in practice, but...
        .len = http_request_len,
        .tot_len = http_request_len,
    };

    return authorize_request(handlers, &buf);
    // No need to free anything related to the buf.
}

/** Called when a POST request has been received. The application can decide
 * whether to accept it or not.
 *
 * @param connection Unique connection identifier, valid until httpd_post_end
 *        is called.
 * @param uri The HTTP header URI receiving the POST request.
 * @param http_request The raw HTTP request (the first packet, normally).
 * @param http_request_len Size of 'http_request'.
 * @param content_len Content-Length from HTTP header.
 * @param response_uri Filename of response file, to be filled when denying the
 *        request
 * @param response_uri_len Size of the 'response_uri' buffer.
 * @param post_auto_wnd Set this to 0 to let the callback code handle window
 *        updates by calling 'httpd_post_data_recved' (to throttle rx speed)
 *        default is 1 (httpd handles window updates automatically)
 * @param content_type Content-Type string.
 * @return ERR_OK: Accept the POST request, data may be passed in
 *         another err_t: Deny the POST request, send back 'bad request'.
 */
err_t httpd_post_begin(void *connection,
    const char *uri,
    const char *http_request,
    u16_t http_request_len,
    int content_len,
    char *response_uri,
    u16_t response_uri_len,
    u8_t *post_auto_wnd) {

#define CONTENT_TYPE "Content-Type: multipart/form-data"

    if (uploader != NULL) {
        uri = "/error/503";
        goto invalid;
    }

    struct HttpHandlers *handlers = extract_http_handlers(connection);

    if (!authenticate_post(handlers, http_request, http_request_len)) {
        uri = "/error/401";
        goto invalid;
    }

    char *content_type_tag = lwip_strnstr(http_request, CONTENT_TYPE, http_request_len);
    if (content_type_tag == NULL) {
        uri = "/error/400";
        goto invalid;
    }

    // Check the URI given with the list
    for (uint8_t i = 0; i < HTTP_POST_URI_NUM; i++) {
        if (lwip_stricmp(uri, endpoints[i]) == 0) {

            http_post_uri_file_index = i;
            http_post_content_len = content_len;

#ifdef HTTPD_DEBUG
            _dbg("httpd_post_begin: Post Content: %s\n", http_request);
#endif
            /*
             * Get the boundary from the content-type
             * Then pass it to the parser
             */
            const char *boundary = find_boundary(http_request);
            if (boundary != NULL) {
                uploader = uploader_init(boundary, handlers);
                if (uploader == NULL) {
                    uri = "/error/503";
                    goto invalid;
                }
            } else {
                uri = "/error/400";
                goto invalid;
            }

            const uint16_t err = uploader_error(uploader);
            if (err != 0) {
                // Failed to initialize (no USB, allocation failed, ...?)
                uploader_finish(uploader);
                uploader = NULL;
                uri = handlers->code_lookup(handlers, err);
                goto invalid;
            }

            return ERR_OK;
        }
    }

invalid:
    snprintf(response_uri, response_uri_len, uri);
    //returns /404.html when response_uri is empty
    return ERR_VAL;

#undef API_KEY_TAG
#undef CONTENT_TYPE
}

/** Called for each pbuf of data that has been received for a POST.
 * ATTENTION: The application is responsible for freeing the pbufs passed in!
 *
 * @param connection Unique connection identifier.
 * @param p Received data.
 * @return ERR_OK: Data accepted.
 *         another err_t: Data denied, http_post_get_response_uri will be called.
 */
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {

    char *data;
    err_t ret_val = ERR_ARG;

    struct pbuf *current = p;

    while (connection != NULL && current != NULL && uploader != NULL) {
        data = current->payload;
        uploader_feed(uploader, data, current->len);
        ret_val = ERR_OK;
        /*
         * TODO: Currently, we let the thing going even if the uploader is in
         *       error state. It'll handle stuff internally OK. But we would
         *       eventually like to get an early error to the user if possible.
         *
         *       Unfortunately, the documentation talks about
         *       http_post_get_response_uri _which doesn't exist_ in the code
         *       base.
         */
        current = current->next;
    }

    if (p != NULL) {
        pbuf_free(p);
    }

    return ret_val;
}

/** Called when all data is received or when the connection is closed.
 * The application must return the filename/URI of a file to send in response
 * to this POST request. If the response_uri buffer is untouched, a 404
 * response is returned.
 *
 * @param connection Unique connection identifier.
 * @param response_uri Filename of response file on success
 * @param response_uri_len Size of the 'response_uri' buffer.
 */
void httpd_post_finished(void *connection,
    char *response_uri,
    u16_t response_uri_len) {

    uint16_t err = 0;

    if (uploader != NULL) {
        err = uploader_error(uploader);
        const bool done = uploader_finish(uploader);
        uploader = NULL;
        if (!done && err == 0) {
            // The form didn't contain anything useful.
            err = 400;
        }
    } else {
        // Can we get here, eg if allocation at the start fails?
        err = 500;
    }

    if (connection != NULL && err == 0) {
        strncpy(response_uri, endpoints[http_post_uri_file_index], response_uri_len);
    } else {
        struct HttpHandlers *handlers = extract_http_handlers(connection);
        const char *uri = handlers->code_lookup(handlers, err);
        snprintf(response_uri, response_uri_len, "%s", uri ?: "");
    }
}

/* Find boundary value in the Content-Type. */
const char *
find_boundary(const char *content_type) {

#define BOUNDARY_TITLE "boundary="
    static const size_t boundary_title_len = strlen(BOUNDARY_TITLE);

    if (content_type != NULL) {
        const char *boundary_begin = strstr(content_type, BOUNDARY_TITLE); // Find Boundary= in Content-Type
        if (boundary_begin != NULL) {
            const char *boundary = boundary_begin + boundary_title_len; // Remove the Boundary=
#ifdef HTTPD_DEBUG
            _dbg("POST multipart Boundary found: %s\n", boundary);
#endif

            return boundary;
        }
    }
    return NULL;
}
