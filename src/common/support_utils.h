#pragma once

#include "qrcodegen.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRINTER_CODE_SIZE 8

extern char *eofstr(char *str);
extern void append_crc(char *str, const uint32_t str_size);

extern void error_url_long(char *str, const uint32_t str_size, const int error_code);
extern void error_url_short(char *str, const uint32_t str_size, const int error_code);

extern void printerCode(char *str);

extern bool appendix_exist();
extern bool signature_exist();

#ifdef __cplusplus
}
#endif
