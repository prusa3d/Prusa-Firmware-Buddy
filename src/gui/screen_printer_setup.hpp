#pragma once

#include <window_header.hpp>
#include <window_menu_adv.hpp>
#include <window_text.hpp>
#include <window_menu.hpp>
#include <MItem_hardware.hpp>
#include <WinMenuContainer.hpp>
#include <screen_menu.hpp>
#include <common/extended_printer_type.hpp>

#include <MItem_menus.hpp>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <gui/menu_item/specific/menu_items_chamber_filtration.hpp>
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
        MI_EXTENDED_PRINTER_TYPE, //< Show always, for non-extended models, there is a non-changeable WiInfo
        MI_TOOLHEAD_SETTINGS,
#if HAS_CHAMBER_FILTRATION_API()
        // At least for C1, the filter addon is considered a hardware option, because it also affects the function of the cooling fans
        // BFW-6719
        MI_CHAMBER_FILTRATION_BACKEND,
#endif
        MI_DONE>;

class ScreenPrinterSetup : public ScreenBase {
public:
    ScreenPrinterSetup();
};

} // namespace screen_printer_setup_private

using screen_printer_setup_private::ScreenPrinterSetup;
