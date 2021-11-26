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

#include <file_list_defs.h> // FILE_NAME_MAX_LEN - but why do we take one from GUI???
#include <dbg.h>

#include "httpd.h"
#include "multipart_parser.h"
#include "lwip/def.h"

#define DEFAULT_UPLOAD_FILENAME "tmp.gcode"

typedef enum {
    UPLOAD_PROCESS_NONE,
    UPLOAD_PROCESS_PATH,
    UPLOAD_PROCESS_PRINT,
    UPLOAD_PROCESS_FILE
} upload_process_phase_t;

typedef struct {
    char filename[FILE_NAME_MAX_LEN];
    uint8_t start_print;
    upload_process_phase_t phase;
} upload_process_t;

// List of accepted URI for POST requests
static uint8_t http_post_uri_file_index = 0;
static uint32_t http_post_content_len = 0;
#define HTTP_POST_URI_NUM 2
const char *endpoints[HTTP_POST_URI_NUM] = {
    "/api/files/sdcard",
    "/api/files/local"
};

/*
 * Mulitpart Parser settings
 *
 * read_on_part_data_begin: (nul
 * read_header_name: Content-Disposition: read_header_value: form-data; name="key_name"
 * read_on_headers_complete: (null)
 * read_part_data: form_value			// May be called multiple times if a file
 * read_on_part_data_end: (null)
 * read_on_body_end: (null)
 *
 */
static multipart_parser_settings callbacks;
static multipart_parser *_parser;

static upload_process_t upload = {
    .filename = { 0 },
    .start_print = 0,
    .phase = UPLOAD_PROCESS_NONE
};

/* Header which contains the Key with the name */
static int read_header_name(multipart_parser *p, const char *at, size_t length) {
#ifdef HTTPD_DEBUG
    _dbg("read_header_name: %.*s: \n", length, at);
#endif

    /* Parse the Header Value */
    /* Content-Disposition: read_header_value: form-data; name="variable_name" */
    const char *key_name = find_header_name(at);

#ifdef HTTPD_DEBUG
    _dbg("Key Name: %s\n", key_name);
#endif
    if (lwip_strnicmp(key_name, "print", 5) == 0) {
        upload.phase = UPLOAD_PROCESS_PRINT;
    } else if (lwip_strnicmp(key_name, "path", 4) == 0) {
        upload.phase = UPLOAD_PROCESS_PATH;
    } else if (lwip_strnicmp(key_name, "file", 4) == 0) {
        upload.phase = UPLOAD_PROCESS_FILE;
        char *filename = strstr(key_name + 4 + 3, "filename");
        const char *end_filename = strtok(filename + 8 + 2, "\"");
        uint32_t filename_length = strlen(end_filename);
        strncpy(upload.filename, end_filename, filename_length);
    }
    return 0;
}

static int read_header_value(multipart_parser *p, const char *at, size_t length) {
#ifdef HTTPD_DEBUG
    _dbg("read_header_value: %.*s\n", length, at);
#endif
    return 0;
}

/* Value for the latest key */
/* If this is a file, this may be called multiple times. */
/* Wait until part_end for the complete file. */
static int read_part_data(multipart_parser *p, const char *at, size_t length) {
#ifdef HTTPD_DEBUG
    _dbg("read_part_data: %.*s\n", length, at);
#endif
    switch (upload.phase) {
    case UPLOAD_PROCESS_PRINT:
        if (lwip_strnicmp(at, "true", 4) == 0) {
            upload.start_print = 1;
        }
        break;
    case UPLOAD_PROCESS_PATH:
        //ignored for now => everything in the root folder
        break;
    case UPLOAD_PROCESS_FILE: {
        struct HttpHandlers *handlers = multipart_parser_get_data(p);
        return handlers->gcode_data(handlers, at, length);
    }
    default:
        break;
    }
    return 0;
}

/* Beginning of a key and value */
static int read_on_part_data_begin(multipart_parser *p) {
#ifdef HTTPD_DEBUG
    _dbg("read_on_part_data_begin:\n");
#endif
    return 0;
}

/* End of header which contains the key */
static int read_on_headers_complete(multipart_parser *p) {
#ifdef HTTPD_DEBUG
    _dbg("read_on_headers_complete:\n");
#endif
    return 0;
}

/** End of the key and value */
/* If this is a file, the file is complete. */
/* If this is a value, then the value is complete. */
static int read_on_part_data_end(multipart_parser *p) {
#ifdef HTTPD_DEBUG
    _dbg("read_on_part_data_end:\n");
#endif
    return 0;
}

/* End of the entire form */
static int read_on_body_end(multipart_parser *p) {
#ifdef HTTPD_DEBUG
    _dbg("read_on_body_end:\n");
#endif
    return 0;
}

static err_t
http_parse_post(char *data, uint32_t length) {
#ifdef HTTPD_DEBUG
    _dbg("http_parse_post POST data: %s\n", data);
#endif

    /* Parse the data */
    multipart_parser_execute(_parser, data, length);

    return ERR_OK;
}

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

    if (upload.phase != UPLOAD_PROCESS_NONE) {
        uri = "/503";
        goto invalid;
    }

    struct HttpHandlers *handlers = extract_http_handlers(connection);

    if (!authenticate_post(handlers, http_request, http_request_len)) {
        uri = "/401";
        goto invalid;
    }

    char *content_type_tag = lwip_strnstr(http_request, CONTENT_TYPE, http_request_len);
    if (content_type_tag == NULL) {
        uri = "/400";
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

            memset(&callbacks, 0, sizeof(multipart_parser_settings));

            callbacks.on_header_field = read_header_name;
            callbacks.on_header_value = read_header_value;
            callbacks.on_part_data = read_part_data;
            callbacks.on_part_data_begin = read_on_part_data_begin;
            callbacks.on_headers_complete = read_on_headers_complete;
            callbacks.on_part_data_end = read_on_part_data_end;
            callbacks.on_body_end = read_on_body_end;

            /*
			 * Get the boundary from the content-type
			 * Then pass it to the parser
			 */
            const char *boundary = find_boundary(http_request);
            if (boundary != NULL) {
                _parser = multipart_parser_init(boundary, &callbacks);
                multipart_parser_set_data(_parser, handlers);
                if (handlers->gcode_start(handlers, DEFAULT_UPLOAD_FILENAME) != 0) {
                    uri = "/500";
                    goto invalid;
                }
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

    if (connection != NULL && p != NULL) {
        data = p->payload;
        ret_val = http_parse_post(data, p->len);
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

    uint32_t status_code = 200;

    struct HttpHandlers *handlers = extract_http_handlers(connection);
    status_code = handlers->gcode_finish(handlers, DEFAULT_UPLOAD_FILENAME, upload.filename, upload.start_print);

    if (connection != NULL && status_code == 200) {
        strncpy(response_uri, endpoints[http_post_uri_file_index], response_uri_len);
    } else {
        snprintf(response_uri, response_uri_len, "/%" PRIu32, status_code);
    }

    /* End the parser */
    multipart_parser_free(_parser);
    memset(&upload, 0, sizeof(upload_process_t));
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

/* Find Header Key Name in the header. */
const char *
find_header_name(const char *header) {

#define HEADER_NAME_TITLE "name="

    if (header == NULL) {
        return NULL;
    }

    // FIXME: This is wrong, as it un-constifies the parameter.
    //
    // It's even worse, because the `const char *header` below is actually
    // modified by putting the `\0` delimiters in there by strtok!
    char *header_name_begin = strstr(header, HEADER_NAME_TITLE); // Find name= in Header
    if (header_name_begin == NULL) {
        return NULL;
    }
    char *saveptr = NULL;
    char *header_name = strtok_r(header_name_begin, "\"", &saveptr); // Find the first "
    if (header_name == NULL) {
        return NULL;
    }
    header_name = strtok_r(NULL, "\"", &saveptr); // Go to the last "
#ifdef HTTPD_DEBUG
    if (header_name != NULL) {
        _dbg("POST multipart Header Key found: %s\n", header_name);
    }
#endif
    return header_name;
}
