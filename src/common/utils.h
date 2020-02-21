#ifndef _UTILS_H
#define _UTILS_H

#include "qrcodegen.h"


#define ER_URL "HTTP://HELP.PRUSA3D.COM/"
#define IR_URL "HTTP://INFO.PRUSA3D.COM/"

#define MAX_LEN_4QR 256//143

#ifdef __cplusplus
extern "C" {
#endif


extern char* eofstr(char* str);
extern void appendCRC(char* str);

extern void get_path_info(char* str, int error_code);
extern void create_path_info(char* str, int error_code);
extern void get_path_info2(char* str);
extern void create_path_info2(char* str);


#ifdef __cplusplus
}
#endif

#endif // _UTILS_H
