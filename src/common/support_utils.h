#ifndef _UTILS_H
#define _UTILS_H

#include "qrcodegen.h"

#define MAX_LEN_4QR 256 //143

#ifdef __cplusplus
extern "C" {
#endif

extern char *eofstr(char *str);
extern void append_crc(char *str, uint32_t str_size);

extern void error_url_long(char *str, uint32_t str_size, int error_code);
extern void error_url_short(char *str, uint32_t str_size, int error_code);
extern void create_path_info_4service(char *str, uint32_t str_size);

#ifdef __cplusplus
}
#endif

#endif // _UTILS_H
