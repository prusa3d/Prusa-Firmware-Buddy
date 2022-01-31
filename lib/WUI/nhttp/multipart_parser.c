/* Based on node-formidable by Felix Geisend�rfer
 * Igor Afonov - afonov@gmail.com - 2012
 * MIT License - http://www.opensource.org/licenses/mit-license.php
 * https://raw.githubusercontent.com/iafonov/multipart-parser-c/master/multipart_parser.c
 */

#include "multipart_parser.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lwip/opt.h"
#include "lwip/def.h"

//#define DEBUG_MULTIPART 1

static void multipart_log(const char *format, ...) {
#ifdef DEBUG_MULTIPART
    va_list args;
    va_start(args, format);

    fprintf(stderr, "[HTTP_MULTIPART_PARSER] %s:%d: ", __FILE__, __LINE__);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif
}

#define NOTIFY_CB(FOR)                           \
    do {                                         \
        if (p->settings->on_##FOR) {             \
            if (p->settings->on_##FOR(p) != 0) { \
                return i;                        \
            }                                    \
        }                                        \
    } while (0)

#define EMIT_DATA_CB(FOR, ptr, len)                        \
    do {                                                   \
        if (p->settings->on_##FOR) {                       \
            if (p->settings->on_##FOR(p, ptr, len) != 0) { \
                return i;                                  \
            }                                              \
        }                                                  \
    } while (0)

#define LF 10
#define CR 13

struct multipart_parser {
    void *data;

    size_t index;
    size_t boundary_length;

    unsigned char state;

    const multipart_parser_settings *settings;

    char *lookbehind;
    char multipart_boundary[1];
};

enum state {
    s_uninitialized = 1,
    s_start,
    s_start_boundary,
    s_header_field_start,
    s_header_field,
    s_headers_almost_done,
    s_header_value_start,
    s_header_value,
    s_header_value_almost_done,
    s_part_data_start,
    s_part_data,
    s_part_data_almost_boundary,
    s_part_data_boundary,
    s_part_data_almost_end,
    s_part_data_end,
    s_part_data_final_hyphen,
    s_end
};

multipart_parser *multipart_parser_init(const char *boundary, const multipart_parser_settings *settings) {

    uint32_t endstring_length = strlen(boundary);
    char *endline_begin = strstr(boundary, "\r\n");
    int32_t boundary_length = LWIP_MIN(endstring_length, (uint32_t)(endline_begin - boundary));
    multipart_parser *p = malloc(sizeof(multipart_parser) + boundary_length + boundary_length + 9 + 2); // Add 2 because the boundary is missing 2 --

    strcpy(p->multipart_boundary, "--");     // Boundary seems to be missing the first 2 --, so add them here (Tested on Chrome and Firefox)
    strcat(p->multipart_boundary, boundary); // Copy the boundary to parser
    p->boundary_length = boundary_length + 2;

    p->lookbehind = (p->multipart_boundary + p->boundary_length + 1);

    p->index = 0;
    p->state = s_start;
    p->settings = settings;

    return p;
}

void multipart_parser_free(multipart_parser *p) {
    free(p);
}

void multipart_parser_set_data(multipart_parser *p, void *data) {
    p->data = data;
}

void *multipart_parser_get_data(multipart_parser *p) {
    return p->data;
}

size_t multipart_parser_execute(multipart_parser *p, const char *buf, size_t len) {
    size_t i = 0;
    size_t mark = 0;
    char c, cl;
    int is_last = 0;

    multipart_log(p->data);
    multipart_log(p->multipart_boundary);

    while (i < len) {
        c = buf[i];
        is_last = (i == (len - 1));
        switch (p->state) {
        case s_start:
            multipart_log("s_start");
            p->index = 0;
            p->state = s_start_boundary;

        /* fallthrough */
        case s_start_boundary:
            multipart_log("s_start_boundary");
            if (p->index == p->boundary_length) {
                if (c != CR) {
                    return i;
                }
                p->index++;
                break;
            } else if (p->index == (p->boundary_length + 1)) {
                if (c != LF) {
                    return i;
                }
                p->index = 0;
                NOTIFY_CB(part_data_begin);
                p->state = s_header_field_start;
                break;
            }
            if (c != p->multipart_boundary[p->index]) {
                return i;
            }
            p->index++;
            break;

        case s_header_field_start:
            multipart_log("s_header_field_start");
            mark = i;
            p->state = s_header_field;

        /* fallthrough */
        case s_header_field:
            multipart_log("s_header_field");
            if (c == CR) {
                p->state = s_headers_almost_done;
                break;
            }

            if (c == ':') {
                EMIT_DATA_CB(header_field, buf + mark, i - mark);
                p->state = s_header_value_start;
                break;
            }

            cl = tolower(c);
            /*
             * FIXME: This one is probably wrong. The multipart/form-data
             * derives from multipart/ * content types from RFC1341. That one
             * derives the individual bodies from the email RFC822 where in the
             * 3.2 section the definition of header name is:
             *
             * field-name  =  1*<any CHAR, excluding CTLs, SPACE, and ":">
             *
             * But ignoring it for now...
             */
            if ((c != '-') && (cl < 'a' || cl > 'z')) {
                multipart_log("invalid character in header name");
                return i;
            }
            if (is_last)
                EMIT_DATA_CB(header_field, buf + mark, (i - mark) + 1);
            break;

        case s_headers_almost_done:
            multipart_log("s_headers_almost_done");
            if (c != LF) {
                return i;
            }

            p->state = s_part_data_start;
            break;

        case s_header_value_start:
            multipart_log("s_header_value_start");
            if (c == ' ') {
                break;
            }

            mark = i;
            p->state = s_header_value;

        /* fallthrough */
        case s_header_value:
            multipart_log("s_header_value");
            if (c == CR) {
                EMIT_DATA_CB(header_value, buf + mark, i - mark);
                p->state = s_header_value_almost_done;
                break;
            }
            if (is_last)
                EMIT_DATA_CB(header_value, buf + mark, (i - mark) + 1);
            break;

        case s_header_value_almost_done:
            multipart_log("s_header_value_almost_done");
            if (c != LF) {
                return i;
            }
            p->state = s_header_field_start;
            break;

        case s_part_data_start:
            multipart_log("s_part_data_start");
            NOTIFY_CB(headers_complete);
            mark = i;
            p->state = s_part_data;

        /* fallthrough */
        case s_part_data:
            multipart_log("s_part_data");
            if (c == CR) {
                EMIT_DATA_CB(part_data, buf + mark, i - mark);
                mark = i;
                p->state = s_part_data_almost_boundary;
                p->lookbehind[0] = CR;
                break;
            }
            if (is_last)
                EMIT_DATA_CB(part_data, buf + mark, (i - mark) + 1);
            break;

        case s_part_data_almost_boundary:
            multipart_log("s_part_data_almost_boundary");
            if (c == LF) {
                p->state = s_part_data_boundary;
                p->lookbehind[1] = LF;
                p->index = 0;
                break;
            }
            EMIT_DATA_CB(part_data, p->lookbehind, 1);
            p->state = s_part_data;
            mark = i--;
            break;

        case s_part_data_boundary:
            multipart_log("s_part_data_boundary");
            if (p->multipart_boundary[p->index] != c) {
                EMIT_DATA_CB(part_data, p->lookbehind, 2 + p->index);
                p->state = s_part_data;
                mark = i--;
                break;
            }
            p->lookbehind[2 + p->index] = c;
            if ((++p->index) == p->boundary_length) {
                NOTIFY_CB(part_data_end);
                p->state = s_part_data_almost_end;
            }
            break;

        case s_part_data_almost_end:
            multipart_log("s_part_data_almost_end");
            if (c == '-') {
                p->state = s_part_data_final_hyphen;
                break;
            }
            if (c == CR) {
                p->state = s_part_data_end;
                break;
            }
            return i;

        case s_part_data_final_hyphen:
            multipart_log("s_part_data_final_hyphen");
            if (c == '-') {
                NOTIFY_CB(body_end);
                p->state = s_end;
                break;
            }
            return i;

        case s_part_data_end:
            multipart_log("s_part_data_end");
            if (c == LF) {
                p->state = s_header_field_start;
                NOTIFY_CB(part_data_begin);
                break;
            }
            return i;

        case s_end:
            multipart_log("s_end: %02X", (int)c);
            break;

        default:
            multipart_log("Multipart parser unrecoverable error");
            return 0;
        }
        ++i;
    }

    return len;
}
