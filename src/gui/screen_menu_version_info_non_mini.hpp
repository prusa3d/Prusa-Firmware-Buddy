/// @file
#pragma once

#include "MItem_menus.hpp"
#include <basic_screen_menu.hpp>
#include <option/bootloader.h>
#include <option/has_mmu2.h>
#include <option/has_cyphal.h>
#include <WindowMenuInfo.hpp>

#if HAS_CYPHAL()
    #include <screen_menu_cyphal_info.hpp>
#endif

class MI_INFO_FW final : public WI_INFO_t {
public:
    MI_INFO_FW();
};

#if BOOTLOADER()
class MI_INFO_BOOTLOADER final : public WI_INFO_t {
public:
    MI_INFO_BOOTLOADER();
};
#endif

#if HAS_MMU2()
class MI_INFO_MMU final : public WI_INFO_t {
public:
    MI_INFO_MMU();
};
#endif

using ScreenMenuVersionInfo__ = BasicScreenMenu<
    MI_INFO_FW,
#if BOOTLOADER()
    MI_INFO_BOOTLOADER,
#endif
#if HAS_MMU2()
    MI_INFO_MMU,
#endif
    MI_BOARD_INFO
#if HAS_CYPHAL()
    ,
    MI_CYPHAL_INFO
#endif
    >;

class ScreenMenuVersionInfo final : public ScreenMenuVersionInfo__ {
public:
    ScreenMenuVersionInfo();
};
