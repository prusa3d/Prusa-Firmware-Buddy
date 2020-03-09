#ifndef _UTILS_H
#define _UTILS_H

#include "qrcodegen.h"

#define MAX_LEN_4QR 256 //143

extern void create_path_info_4error(char *str, int err_article);
extern void create_path_info_4service(char *str);

#endif // _UTILS_H
