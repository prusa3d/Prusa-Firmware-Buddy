// menu_support.cpp

#ifndef _EXTUI

    #include "sys.h"
    #include "../Marlin/src/lcd/menu/menu.h"

void menu_support() {
    START_MENU();
    MENU_BACK(MSG_MAIN);
    //                 01234567890123456789
    //	STATIC_ITEM_P("");
    #if (BOARD == A3IDES2209_REV01)
    STATIC_ITEM_P("Marlin-A3ides 2209");
    #elif (BOARD == A3IDES2209_REV02)
    STATIC_ITEM_P("Marlin-A3ides 2209-2");
    #elif (BOARD == A3IDES2130_REV01)
    STATIC_ITEM_P("Marlin-A3ides 2130");
    #else
    STATIC_ITEM_P("Marlin - A3ides ????");
    #endif
    static const uint32_t version_size = 32;
    static const uint32_t build_size = 32;
    char version[version_size];
    char build[build_size];
    int ver_maj = FW_VERSION / 100;
    int ver_min = (FW_VERSION - 100 * ver_maj) / 10;
    int ver_sub = FW_VERSION % 10;
    const char *stages[] = { "pre-alpha", "alpha", "beta", "RC", "final" };
    snprintf(version, version_size, " %d.%d.%d %s", ver_maj, ver_min, ver_sub, (char *)stages[FW_STAGENR]);
    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    const char *printer = "MINI";
    #else
    const char *printer = "???";
    #endif
    #ifdef _DEBUG
    sprintf(build, " %d%s (DEBUG_%s)", version_build_nr, (char *)FW_BUILDSX, printer);
    #else //_DEBUG
    snprintf(build, build_size, " %d%s (%s)", version_build_nr, (char *)FW_BUILDSX, printer);
    #endif //_DEBUG
    STATIC_ITEM_P("version:            ");
    STATIC_ITEM(version);
    STATIC_ITEM_P("build:            ");
    STATIC_ITEM(build);
    END_MENU();
}

#endif //_EXTUI
