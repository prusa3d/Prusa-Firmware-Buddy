#ifndef WUI_REQUEST_PARSER_H
#define WUI_REQUEST_PARSER_H

#include "httpd.h"

uint32_t httpd_json_parser(char *json, uint32_t len);

#endif // WUI_REQUEST_PARSER_H
