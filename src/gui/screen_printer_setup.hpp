#pragma once

#include <screen.hpp>
#include <window_header.hpp>
#include <window_menu_adv.hpp>
#include <window_text.hpp>
#include <window_menu.hpp>
#include <MItem_hardware.hpp>
#include <WinMenuContainer.hpp>
#include <common/extended_printer_type.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif

namespace screen_printer_setup_private {

class MI_DONE : public IWindowMenuItem {

public:
    MI_DONE();

protected:
    void click(IWindowMenu &menu) override;
};

class MI_NOZZLE_DIAMETER_HELP : public IWindowMenuItem {

public:
    MI_NOZZLE_DIAMETER_HELP();

protected:
    void click(IWindowMenu &menu) override;
};

class ScreenPrinterSetup : public screen_t {

public:
    ScreenPrinterSetup();

private:
    window_header_t header;
    window_text_t prompt;
    window_menu_t menu;

    WinMenuContainer<
        MI_EXTENDED_PRINTER_TYPE, //< Show always, for non-extended models, there is a non-changeable WiInfo
        MI_NOZZLE_DIAMETER,
#if PRINTER_IS_PRUSA_XL
        MI_NOZZLE_DIAMETER_HELP,
#endif
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
        MI_NOZZLE_TYPE,
#endif
        MI_NOZZLE_SOCK,
#if HAS_MMU2()
        MI_MMU_NEXTRUDER_REWORK,
#endif
        MI_DONE>
        menu_container;
};

} // namespace screen_printer_setup_private

using screen_printer_setup_private::ScreenPrinterSetup;
