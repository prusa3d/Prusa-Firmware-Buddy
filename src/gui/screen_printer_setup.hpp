#pragma once

#include <window_header.hpp>
#include <window_menu_adv.hpp>
#include <window_text.hpp>
#include <window_menu.hpp>
#include <MItem_hardware.hpp>
#include <WinMenuContainer.hpp>
#include <screen_menu.hpp>
#include <common/extended_printer_type.hpp>
#include <common/printer_variant/printer_variant.hpp>
#include <option/has_chamber_vents.h>
#include <option/has_expansion_joints_gen_2.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_15gt_belts.h>

#include <MItem_menus.hpp>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <gui/menu_item/specific/menu_items_chamber_filtration.hpp>
#endif

#include <option/xbuddy_extension_variant.h>
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

namespace screen_printer_setup_private {

class MI_DONE : public IWindowMenuItem {

public:
    MI_DONE();

protected:
    void click(IWindowMenu &menu) override;
};

using ScreenBase
    = ScreenMenu<EFooter::Off,
        MI_EXTENDED_PRINTER_TYPE, //< Show always; non-changeable WiInfo if !IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE
#if HAS_PRINTER_VARIANT()
        MI_PRINTER_VARIANT,
#endif
        MI_TOOLHEAD_SETTINGS,
#if HAS_CHAMBER_FILTRATION_API()
        // At least for C1, the filter addon is considered a hardware option, because it also affects the function of the cooling fans
        // BFW-6719
        MI_CHAMBER_FILTRATION_BACKEND,
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        MI_CAM_USB_PWR,
#endif
#if HAS_CHAMBER_VENTS()
        MI_SWITCH_VENT_MECHANISM,
#endif
#if HAS_EXPANSION_JOINTS_GEN_2()
        MI_EXPANSION_JOINTS_GEN_2,
#endif
#if HAS_NOZZLE_CLEANER_LITE()
        MI_NOZZLE_CLEANER_LITE,
#endif
#if HAS_15GT_BELTS()
        MI_BELTS_15GT,
#endif
        MI_DONE>;

class ScreenPrinterSetup : public ScreenBase {
public:
    ScreenPrinterSetup();

    [[nodiscard]] static bool should_show();
};

} // namespace screen_printer_setup_private

using screen_printer_setup_private::ScreenPrinterSetup;
