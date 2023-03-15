// resource.c - generated file - do not edit!

#include "guiconfig.h"
#include "config.h"
#include "guitypes.h"
#ifdef USE_ST7789
    #include "res/cc/font_7x13.c"  //IDR_FNT_SMALL
    #include "res/cc/font_11x18.c" //IDR_FNT_NORMAL
    //#include "res/cc/font_10x18.c" //IDR_FNT_NORMAL
    #include "res/cc/font_12x21.c" //IDR_FNT_BIG
    #include "res/cc/font_9x16.c"  //IDR_FNT_SPECIAL
#endif                             // USE_ST7789
#ifdef USE_ILI9488
    #include "res/cc/font_9x16_new.c" //IDR_FNT_SMALL
    #include "res/cc/font_11x19.c"    //IDR_FNT_NORMAL
    #include "res/cc/font_13x22.c"    //IDR_FNT_BIG
    #include "res/cc/font_9x15.c"     //IDR_FNT_TERMINAL
    #include "res/cc/font_30x53.c"    //IDR_FNT_LARGE
    #include "res/cc/font_9x16.c"     //IDR_FNT_SPECIAL
#endif                                // USE_ILI9488

RESOURCE_TABLE_BEGIN
//fonts
#ifdef USE_ST7789
RESOURCE_ENTRY_FNT(font_7x13)  //IDR_FNT_SMALL
RESOURCE_ENTRY_FNT(font_11x18) //IDR_FNT_NORMAL
RESOURCE_ENTRY_FNT(font_12x21) //IDR_FNT_BIG
RESOURCE_ENTRY_FNT(font_9x16)  //IDR_FNT_SPECIAL
#endif                         // USE_ST7789

#ifdef USE_ILI9488
RESOURCE_ENTRY_FNT(font_9x16_new) //IDR_FNT_SMALL
RESOURCE_ENTRY_FNT(font_11x19)    //IDR_FNT_NORMAL
RESOURCE_ENTRY_FNT(font_13x22)    //IDR_FNT_BIG
RESOURCE_ENTRY_FNT(font_9x16)     //IDR_FNT_SPECIAL
RESOURCE_ENTRY_FNT(font_30x53)    //IDR_FNT_LARGE
#endif                            // USE_ILI9488

RESOURCE_TABLE_END
