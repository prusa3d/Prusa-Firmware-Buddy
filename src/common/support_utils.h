#pragma once

#include "qrcodegen.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *eofstr(char *str);
extern void append_crc(char *str, const uint32_t str_size);

extern void error_url_long(char *str, const uint32_t str_size, const int error_code);
extern void error_url_short(char *str, const uint32_t str_size, const int error_code);
extern void create_path_info_4service(char *str, const uint32_t str_size);

extern void printerCode(char *str);

extern bool appendix_exist();

#ifdef __cplusplus
}
#endif
