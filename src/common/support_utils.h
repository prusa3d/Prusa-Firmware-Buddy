#ifndef _UTILS_H
#define _UTILS_H

#include "qrcodegen.h"

#define ER_URL "HTTP://HELP.PRUSA3D.COM/"
#define IR_URL "HTTP://INFO.PRUSA3D.COM/"

#define MAX_LEN_4QR 256 //143

#ifdef __cplusplus
extern "C" {
#endif

extern char *eofstr(char *str);
extern void append_crc(char *str);

extern void create_path_info_4error(char *str, int error_code);
extern void create_path_info_4service(char *str);

#ifdef __cplusplus
}
#endif

#endif // _UTILS_H
