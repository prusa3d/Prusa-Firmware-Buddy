#pragma once

#define PROGMEM             /* */
#define PSTR(str)           (str)
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
