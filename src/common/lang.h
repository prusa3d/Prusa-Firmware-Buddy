// lang.h
#ifndef _LANG_H
#define _LANG_H

typedef enum {
    LANG_UNDEF = 0,
    LANG_EN = 1,
    LANG_CS,
    LANG_DE,
    LANG_FR,
    LANG_ES,
    LANG_IT,
    LANG_SK,
    LANG_PL,
    LANG_BMORD = 666,
    LANG_ELV = 777,
    LANG_KLING = 999,
} lang_code_t;

typedef struct {
    lang_code_t lang_code;
    const char *err_url;
    const char *service_url;
    const char *help_text;
} lang_t;

extern void set_actual_lang(lang_code_t lang_code);
extern const lang_t *get_actual_lang(void);

#endif // _LANG_H
