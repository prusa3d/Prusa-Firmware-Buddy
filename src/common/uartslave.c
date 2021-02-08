// uartslave.c

#include "uartslave.h"
#include <stdarg.h>
#include "stm32f4xx_hal.h"
#include "dbg.h"

//#define UARTSLAVE_TST

int uartslave_parse_mod_mask(uartslave_t *pslave, char *pstr, uint16_t *pmod_msk) {
    int ret = 0;
    char ch;
    do {
        ch = *pstr++;
        switch (ch) {
        case '!':
        case '?':
            if (ret == 0)
                *pmod_msk = UARTSLAVE_MOD_MSK_0;
            else if (ret == 3)
#ifdef UARTSLAVE_TST
                if (strncmp(pstr - 4, "tst", 3) == 0)
                    *pmod_msk = 1;
#else  //UARTSLAVE_TST
                *pmod_msk = UARTSLAVE_MOD_MSK_0;
#endif //UARTSLAVE_TST
            return ret;
        }
        ret++;
    } while (ret <= 3);
    return UARTSLAVE_ERR_SYN;
}

int uartslave_parse_cmd_id(uartslave_t *pslave, char *pstr, uint16_t *pcmd_id) {
    uint16_t cmd_id = UARTSLAVE_CMD_ID_UNK;
    if (pstr[0] == 0) {
        *pcmd_id = UARTSLAVE_CMD_ID_0;
        return 0;
    } else if ((pstr[3] == 0) || (pstr[3] == ' ')) {
#ifdef UARTSLAVE_TST
        if (strncmp(pstr, "tst", 3) == 0)
            cmd_id = 1;
#endif //UARTSLAVE_TST
        if (cmd_id != UARTSLAVE_CMD_ID_UNK) {
            *pcmd_id = cmd_id;
            return 3;
        }
    }
    return UARTSLAVE_ERR_SYN;
}

int uartslave_do_cmd(uartslave_t *pslave, uint16_t mod_msk, char cmd, uint16_t cmd_id, char *pstr) {
    _dbg0("uartslave_do_cmd: pstr='%s', cmd='%c', mod_msk=%d, cmd_id = %d", pstr ? pstr : "null", cmd, mod_msk, cmd_id);
    return UARTSLAVE_OK;
}

void uartslave_init(uartslave_t *pslave, uartrxbuff_t *prxbuff, UART_HandleTypeDef *huart, int size, char *pline) {
    pslave->huart = huart;
    pslave->prxbuff = prxbuff;
    pslave->flags = 0;
    pslave->count = 0;
    pslave->size = size;
    pslave->pline = pline;
    pslave->parse_mod_mask = uartslave_parse_mod_mask;
    pslave->parse_cmd_id = uartslave_parse_cmd_id;
    pslave->do_cmd = uartslave_do_cmd;
}

void uartslave_cycle(uartslave_t *pslave) {
    int ch = -1;
    int ret;
    char *pstr;
    uint16_t mod_msk = 0;
    uint16_t cmd_id = 0;
    if (pslave->count < pslave->size) {
        while ((ch = uartrxbuff_getchar(pslave->prxbuff)) >= 0) {
            if (ch == '\r')
                ch = 0;
            if (ch == '\n')
                ch = 0;
            pslave->pline[pslave->count] = ch;
            if (ch)
                pslave->count++;
            if ((ch == 0) || (pslave->count >= pslave->size))
                break;
        }
        if (ch == UARTRXBUFF_ERR_OVERFLOW) {
            uartrxbuff_reset(pslave->prxbuff);
            pslave->count = 0;
        }
    }
    if (pslave->count >= pslave->size) { //command overflow
        _dbg0("ERROR: command overflow\n");
        pslave->count = 0;
    } else if ((pslave->count > 0) && (ch == 0)) { //line received
        //_dbg0("line received: '%s'", pslave->pline);
        if (pslave->flags & UARTSLAVE_FLG_ECHO)
            uartslave_printf(pslave, "%s\n", pslave->pline);
        pstr = pslave->pline;
        if (pslave->parse_mod_mask)
            ret = pslave->parse_mod_mask(pslave, pstr, &mod_msk);
        else
            ret = 0;
        //_dbg0("ret = %d, mod_msk = %d", ret, mod_msk);
        if (ret >= 0) {
            pstr += ret;
            ch = *pstr;
            pstr++;
            if (pslave->parse_cmd_id)
                ret = pslave->parse_cmd_id(pslave, pstr, &cmd_id);
            else
                ret = 0;
            //_dbg0("ret = %d, ch='%c', cmd_id = %d", ret, ch, cmd_id);
            if (ret >= 0) {
                pstr += ret;
                if (*pstr == 0)
                    pstr = 0;
                if (pslave->do_cmd)
                    ret = pslave->do_cmd(pslave, mod_msk, ch, cmd_id, pstr);
                else
                    ret = 0;
            } else
                ret = UARTSLAVE_ERR_SYN;
        }
        //_dbg0("ret = %d", ret);
        if (ret == UARTSLAVE_OK)
            uartslave_printf(pslave, "ok\n");
        else
            uartslave_printf(pslave, "e%d\n", -((int)ret));
        pslave->count = 0;
    }
}

int uartslave_printf(uartslave_t *pslave, const char *fmt, ...) {
    const unsigned int text_len = 32;
    char text[text_len];
    va_list va;
    va_start(va, fmt);
    const int len = vsnprintf(text, text_len, fmt, va);
    va_end(va);
    HAL_StatusTypeDef ret = HAL_UART_Transmit(pslave->huart, (uint8_t *)text, len, HAL_MAX_DELAY);
    if (ret == HAL_OK)
        return len;
    return -1;
}
