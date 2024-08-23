#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define PRINTER_CODE_SIZE 8

extern char *eofstr(char *str);

extern void printerCode(char *str);
// Similar to printerCode, but more generic.
//
// * Does _not_ set the terminating '\0' (if you need it, set it yourself).
// * Allows setting the number of characters output.
// * Allows setting if it shall be prefixed by the 2 bits of appendix and
//   signature of firmware.
extern void printerHash(char *str_buffer, size_t size, bool state_prefix);

extern bool appendix_exist();
extern bool signature_exist();

#ifdef __cplusplus
}
#endif
