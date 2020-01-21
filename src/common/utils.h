#ifndef _UTILS_H
#define _UTILS_H

#include "qrcodegen.h"


#define ER_URL "HTTP://HELP.PRUSA3D.COM/"
#define X0 65
#define Y0 166
#define MS 2
#define BORDER 4

#define qrcodegen_VERSION 9
#define qrcodegen_ECC qrcodegen_Ecc_HIGH
#define MAX_LEN_4QR 143

#ifdef __cplusplus
extern bool createQR(char* str, enum qrcodegen_Ecc qrcodegen_ecl=qrcodegen_ECC);
extern "C" {
#endif


extern char* eofstr(char* str);
extern void appendCRC(char* str);

extern void get_path_info(char* str, int error_code);
extern void create_path_info(char* str, int error_code);

extern bool getQR(char* str, uint8_t* pData, enum qrcodegen_Ecc qrcodegen_ecl);
extern void drawQR(uint8_t* pData);


#ifdef __cplusplus
}
#endif

#endif // _UTILS_H
