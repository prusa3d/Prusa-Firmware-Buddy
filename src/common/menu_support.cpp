// menu_support.cpp

#ifndef _EXTUI

    #include "sys.h"
    #include "../Marlin/src/lcd/menu/menu.h"

void menu_support() {
    START_MENU();
    MENU_BACK(MSG_MAIN);
    //                 01234567890123456789
    //	STATIC_ITEM_P("");
    #if (MOTHERBOARD == 1823)
    STATIC_ITEM_P("Marlin-Buddy 2209-2");
    #else
        #error "Unknown MOTHERBOARD."
    #endif
    const int version_max_len = 32;
    char version[version_max_len];
    const int build_max_len = 32;
    char build[build_max_len];
    int ver_maj = FW_VERSION / 100;
    int ver_min = (FW_VERSION - 100 * ver_maj) / 10;
    int ver_sub = FW_VERSION % 10;
    const char *stages[] = { "pre-alpha", "alpha", "beta", "RC", "final" };
    snprintf(version, version_max_len, " %d.%d.%d %s", ver_maj, ver_min, ver_sub, (char *)stages[FW_STAGENR]);
    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    const char *printer = "MINI";
    #else
    const char *printer = "???";
    #endif
    #ifdef _DEBUG
    snprintf(build, build_max_len, " %d%s (DEBUG_%s)", version_build_nr, (char *)FW_BUILDSX, printer);
    #else  //_DEBUG
    snprintf(build, build_max_len, " %d%s (%s)", version_build_nr, (char *)FW_BUILDSX, printer);
    #endif //_DEBUG
    STATIC_ITEM_P("version:            ");
    STATIC_ITEM(version);
    STATIC_ITEM_P("build:            ");
    STATIC_ITEM(build);
    END_MENU();
}

#endif //_EXTUI
