// errors.h
#ifndef _ERRORS_H
#define _ERRORS_H

typedef struct
{
    const char *error_text;
    const char *error_description;
} error_t;

extern const error_t errors[];

#endif // _ERRORS_H
