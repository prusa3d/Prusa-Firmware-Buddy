// uartslave.h
#ifndef _UARTSLAVE_H
#define _UARTSLAVE_H

#include "uartrxbuff.h"

#define UARTSLAVE_FLG_ECHO 0x01

#define UARTSLAVE_MOD_MSK_0 0x0000

#define UARTSLAVE_CMD_ID_0   0x0000
#define UARTSLAVE_CMD_ID_UNK 0xffff

#define UARTSLAVE_OK      0  //ok - success
#define UARTSLAVE_ERR_UNK -1 //error 1 - unknown/unspecified failure
#define UARTSLAVE_ERR_BSY -2 //error 2 - busy
#define UARTSLAVE_ERR_SYN -3 //error 3 - syntax error
#define UARTSLAVE_ERR_OOR -4 //error 4 - parameter out of range
#define UARTSLAVE_ERR_ONP -5 //error 5 - operation not permitted
#define UARTSLAVE_ERR_NUL -6 //error 6 - null pointer
#define UARTSLAVE_ERR_CNF -7 //error 7 - command not found

typedef struct _uartslave_t uartslave_t;

typedef int(uartslave_parse_mod_mask_t)(uartslave_t *pslave, char *pstr, uint16_t *pmod_msk);
typedef int(uartslave_parse_cmd_id_t)(uartslave_t *pslave, char *pstr, uint16_t *pcmd_id);
typedef int(uartslave_do_cmd_t)(uartslave_t *pslave, uint16_t mod_msk, char cmd, uint16_t pcmd_id, char *pstr);

#pragma pack(push)
#pragma pack(1)

typedef struct _uartslave_t {
    uartrxbuff_t *prxbuff;
    uint8_t flags;
    int count;
    int size;
    char *pline;
    uartslave_parse_mod_mask_t *parse_mod_mask;
    uartslave_parse_cmd_id_t *parse_cmd_id;
    uartslave_do_cmd_t *do_cmd;
} uartslave_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void uartslave_init(uartslave_t *pslave, uartrxbuff_t *prxbuff, int size, char *pline);

extern void uartslave_cycle(uartslave_t *pslave);

int uartslave_printf(uartslave_t *pslave, const char *fmt, ...);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _UARTSLAVE_H
