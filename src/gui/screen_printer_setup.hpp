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

namespace screen_printer_setup_private {

#if HAS_TOOLCHANGER()
template <typename Item, uint8_t tool_index_>
class MI_TOOL_SPECIFIC_ITEM : public Item {
public:
    MI_TOOL_SPECIFIC_ITEM()
        : Item(tool_index_) {
        this->set_is_hidden(!prusa_toolchanger.is_tool_enabled(this->tool_index));
    }
};

template <typename ToolSpecificItem, typename... ItemsOnEnd>
using ScreenPropertySpecific_ = ScreenMenu<EFooter::Off, MI_RETURN, MI_TOOL_SPECIFIC_ITEM<ToolSpecificItem, 0>, MI_TOOL_SPECIFIC_ITEM<ToolSpecificItem, 1>, MI_TOOL_SPECIFIC_ITEM<ToolSpecificItem, 2>, MI_TOOL_SPECIFIC_ITEM<ToolSpecificItem, 3>, MI_TOOL_SPECIFIC_ITEM<ToolSpecificItem, 4>, ItemsOnEnd...>;

template <typename ToolSpecificItem, typename... ItemsOnEnd>
class ScreenPropertySpecific : public ScreenPropertySpecific_<ToolSpecificItem, ItemsOnEnd...> {
public:
    ScreenPropertySpecific(const string_view_utf8 &label)
        : ScreenPropertySpecific_<ToolSpecificItem, ItemsOnEnd...>(label) {}
};
#endif

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

#if HAS_TOOLCHANGER()
class MI_NOZZLE_DIAMETER_T : public WiSpin {

public:
    MI_NOZZLE_DIAMETER_T(uint8_t tool_index = 0);

    const uint8_t tool_index;

protected:
    void OnClick() final;
};

class MI_NOZZLE_DIAMETER_MENU : public IWindowMenuItem {

public:
    MI_NOZZLE_DIAMETER_MENU();

protected:
    void click(IWindowMenu &) final;
};

class ScreenNozzleDiameter final : public ScreenPropertySpecific<MI_NOZZLE_DIAMETER_T, MI_NOZZLE_DIAMETER_HELP> {
public:
    ScreenNozzleDiameter();
};
#endif

using ScreenBase
    = ScreenMenu<EFooter::Off,
        MI_EXTENDED_PRINTER_TYPE, //< Show always, for non-extended models, there is a non-changeable WiInfo
#if HAS_TOOLCHANGER()
        MI_NOZZLE_DIAMETER_HELP,
        MI_NOZZLE_DIAMETER_MENU,
#endif
        MI_NOZZLE_DIAMETER,
        MI_TOOLHEAD_SETTINGS,
        MI_HOTEND_SOCK_OR_TYPE,
#if HAS_MMU2()
        MI_MMU_NEXTRUDER_REWORK,
#endif
        MI_DONE>;

class ScreenPrinterSetup : public ScreenBase {
public:
    ScreenPrinterSetup();
};

} // namespace screen_printer_setup_private

using screen_printer_setup_private::ScreenPrinterSetup;
