/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include <MItem_tools.hpp>
#include <option/has_bed_fan.h>
#include <option/has_cpu_fan.h>
#include <option/has_psu_fan.h>
#include <option/has_xl_can.h>
#include <option/xbuddy_extension_variant.h>
#include <WindowItemFanLabel.hpp>

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

class MI_INFO_PRINT_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_PRINT_FAN();
};

class MI_INFO_HBR_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_HBR_FAN();
};

#if HAS_BED_FAN()
class MI_INFO_BED_FAN1 : public WI_FAN_LABEL_t {
public:
    MI_INFO_BED_FAN1();

private:
    StringViewUtf8Parameters<2> label_params_;
};

class MI_INFO_BED_FAN2 : public WI_FAN_LABEL_t {
public:
    MI_INFO_BED_FAN2();

private:
    StringViewUtf8Parameters<2> label_params_;
};
#endif

#if HAS_PSU_FAN()
class MI_INFO_PSU_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_PSU_FAN();
};
#endif

#if HAS_CPU_FAN()
class MI_INFO_CPU_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_CPU_FAN();
};
#endif

#if HAS_XL_CAN()
class MI_INFO_BED_MCU_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_BED_MCU_FAN();
};
#endif

using ScreenMenuFanInfo_ = BasicScreenMenu<
    MI_INFO_PRINT_FAN,
    MI_INFO_HBR_FAN,
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    MI_INFO_XBUDDY_EXTENSION_FAN1,
    MI_INFO_XBUDDY_EXTENSION_FAN2,
    MI_INFO_XBUDDY_EXTENSION_FAN3,
#endif
#if HAS_BED_FAN()
    MI_INFO_BED_FAN1,
    MI_INFO_BED_FAN2,
#endif
#if HAS_PSU_FAN()
    MI_INFO_PSU_FAN,
#endif
#if HAS_CPU_FAN()
    MI_INFO_CPU_FAN,
#endif
#if HAS_XL_CAN()
    MI_INFO_BED_MCU_FAN,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuFanInfo final : public ScreenMenuFanInfo_ {
public:
    ScreenMenuFanInfo();
};
