// dump.h

#ifndef _DUMP_H
#define _DUMP_H

#define DUMP_HARDFAULT_TO_CCRAM


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void dump_to_xflash(void);

extern int dump_save_xflash_to_usb(const char* fn);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_H
